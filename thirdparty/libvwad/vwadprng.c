/* coded by Ketmar Dark */
/* public domain */
/* get (almost) crypto-secure random bytes */
/* this will try to use the best PRNG available */
#include "vwadprng.h"

/*#include <stdint.h>*/
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
# include <windows.h>
#else
# include <unistd.h>
# include <fcntl.h>
# include <errno.h>
# include <time.h>
# include <sys/time.h>
# if defined(__SWITCH__)
#  include <switch/kernel/random.h> // for randomGet()
/*
# elif defined(__linux__) && !defined(ANDROID)
#  include <sys/random.h>
*/
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


// this should be 8 bit (i hope so)
typedef unsigned char xprng_ubyte;
// this should be 32 bit (i hope so)
typedef unsigned int xprng_uint;
// this should be 64 bit (i hope so)
typedef unsigned long long xprng_uint64;

// size checks
typedef char xprng_temp_typedef_check_ubyte[(sizeof(xprng_ubyte) == 1) ? 1 : -1];
typedef char xprng_temp_typedef_check_uint[(sizeof(xprng_uint) == 4) ? 1 : -1];
typedef char xprng_temp_typedef_check_uint64[(sizeof(xprng_uint64) == 8) ? 1 : -1];


static int g_xrng_strong_seed = 0;


/* turning off inlining saves ~5kb of binary code on x86 */
#ifdef _MSC_VER
# define XPRNG_INLINE  __forceinline
#else
# define XPRNG_INLINE  inline __attribute__((always_inline))
#endif


static XPRNG_INLINE xprng_uint isaac_rra (xprng_uint value, unsigned int count) { return (value>>count)|(value<<(32-count)); }
static XPRNG_INLINE xprng_uint isaac_rla (xprng_uint value, unsigned int count) { return (value<<count)|(value>>(32-count)); }

static XPRNG_INLINE void isaac_getu32 (xprng_ubyte *p, const xprng_uint v) {
  p[0] = (xprng_ubyte)(v&0xffu);
  p[1] = (xprng_ubyte)((v>>8)&0xffu);
  p[2] = (xprng_ubyte)((v>>16)&0xffu);
  p[3] = (xprng_ubyte)((v>>24)&0xffu);
}

/*
 * ISAAC+ "variant", the paper is not clear on operator precedence and other
 * things. This is the "first in, first out" option!
 */
typedef struct isaacp_state_t {
  xprng_uint state[256];
  xprng_ubyte buffer[1024];
  union __attribute__((packed)) {
    xprng_uint abc[3];
    struct __attribute__((packed)) {
      xprng_uint a, b, c;
    };
  };
  size_t left;
} isaacp_state;

#define isaacp_step(offset,mix) \
  x = mm[i+offset]; \
  a = (a^(mix))+(mm[(i+offset+128u)&0xffu]); \
  y = (a^b)+mm[(x>>2)&0xffu]; \
  mm[i+offset] = y; \
  b = (x+a)^mm[(y>>10)&0xffu]; \
  isaac_getu32(out+(i+offset)*4u, b);

static XPRNG_INLINE void isaacp_mix (isaacp_state *st) {
  xprng_uint x, y;
  xprng_uint a = st->a, b = st->b, c = st->c;
  xprng_uint *mm = st->state;
  xprng_ubyte *out = st->buffer;
  c = c+1u;
  b = b+c;
  for (unsigned i = 0u; i < 256u; i += 4u) {
    isaacp_step(0u, isaac_rla(a,13u))
    isaacp_step(1u, isaac_rra(a, 6u))
    isaacp_step(2u, isaac_rla(a, 2u))
    isaacp_step(3u, isaac_rra(a,16u))
  }
  st->a = a;
  st->b = b;
  st->c = c;
  st->left = 1024u;
}

static void isaacp_random (isaacp_state *st, void *p, size_t len) {
  xprng_ubyte *c = (xprng_ubyte *)p;
  while (len) {
    const size_t use = (len > st->left ? st->left : len);
    memcpy(c, st->buffer+(sizeof(st->buffer)-st->left), use);
    st->left -= use;
    c += use;
    len -= use;
    if (!st->left) isaacp_mix(st);
  }
}


// ////////////////////////////////////////////////////////////////////////// //
// SplitMix; mostly used to generate 64-bit seeds
static XPRNG_INLINE xprng_uint64 splitmix64_next (xprng_uint64 *state) {
  xprng_uint64 result = *state;
  *state = result+(xprng_uint64)0x9E3779B97f4A7C15ULL;
  result = (result^(result>>30))*(xprng_uint64)0xBF58476D1CE4E5B9ULL;
  result = (result^(result>>27))*(xprng_uint64)0x94D049BB133111EBULL;
  return result^(result>>31);
}

static XPRNG_INLINE void splitmix64_seedU64 (xprng_uint64 *state, xprng_uint seed0, xprng_uint seed1) {
  // hashU32
  xprng_uint res = seed0;
  res -= (res<<6);
  res ^= (res>>17);
  res -= (res<<9);
  res ^= (res<<4);
  res -= (res<<3);
  res ^= (res<<10);
  res ^= (res>>15);
  xprng_uint64 n = res;
  n <<= 32;
  // hashU32
  res += seed1; // `+=`!
  res -= (res<<6);
  res ^= (res>>17);
  res -= (res<<9);
  res ^= (res<<4);
  res -= (res<<3);
  res ^= (res<<10);
  res ^= (res>>15);
  n |= res;
  *state = n;
}


#ifdef WIN32
// ////////////////////////////////////////////////////////////////////////// //
// use shitdoze `RtlGenRandom()` if possible
typedef BOOLEAN WINAPI (*RtlGenRandomFn) (PVOID RandomBuffer, ULONG RandomBufferLength);

static void RtlGenRandomX (PVOID RandomBuffer, ULONG RandomBufferLength) {
  if (RandomBufferLength <= 0) return;
  RtlGenRandomFn RtlGenRandomXX = NULL;
  HMODULE libh = LoadLibraryA("advapi32.dll");
  if (libh) {
    RtlGenRandomXX = (RtlGenRandomFn)(void *)GetProcAddress(libh, "SystemFunction036");
    //if (!RtlGenRandomXX) fprintf(stderr, "WARNING: `RtlGenRandom()` is not found!\n");
    //else fprintf(stderr, "MESSAGE: `RtlGenRandom()` found!\n");
    if (RtlGenRandomXX) {
      BOOLEAN res = RtlGenRandomXX(RandomBuffer, RandomBufferLength);
      FreeLibrary(libh);
      if (res) {
        g_xrng_strong_seed = 1;
        return;
      }
      //fprintf(stderr, "WARNING: `RtlGenRandom()` fallback for %u bytes!\n", (unsigned)RandomBufferLength);
    }
  }
  // use some semi-random shit to seed ISAAC+, and ask it for random numbers
  isaacp_state rng;
  // initialise ISAAC+ with some shit
  xprng_uint smxseed0 = 0;
  xprng_uint smxseed1 = (xprng_uint)GetCurrentProcessId();
  SYSTEMTIME st;
  FILETIME ft;
  GetLocalTime(&st);
  if (!SystemTimeToFileTime(&st, &ft)) {
    //fprintf(stderr, "SHIT: `SystemTimeToFileTime()` failed!\n");
    smxseed0 = (xprng_uint)(GetTickCount());
  } else {
    smxseed0 = (xprng_uint)(ft.dwLowDateTime);
  }
  // use SplitMix to generate ISAAC+ seed
  xprng_uint64 smx;
  splitmix64_seedU64(&smx, smxseed0, smxseed1);
  for (unsigned n = 0; n < 256; ++n) rng.state[n] = splitmix64_next(&smx);
  rng.a = splitmix64_next(&smx);
  rng.b = splitmix64_next(&smx);
  rng.c = splitmix64_next(&smx);
  isaacp_mix(&rng);
  isaacp_mix(&rng);
  // generate random bytes with ISAAC+
  isaacp_random(&rng, RandomBuffer, RandomBufferLength);
}

#elif defined(__linux__) && !defined(ANDROID)
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
/*#include <stdio.h>*/

static ssize_t why_glibc_is_so_fucked_get_random (void *buf, size_t bufsize) {
  #ifdef SYS_getrandom
  # ifndef GRND_NONBLOCK
  #  define GRND_NONBLOCK (1)
  # endif
  for (;;) {
    ssize_t ret = syscall(SYS_getrandom, buf, bufsize, GRND_NONBLOCK);
    if (ret >= 0) {
      /*fprintf(stderr, "*** SYS_getrandom is here! read %u bytes out of %u\n", (unsigned)ret, (unsigned)bufsize);*/
      return ret;
    }
    if (ret != -EINTR) break;
  }
  #endif
  return -1;
}
#elif defined(__linux__) || defined(__SWITCH__)
/* ok, do nothing */
#else
# error "unsupported OS!"
#endif


//==========================================================================
//
//  randombytes_init
//
//  initialize ISAAC+
//
//==========================================================================
static void randombytes_init (isaacp_state *rng) {
  xprng_uint xstate[256+3]; /* and `abc` */
  for (unsigned f = 0u; f < 256u+3u; ++f) xstate[f] = f+666u;
  memset(rng, 0, sizeof(isaacp_state));
  #ifdef __SWITCH__
  randomGet(xstate, sizeof(xstate));
  g_xrng_strong_seed = 1; /* i hope so */
  #elif defined(WIN32)
  RtlGenRandomX(xstate, sizeof(xstate));
  #else
  size_t pos = 0;
  #if defined(__linux__) && !defined(ANDROID)
  /* try to use kernel syscall first */
  while (pos < sizeof(xstate)) {
    /* reads up to 256 bytes should not be interrupted by signals */
    size_t len = sizeof(xstate)-pos;
    if (len > 256) len = 256; /* maximum block for syscall; or at least i heard so */
    ssize_t rd = why_glibc_is_so_fucked_get_random(((xprng_ubyte *)xstate)+pos, len);
    if (rd < 0) break;
    pos += (size_t)rd;
  }
  /* do not mix additional sources if we got all random bytes from kernel */
  g_xrng_strong_seed = (pos == sizeof(xstate));
  const unsigned mixother = (pos != sizeof(xstate));
  #else
  const unsigned mixother = 1u;
  #endif
  /* fill up what is left with "/dev/urandom" */
  /* note that "/dev/urandom" is considered strong too, even if it is tampered */
  if (pos < sizeof(xstate)) {
    int fd = open("/dev/urandom", O_RDONLY/*|O_CLOEXEC*/);
    if (fd >= 0) {
      while (pos < sizeof(xstate)) {
        size_t len = sizeof(xstate)-pos;
        ssize_t rd = read(fd, ((xprng_ubyte *)xstate)+pos, len);
        if (rd < 0) {
          if (errno != EINTR) break;
        } else {
          pos += (size_t)rd;
        }
      }
      close(fd);
      g_xrng_strong_seed = (pos == sizeof(xstate));
    }
  }
  /* mix some other random sources, just in case */
  if (mixother) {
    xprng_uint smxseed0 = 0;
    xprng_uint smxseed1 = (xprng_uint)getpid();
    #if defined(__linux__)
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
      smxseed0 = ts.tv_sec^ts.tv_nsec;
    } else {
      struct timeval tp;
      struct timezone tzp;
      gettimeofday(&tp, &tzp);
      smxseed0 = tp.tv_sec^tp.tv_usec;
    }
    #else
    struct timeval tp;
    struct timezone tzp;
    gettimeofday(&tp, &tzp);
    smxseed0 = tp.tv_sec^tp.tv_usec;
    #endif
    isaacp_state rngtmp;
    xprng_uint64 smx;
    splitmix64_seedU64(&smx, smxseed0, smxseed1);
    for (unsigned n = 0; n < 256u; ++n) rngtmp.state[n] = splitmix64_next(&smx);
    rngtmp.a = splitmix64_next(&smx);
    rngtmp.b = splitmix64_next(&smx);
    rngtmp.c = splitmix64_next(&smx);
    isaacp_mix(&rngtmp);
    isaacp_mix(&rngtmp);
    isaacp_random(&rngtmp, rng->state, sizeof(rng->state));
    isaacp_random(&rngtmp, rng->abc, sizeof(rng->abc));
  }
  #endif
  /* xor ISAAC+ state with random bytes for from various sources */
  for (unsigned f = 0u; f < 256u; ++f) rng->state[f] ^= xstate[f];
  for (unsigned f = 0u; f < 3u; ++f) rng->abc[f] ^= xstate[256u+f];
  isaacp_mix(rng);
  isaacp_mix(rng);
}


#ifdef WIN32
static volatile LONG g_xrng_lock = 0;
#elif defined(__linux__) || defined(__SWITCH__)
static volatile unsigned g_xrng_lock = 0;
#else
# error "unsupported OS!"
#endif

static unsigned g_xrng_initialized = 0;
static isaacp_state g_xrng;


//==========================================================================
//
//  prng_is_strong_seed
//
//==========================================================================
int prng_is_strong_seed () {
  int res;

  // spinlock
  #ifdef WIN32
  while (InterlockedCompareExchange(&g_xrng_lock, 1, 0)) {}
  #else
  while (__atomic_exchange_n(&g_xrng_lock, 1, __ATOMIC_SEQ_CST)) {}
  #endif

  if (!g_xrng_initialized) {
    randombytes_init(&g_xrng);
    g_xrng_initialized = 1;
  }

  res = g_xrng_strong_seed;

  // release lock
  #ifdef WIN32
  // `g_xrng_lock` is definitely `1` here
  (void)InterlockedCompareExchange(&g_xrng_lock, 0, 1);
  #else
  __atomic_store_n(&g_xrng_lock, 0, __ATOMIC_SEQ_CST);
  #endif

  return res;
}


//==========================================================================
//
//  prng_randombytes
//
//==========================================================================
void prng_randombytes (void *p, unsigned len) {
  if (!len || !p) return;

  // spinlock
  #ifdef WIN32
  while (InterlockedCompareExchange(&g_xrng_lock, 1, 0)) {}
  #else
  while (__atomic_exchange_n(&g_xrng_lock, 1, __ATOMIC_SEQ_CST)) {}
  #endif

  if (!g_xrng_initialized) {
    randombytes_init(&g_xrng);
    g_xrng_initialized = 1;
  }

  isaacp_random(&g_xrng, p, len);

  // release lock
  #ifdef WIN32
  // `g_xrng_lock` is definitely `1` here
  (void)InterlockedCompareExchange(&g_xrng_lock, 0, 1);
  #else
  __atomic_store_n(&g_xrng_lock, 0, __ATOMIC_SEQ_CST);
  #endif
}


#ifdef __cplusplus
}
#endif
