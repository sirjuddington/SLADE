
#ifndef __MAIN_H__
#define __MAIN_H__

#include "Common.h"
#include "CommonFwd.h"

#if defined _MSC_VER && _MSC_VER < 1900
#define _CRT_SECURE_NO_WARNINGS 1
typedef __int8           int8_t;
typedef unsigned __int8  uint8_t;
typedef __int16          int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32          int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64          int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <cstdint>
#endif

// String stuff
#define S_CMP(s1, s2) s1.Cmp(s2) == 0
#define S_CMPNOCASE(s1, s2) s1.CmpNoCase(s2) == 0
#define WX_FMT(...) wxString::FromUTF8(fmt::format(__VA_ARGS__))

// Usings for common std types, to avoid std:: spam
using std::optional;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;
using std::weak_ptr;

// A macro to check if a value exists in a vector
#define VECTOR_EXISTS(vec, val) find((vec).begin(), (vec).end(), val) != (vec).end()

// A macro to add a value to a vector if the value doesn't already exist in the vector
#define VECTOR_ADD_UNIQUE(vec, val) \
	if (!(VECTOR_EXISTS(vec, val))) \
	(vec).push_back(val)

// A macro to remove an item with a given value from a vector
#define VECTOR_REMOVE(vec, val) vec.erase(find((vec).begin(), (vec).end(), val))

// A macro to remove an item at the given index from a vector
#define VECTOR_REMOVE_AT(vec, index) vec.erase((vec).begin() + (index))

#undef Bool

// Namespace to hold 'global' variables
namespace slade::global
{
extern string error;
extern string sc_rev;
extern bool   debug;
extern int    win_version_major;
extern int    win_version_minor;
}; // namespace slade::global

// Rust-style numeric type aliases
namespace slade
{
using i8  = int8_t;
using u8  = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
} // namespace slade

// Global internal includes
// clang-format off
#include "General/CVar.h"
#include "Geometry/Vectors.h"
#include "General/Log.h"
#include "Utility/MemChunk.h"
#include "OpenGL/GLHeaders.h"
#include "Utility/ColRGBA.h"
// clang-format on

#endif // __MAIN_H__
