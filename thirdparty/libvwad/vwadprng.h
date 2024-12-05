/*
  coded by Ketmar Dark, public domain

  get (almost) crypto-secure random bytes.
  this will try to use the best PRNG available.

  what it really means is that it tries to use the best
  PRNG from the underlying OS to seed ISAAC+ PRNG, and
  then use ISAAC+ to generate random numbers. generated
  values are always cryptographically strong (due to
  ISAAC+ guarantees), but seed values may be weak.
*/
#ifndef VV_PRNG_RANDOMBYTES_HEADER
#define VV_PRNG_RANDOMBYTES_HEADER

#ifdef __cplusplus
extern "C" {
#endif


// return non-zero if used seed was strong.
// this should be thread-safe.
int prng_is_strong_seed ();

// be sure to cast `sizeof(...)` to `unsigned`!
// this should be thread-safe.
void prng_randombytes (void *p, unsigned len);


#ifdef __cplusplus
}
#endif
#endif
