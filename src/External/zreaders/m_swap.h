// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//		Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------


#ifndef __M_SWAP_H__
#define __M_SWAP_H__

#include <stdlib.h>

// Endianess handling.
// WAD files are stored little endian.

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>

inline short LittleShort(short x)
{
	return (short)CFSwapInt16LittleToHost((uint16_t)x);
}

inline unsigned short LittleShort(unsigned short x)
{
	return CFSwapInt16LittleToHost(x);
}

inline short LittleShort(int x)
{
	return CFSwapInt16LittleToHost((uint16_t)x);
}

inline int LittleLong(int x)
{
	return CFSwapInt32LittleToHost((uint32_t)x);
}

inline unsigned int LittleLong(unsigned int x)
{
	return CFSwapInt32LittleToHost(x);
}

inline short BigShort(short x)
{
	return (short)CFSwapInt16BigToHost((uint16_t)x);
}

inline unsigned short BigShort(unsigned short x)
{
	return CFSwapInt16BigToHost(x);
}

inline int BigLong(int x)
{
	return CFSwapInt32BigToHost((uint32_t)x);
}

inline unsigned int BigLong(unsigned int x)
{
	return CFSwapInt32BigToHost(x);
}

#else
#ifdef __BIG_ENDIAN__

// Swap 16bit, that is, MSB and LSB byte.
// No masking with 0xFF should be necessary. 
inline short LittleShort (short x)
{
	return (short)((((unsigned short)x)>>8) | (((unsigned short)x)<<8));
}

inline unsigned short LittleShort (unsigned short x)
{
	return (unsigned short)((x>>8) | (x<<8));
}

// Swapping 32bit.
inline unsigned int LittleLong (unsigned int x)
{
	return (unsigned int)(
		(x>>24)
		| ((x>>8) & 0xff00)
		| ((x<<8) & 0xff0000)
		| (x<<24));
}

inline int LittleLong (int x)
{
	return (int)(
		(((unsigned int)x)>>24)
		| ((((unsigned int)x)>>8) & 0xff00)
		| ((((unsigned int)x)<<8) & 0xff0000)
		| (((unsigned int)x)<<24));
}

#define BigShort(x)		(x)
#define BigLong(x)		(x)

#else

#define LittleShort(x)		(x)
#define LittleLong(x) 		(x)

#if defined(_MSC_VER)

inline short BigShort (short x)
{
	return (short)_byteswap_ushort((unsigned short)x);
}

inline unsigned short BigShort (unsigned short x)
{
	return _byteswap_ushort(x);
}

inline int BigLong (int x)
{
	return (int)_byteswap_ulong((unsigned long)x);
}

inline unsigned int BigLong (unsigned int x)
{
	return (unsigned int)_byteswap_ulong((unsigned long)x);
}
#pragma warning (default: 4035)

#else

inline short BigShort (short x)
{
	return (short)((((unsigned short)x)>>8) | (((unsigned short)x)<<8));
}

inline unsigned short BigShort (unsigned short x)
{
	return (unsigned short)((x>>8) | (x<<8));
}

inline unsigned int BigLong (unsigned int x)
{
	return (unsigned int)(
		(x>>24)
		| ((x>>8) & 0xff00)
		| ((x<<8) & 0xff0000)
		| (x<<24));
}

inline int BigLong (int x)
{
	return (int)(
		(((unsigned int)x)>>24)
		| ((((unsigned int)x)>>8) & 0xff00)
		| ((((unsigned int)x)<<8) & 0xff0000)
		| (((unsigned int)x)<<24));
}
#endif

#endif // __BIG_ENDIAN__
#endif // __APPLE__


// Data accessors, since some data is highly likely to be unaligned.
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) 
inline int GetShort(const unsigned char *foo)
{
	return *(const short *)foo;
}
inline int GetInt(const unsigned char *foo)
{
	return *(const int *)foo;
}
inline int GetBigInt(const unsigned char *foo)
{
	return BigLong(GetInt(foo));
}
#else
inline int GetShort(const unsigned char *foo)
{
	return short(foo[0] | (foo[1] << 8));
}
inline int GetInt(const unsigned char *foo)
{
	return int(foo[0] | (foo[1] << 8) | (foo[2] << 16) | (foo[3] << 24));
}
inline int GetBigInt(const unsigned char *foo)
{
	return int((foo[0] << 24) | (foo[1] << 16) | (foo[2] << 8) | foo[3]);
}
#endif
#ifdef __BIG_ENDIAN__
inline int GetNativeInt(const unsigned char *foo)
{
	return GetBigInt(foo);
}
#else
inline int GetNativeInt(const unsigned char *foo)
{
	return GetInt(foo);
}
#endif

#ifndef __BIG_ENDIAN__
#define MAKE_ID(a,b,c,d)	((uint32_t)((a)|((b)<<8)|((c)<<16)|((d)<<24)))
#else
#define MAKE_ID(a,b,c,d)	((uint32_t)((d)|((c)<<8)|((b)<<16)|((a)<<24)))
#endif

#endif // __M_SWAP_H__
