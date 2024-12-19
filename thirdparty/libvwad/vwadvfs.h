/* coded by Ketmar // Invisible Vector (psyc://ketmar.no-ip.org/~Ketmar)
 * Understanding is not required. Only obedience.
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar.
 * See http://www.wtfpl.net/ for more details.
 */
/*
  VWADs are chunked archives with zlib-comparable compression ratio.
  usually VWAD size on max copression level is little lesser than
  the corresponding ZIP file. the most useful feature of VWAD is the
  ability to read files non-sequentially without unpacking the whole
  file first. i.e. seeking in archive files is quite cheap, and can
  be used freely.

  any archive can be signed with Ed25519 digital signature. please,
  note that you have to provide good cryptographically strong PRNG
  by yourself (actually, you should use it to generate a private key,
  the library itself doesn't have any PRNG generator).

  note that while archive chunks are "encrypted", this "encryption"
  is not cryptographically strong, and used mostly to hide non-compressed
  data from simple viewing tools.

  archived files can be annotated with arbitrary "group name". this can
  be used in various content creation tools. group tags are not used
  by the library itself. group names are case-insensitive.

  also, archived files can have a 64-bit last modification timestamp
  (seconds since Unix Epoch). you can use zero as timestamp if you
  don't care.

  file names can containprintable unicode chars from the first plane
  (see `vwad_is_uni_printable()`), and names are case-insensitive.
  all names should be utf-8 encoded. also note that whice case folding
  is unicode-aware, not all codepoints are implemented. latin-1 and
  basic cyrillic should be safe, though.
  path delimiter is "/" (and only "/").
  file names may not contain chars [1..31], and char 127.

  archive can be tagged with author name, short description, and a comment.
  author name and description can contain unicode chars, but cannot have
  control chars (with codes < 31).
  comments can be multiline (only '\x0a' is allowed as a newline char).
  tabs are allowed too.

  currently, only limited subset of the first unicode plane is supported.
  deal with it.

  WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
  the reader is not thread-safe. i.e. you cannot use opened handle to
  read different files in different threads without locking. but different
  handles for different threads are allowed.
  WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
*/
#ifndef VWADVFS_HEADER
#define VWADVFS_HEADER

#if defined(__cplusplus)
extern "C" {
#endif

// this should be 8 bit (i hope so)
typedef unsigned char vwad_ubyte;
// this should be 16 bit (i hope so)
typedef unsigned short vwad_ushort;
// this should be 32 bit (i hope so)
typedef unsigned int vwad_uint;
// this should be 64 bit (i hope so)
typedef unsigned long long vwad_uint64;

// size checks
typedef char vwad_temp_typedef_check_char[(sizeof(char) == 1) ? 1 : -1];
typedef char vwad_temp_typedef_check_char[(sizeof(int) == 4) ? 1 : -1];
typedef char vwad_temp_typedef_check_ubyte[(sizeof(vwad_ubyte) == 1) ? 1 : -1];
typedef char vwad_temp_typedef_check_ushort[(sizeof(vwad_ushort) == 2) ? 1 : -1];
typedef char vwad_temp_typedef_check_uint[(sizeof(vwad_uint) == 4) ? 1 : -1];
typedef char vwad_temp_typedef_check_uint64[(sizeof(vwad_uint64) == 8) ? 1 : -1];

// this should be 64 bit (i hope so)
typedef vwad_uint64 vwad_ftime;


// for self-documentation purposes
typedef int vwad_bool;

// file index; <0 means "no file"
typedef int vwad_fidx;

// file descriptor; negative means "invalid", zero or positive is ok
typedef int vwad_fd;

// for self-documentation purposes
// 0 is success, negative value is error
typedef int vwad_result;

// for `vwad_result`
#define VWAD_OK  (0)

// opaque handle
typedef struct vwad_handle_t vwad_handle;

// i/o stream
typedef struct vwad_iostream_t vwad_iostream;
struct vwad_iostream_t {
  // return non-zero on failure; failure can be delayed
  // will never be called with negative `pos`
  vwad_result (*seek) (vwad_iostream *strm, int pos);
  // read *exactly* bufsize bytes; return 0 on success, negative on failure
  // will never be called with zero or negative `bufsize`
  vwad_result (*read) (vwad_iostream *strm, void *buf, int bufsize);
  // user data
  void *udata;
};

// memory allocation
typedef struct vwad_memman_t vwad_memman;
struct vwad_memman_t {
  // will never be called with zero `size`
  // return NULL on OOM
  void *(*alloc) (vwad_memman *mman, vwad_uint size);
  // will never be called with NULL `p`
  void (*free) (vwad_memman *mman, void *p);
  // user data
  void *udata;
};

// Ed25519 public key, 32 bytes
typedef vwad_ubyte vwad_public_key[32];

// 40 bytes of key, 5 bytes of crc32, plus zero
typedef char vwad_z85_key[46];


enum {
  VWAD_LOG_NOTE,
  VWAD_LOG_WARNING,
  VWAD_LOG_ERROR,
  VWAD_LOG_DEBUG,
};

// logging; can be NULL
// `type` is the above enum
extern void (*vwad_logf) (int type, const char *fmt, ...);

// assertion; can be NULL, then the lib will simply traps
extern void (*vwad_assertion_failed) (const char *fmt, ...);


// ////////////////////////////////////////////////////////////////////////// //
// can be used to debug the library. you don't need this.
extern void (*vwad_debug_open_file) (vwad_handle *wad, vwad_fidx fidx, vwad_fd fd);
extern void (*vwad_debug_close_file) (vwad_handle *wad, vwad_fidx fidx, vwad_fd fd);
extern void (*vwad_debug_read_chunk) (vwad_handle *wad, int bidx, vwad_fidx fidx, vwad_fd fd, int chunkidx);
extern void (*vwad_debug_flush_chunk) (vwad_handle *wad, int bidx, vwad_fidx fidx, vwad_fd fd, int chunkidx);


// ////////////////////////////////////////////////////////////////////////// //
// you can encode keys in printable ASCII chars, and decode them back.
// encoded key contains a checksum, so mistyped keys could be detected.
void vwad_z85_encode_key (const vwad_public_key inkey, vwad_z85_key enkey);
vwad_result vwad_z85_decode_key (const vwad_z85_key enkey, vwad_public_key outkey);


// ////////////////////////////////////////////////////////////////////////// //
// flags for `vwad_open_archive()`, to be bitwise ORed
#define VWAD_OPEN_DEFAULT          (0u)
// if you are not interested in archive comment, use this flag.
// it will save some memory.
#define VWAD_OPEN_NO_MAIN_COMMENT  (0x2000u)
// this flag can be used to omit digital signature checks.
// signature checks are fast, so you should use this flag
// only if you have a partially corrupted file which you
// want to read anyway.
// note that ed25519 public key will be read and accessible
// even with disabled checks, but you should not use it to
// determine the authorship, for example.
#define VWAD_OPEN_NO_SIGN_CHECK    (0x4000u)
// WARNING! DO NOT USE THIS!
// this can be used to recover files with partially corrupted data,
// but DON'T pass it "for speed", CRC checks are fast, and you
// should not omit them.
#define VWAD_OPEN_NO_CRC_CHECKS    (0x8000u)

// on success, will copy stream pointer and memman pointer
// (i.e. they should be alive until `vwad_close_archive()`).
// if `mman` is `NULL`, use libc memory manager.
// default cache settings is 256KB (4 buffers).
vwad_handle *vwad_open_archive (vwad_iostream *strm, vwad_uint flags, vwad_memman *mman);

// will free handle memory, and clean handle pointer.
void vwad_close_archive (vwad_handle **wadp);

// setup global chunk cache for the archive, in chunks. each chunk is ~64KB.
// by default, there is 4-chunk cache.
// this can speedup Doom WAD reading, for example, when the caller is
// often open/close the same files.
// this is an advice, not a demand.
// <=0 means "disable global cache" (each opened file will use one local buffer).
// (this mode is not heavily tested, and may be removed in the future.)
// notice: "global" here means that all files opened in this archive
//         share the cache. different archives has different caches, though.
void vwad_set_archive_cache (vwad_handle *wad, int bufferCount);


// this includes the trailing 0 byte. i.e. allocating a buffer
// of this size should be enough to get any comment untruncated.
#define VWAD_MAX_COMMENT_SIZE  (65536)

// get size of the comment, without the trailing zero byte.
// returns 0 on error, or on empty comment.
vwad_uint vwad_get_archive_comment_size (vwad_handle *wad);

// can return empty string if there is no comment,
// or `VWAD_OPEN_NO_MAIN_COMMENT` passed,
// or if `vwad_free_archive_comment()` was called.
// WARNING! `dest` must be at least `vwad_get_archive_comment_size()` + 1 bytes!
//          this is because the comment is terminated with zero byte.
// will truncate comment if destination buffer is not big enough (with zero byte).
// `destsize` if the full buffer size in bytes. returned data is 0-terminated, i.e.
// it is a valid C string (except when `destsize` is 0).
void vwad_get_archive_comment (vwad_handle *wad, char *dest, vwad_uint destsize);

// forget main archive comment and free its memory.
// you will not be able to get archive comment after calling this.
void vwad_free_archive_comment (vwad_handle *wad);

// never returns NULL
const char *vwad_get_archive_author (vwad_handle *wad);
// never returns NULL
const char *vwad_get_archive_title (vwad_handle *wad);

// check if the given archive has any files opened with `vwad_fopen()`.
vwad_bool vwad_has_opened_files (vwad_handle *wad);

// check if opened wad is authenticated. if the library was
// built without ed25519 support, or the wad was opened with
// "omit signature check" flag, there could be a public key,
// but the wad will not be authenticated with that key.
vwad_bool vwad_is_authenticated (vwad_handle *wad);
// check if the wad has a ed25519 public key. it doesn't matter
// if the wad was authenticated or not, the key is always avaliable
// (if the wad contains it, of course).
// i.e. you can turn off auth check, and use this function to
// detect if there is a digital signature without actually checking it.
vwad_bool vwad_has_pubkey (vwad_handle *wad);
// on failure, `pubkey` content is zeroed.
// if there is no digital signature, there is no pubkey.
// it doesn't matter if the archive was authenticated or not.
vwad_result vwad_get_pubkey (vwad_handle *wad, vwad_public_key pubkey);

// returns maximum fidx in this wad.
// avaliable files are [0..res).
// on error, returns 0.
vwad_fidx vwad_get_archive_file_count (vwad_handle *wad);

// returns NULL on invalid `fidx`; group name can be empty string.
// WARNING! while currenly VWAD creation tools will use the same
//          name (pointer) for the same group name, this is not
//          enforced, and you should not rely on it. always compare
//          string contents instead.
const char *vwad_get_file_group_name (vwad_handle *wad, vwad_fidx fidx);

// first index is 0, last is `vwad_get_file_count() - 1`.
// returns NULL on invalid `fidx`
const char *vwad_get_file_name (vwad_handle *wad, vwad_fidx fidx);
// returns negative number on error.
int vwad_get_file_size (vwad_handle *wad, vwad_fidx fidx);
// returns 0 if there is an error, or the time is not set
// returned time is in UTC, seconds since Epoch (or 0 if unknown)
vwad_ftime vwad_get_ftime (vwad_handle *wad, vwad_fidx fidx);
// get crc32 for the whole file
vwad_uint vwad_get_fcrc32 (vwad_handle *wad, vwad_fidx fidx);

// normalize file name: remove "/./", resolve "/../", remove
// double slashes, etc. it should be safe to pass the result
// to `vwad_find_file()`. if the result is longer than 255
// chars, returns error, and `res` contents are undefined.
// also, converts backslashes to proper forward slashes.
// please note that last slash will NOT be removed, but
// such file name is invalid.
// starting "/" will be retained, though.
vwad_result vwad_normalize_file_name (const char *fname, char res[256]);

// find file by name. internally, the library is using a hash
// table, so this is quite fast.
// searching is case-insensitive for ASCII chars.
// use slashes ("/") to separate directories. do not start
// `name` with a slash, use strictly one slash to separate
// path parts. "." and ".." pseudodirs are not supported.
// returns negative number if no file found.
vwad_fidx vwad_find_file (vwad_handle *wad, const char *name);

// open file for reading using its index
// (obtained from `vwad_find_file()`, for example).
// return file handle or negative value on error.
// (note that 0 is a valid file handle!)
// note that maximum number of simultaneously opened files is 128.
vwad_fd vwad_open_fidx (vwad_handle *wad, vwad_fidx fidx);

// open file by name.
vwad_fd vwad_open_file (vwad_handle *wad, const char *name);

// close opened file. calling with invalid fd is ok.
// calling with closed fd may close the file opened elsewhere
// (because fds are reused).
void vwad_fclose (vwad_handle *wad, vwad_fd fd);

// return negative number if fd is invalid.
vwad_fidx vwad_fdfidx (vwad_handle *wad, vwad_fd fd);

// change current reading position.
vwad_result vwad_seek (vwad_handle *wad, vwad_fd fd, int pos);

// return current reading position, or negative number on error.
int vwad_tell (vwad_handle *wad, vwad_fd fd);

// return number of read bytes, or negative on error.
// trying to read 0 bytes will return 0 (after performing validity checks).
// still, trying to read 0 bytes may succeed even if some arguments
// are invalid, so don't use this as cheap check for "is this fd valid".
// if you want to know if fd is valid, use `vwad_fdfidx()`, it is faster anyway.
int vwad_read (vwad_handle *wad, vwad_fd fd, void *buf, int len);


// ////////////////////////////////////////////////////////////////////////// //
// reading raw chunks can be used in archive creator to copy data
// without recompressing it.

#define VWAD_MAX_RAW_CHUNK_SIZE  (65536 + 4)

// this is used in creator, to copy raw chunks
// returns -1 on error
int vwad_get_file_chunk_count (vwad_handle *wad, vwad_fidx fidx);
// this is used in creator, to copy raw chunks
// 4 bytes of CRC32 are INCLUDED in `pksz` (but not in `upksz`)!
// `pksz` is never zero; to check if the chunk is packed, use `packed`
vwad_result vwad_get_raw_file_chunk_info (vwad_handle *wad, vwad_fidx fidx, int chunkidx,
                                          int *pksz, int *upksz, vwad_bool *packed);
// this is used in creator, to copy raw chunks
// reads chunk WITH CRC32!
vwad_result vwad_read_raw_file_chunk (vwad_handle *wad, vwad_fidx fidx, int chunkidx, void *buf);


// ////////////////////////////////////////////////////////////////////////// //
// your bog stadard CRC32 calculator. uses the same poly as zlib.

vwad_uint vwad_crc32_init (void);
vwad_uint vwad_crc32_part (vwad_uint crc32, const void *src, vwad_uint len);
vwad_uint vwad_crc32_final (vwad_uint crc32);


// ////////////////////////////////////////////////////////////////////////// //
// utf-8 case insensitive string equality check.
vwad_bool vwad_str_equ_ci (const char *s0, const char *s1);

// case-insensitive utf-8 wildcard matching
// returns:
//   -1: malformed pattern
//    0: equal
//    1: not equal
vwad_result vwad_wildmatch (const char *pat, vwad_uint plen, const char *str, vwad_uint slen);

// this matches individual path parts.
// exception: if `pat` contains no slashes, match only the name.
// if `pat` is like "/mask", match only root dir files.
// masks like "/*/*/" will match anything 2 subdirs or deeper.
// masks like "/*/*" will match exactly the one subdir.
vwad_result vwad_wildmatch_path (const char *pat, vwad_uint plen, const char *str, vwad_uint slen);


// ////////////////////////////////////////////////////////////////////////// //
// WARNING! the following API works only with unicode plane 1 -- [0..65535]!

// "invalid char" unicode code
#define VWAD_REPLACEMENT_CHAR  (0x0FFFDu)

vwad_uint vwad_utf_char_len (const void *str);
// advances `strp` at least by one byte.
// returns `VWAD_REPLACEMENT_CHAR` on invalid char.
// invalid chars are thouse with invalid utf-8, 0x7f, and unprintable.
// printable chars are determined with `vwad_is_uni_printable()`.
vwad_ushort vwad_utf_decode (const char **strp);

// control chars are not printable, except tab (0x09), and newline (0x0a).
// del (0x7f) is not printable too.
vwad_bool vwad_is_uni_printable (vwad_ushort ch);
// supports only limited set of known chars (mostly latin and slavic).
vwad_ushort vwad_uni_tolower (vwad_ushort ch);


#if defined(__cplusplus)
}
#endif
#endif
