
#pragma once

#ifdef _WINDOWS
#include <windows.h>
#endif

#ifdef _MSC_VER
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
#include <wx/string.h>
typedef wxString string;
#define S_FMT wxString::Format
#define S_CMP(s1, s2) s1.Cmp(s2) == 0
#define S_CMPNOCASE(s1, s2) s1.CmpNoCase(s2) == 0

// Macro to convert a wxString to a c string
#define CHR(s) (static_cast<const char*>((s).ToAscii()))


// Vectors
#include <vector>
using std::vector;
#include <algorithm>

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

// Logfile
#include <wx/log.h>
#define LOG_MESSAGE(level, ...) if (Global::log_verbosity >= level) wxLogMessage(__VA_ARGS__)

// File handling
#include <wx/file.h>

// Global internal includes
#include "MemChunk.h"
#include "CVar.h"
#include "Structs.h"

// Namespace to hold 'global' variables
namespace Global
{
	extern string error;
	extern string version;
	extern int log_verbosity;
	extern bool debug;
	extern double ppi_scale;
};


// Path related stuff
enum Directory { DIR_USER, DIR_DATA, DIR_APP, DIR_TEMP };
string appPath(string filename, int dir);


// Random useful defines
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define DPrintf wxLogMessage

// Define map types
enum MapTypes
{
	MAP_DOOM = 0,
	MAP_HEXEN,
	MAP_DOOM64,
	MAP_UDMF,
	MAP_UNKNOWN,	// Needed for maps in zip archives
};
