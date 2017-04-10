
#ifdef __cplusplus

#ifdef _MSC_VER
// Avoid many '#pragma once in main file' warnings from GCC or Clang
// when this file is used as the prefix header
#pragma once
#endif // _MSC_VER

#ifndef __MAIN_H__
#define __MAIN_H__

#include "common.h"
#include "common2.h"

#if defined _MSC_VER && _MSC_VER < 1900
#define _CRT_SECURE_NO_WARNINGS 1
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

// String stuff
typedef wxString string;
#define S_FMT wxString::Format
#define S_CMP(s1, s2) s1.Cmp(s2) == 0
#define S_CMPNOCASE(s1, s2) s1.CmpNoCase(s2) == 0

// Macro to convert a wxString to a c string
#define CHR(s) (static_cast<const char*>((s).ToAscii()))
#define UTF8(s) (static_cast<const char*>((s).c_str()))

// Vectors
using std::vector;

// A macro to check if a value exists in a vector
#define VECTOR_EXISTS(vec, val) find(vec.begin(), vec.end(), val) != vec.end()

// A macro to add a value to a vector if the value doesn't already exist in the vector
#define VECTOR_ADD_UNIQUE(vec, val) if (!(VECTOR_EXISTS(vec, val))) vec.push_back(val)

// A macro to remove an item with a given value from a vector
#define VECTOR_REMOVE(vec, val) vec.erase(find(vec.begin(), vec.end(), val))

// A macro to remove an item at the given index from a vector
#define VECTOR_REMOVE_AT(vec, index) vec.erase(vec.begin() + index)

// Template function to find something in an associative map.
// M::mapped_type should be default constructible, or just provide
// a value for the third argument (the default value if not found).
// This really only works for value types right now, like maps to pointers.
template<typename M>
#if defined(_MSC_VER) && (_MSC_VER < 1500)
// MSVC++ 2005 will give error C2899 if attempting to compile "typename M::mapped_type()".
typename M::mapped_type findInMap(M& m, const typename M::key_type& k, typename M::mapped_type def = M::mapped_type())
{
#else
// On the other hand, other compilers will fail if this typename isn't there.
typename M::mapped_type findInMap(M& m, const typename M::key_type& k, typename M::mapped_type def = typename M::mapped_type())
{
#endif
	typename M::iterator i = m.find(k);
	if(i == m.end())
	{
		return const_cast<typename M::mapped_type&>(def);
	}
	else
	{
		return i->second;
	}
}

#undef Bool

// Global internal includes
#include "General/CVar.h"
#include "General/Log.h"
#include "Utility/MemChunk.h"
#include "Utility/Structs.h"


// Namespace to hold 'global' variables
namespace Global
{
	extern string error;
	extern string version;
	extern string sc_rev;
	extern bool debug;
	extern double ppi_scale;
	extern int win_version_major;
	extern int win_version_minor;
};


// Random useful defines
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

// Define map types
enum MapTypes
{
	MAP_DOOM = 0,
	MAP_HEXEN,
	MAP_DOOM64,
	MAP_UDMF,
	MAP_UNKNOWN,	// Needed for maps in zip archives
};

const string MAP_TYPE_NAMES[] = {
	"Doom",
	"Hexen",
	"Doom64",
	"UDMF",
	"Unknown",
};

// Debug helper type
// Note: NDEBUG is a standard C macro indicating that assert()s should be
// disabled, but it's also the only macro cmake defines differently between
// debug and release builds, so we co-opt it just for this.
#ifndef NDEBUG
#include <typeinfo>
class Debuggable
{
	string repr;

public:
	Debuggable(string v) { repr = v; }
	Debuggable(const char* v) { repr = v; }
	Debuggable(bool v) { repr = v ? "true" : "false"; }
	Debuggable(int v) { repr = S_FMT("%d", v); }
	Debuggable(unsigned int v) { repr = S_FMT("%u", v); }
	Debuggable(long v) { repr = S_FMT("%ld", v); }
	Debuggable(unsigned long v) { repr = S_FMT("%lu", v); }
	Debuggable(double v) { repr = S_FMT("%g", v); }

	Debuggable(fpoint2_t v) { repr = S_FMT("(%0.6f, %0.6f)", v.x, v.y); }
	Debuggable(fpoint3_t v) { repr = S_FMT("(%0.6f, %0.6f, %0.6f)", v.x, v.y, v.z); }
	Debuggable(frect_t v) { repr = S_FMT("(%0.6f, %0.6f to %0.6f, %0.6f)", v.x1(), v.y1(), v.x2(), v.y2()); }

	template<typename T>
	Debuggable(T* v) { repr = Debuggable(*v).repr; }

	template<typename T>
	Debuggable(vector<T> v) {
		repr << "{";
		for (unsigned int a = 0; a < v.size(); a++)
		{
			repr << Debuggable(v[a]).get();
			if (a < v.size() - 1)
				repr << ", ";
		}
		repr << "}";
	}

	string get() { return this->repr; }
};

inline void LOG_DEBUG(
	Debuggable a1 = "",
	Debuggable a2 = "",
	Debuggable a3 = "",
	Debuggable a4 = "",
	Debuggable a5 = "",
	Debuggable a6 = "",
	Debuggable a7 = "",
	Debuggable a8 = "",
	Debuggable a9 = "",
	Debuggable a10 = "",
	Debuggable a11 = "",
	Debuggable a12 = ""
)
{
	string message;
	message << a1.get() << " ";
	message << a2.get() << " ";
	message << a3.get() << " ";
	message << a4.get() << " ";
	message << a5.get() << " ";
	message << a6.get() << " ";
	message << a7.get() << " ";
	message << a8.get() << " ";
	message << a9.get() << " ";
	message << a10.get() << " ";
	message << a11.get() << " ";
	message << a12.get();
	message.Trim();
	Log::debug(S_FMT("%s", message));
}

#define LOG_DEBUG_VAR(name) LOG_DEBUG(#name ": ", name)
#else  // not NDEBUG
struct Debuggable {
	template<typename T>
	Debuggable(T _unused) { }
};
#define LOG_DEBUG(...)
#define LOG_DEBUG_VAR(name)
#endif  // DEBUG

#endif // __MAIN_H__

#endif /* __cplusplus */
