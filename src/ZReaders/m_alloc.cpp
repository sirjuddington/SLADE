/*
** m_alloc.cpp
** Wrappers for the malloc family of functions that count used bytes.
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#if defined(__FreeBSD__)
#include <stdlib.h>
#include <malloc_np.h>
#elif defined(__APPLE__)
#include <stdlib.h>
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include "Main.h"
#define I_FatalError(...) slade::log::error(wxString::Format(__VA_ARGS__))

namespace GC
{
	size_t AllocBytes;
}

#ifndef _MSC_VER
#define _NORMAL_BLOCK			0
#define _malloc_dbg(s,b,f,l)	malloc(s)
#define _realloc_dbg(p,s,b,f,l)	realloc(p,s)
#endif
#if defined(__APPLE__)
#define _msize(p)				malloc_size(p)
#elif defined(__FreeBSD__) || defined(__linux__)
#define _msize(p)				malloc_usable_size(p)	// from glibc/FreeBSD
#elif !defined(_WIN32)
// For all other OS not handled above
#define M_ALLOC_GENERIC  1
#define _msize(p)                              (*((size_t*)(p)-1))
#endif

#ifndef _DEBUG
#if !M_ALLOC_GENERIC
void *M_Malloc(size_t size)
{
	void *block = malloc(size);

	if (block == NULL)
		I_FatalError("Could not malloc %zu bytes", size);

	GC::AllocBytes += _msize(block);
	return block;
}

void *M_Realloc(void *memblock, size_t size)
{
	if (memblock != NULL)
	{
		GC::AllocBytes -= _msize(memblock);
	}
	void *block = realloc(memblock, size);
	if (block == NULL)
	{
		I_FatalError("Could not realloc %zu bytes", size);
	}
	GC::AllocBytes += _msize(block);
	return block;
}
#else
void *M_Malloc(size_t size)
{
	void *block = malloc(size+sizeof(size_t));

	if (block == NULL)
		I_FatalError("Could not malloc %zu bytes", size);

	size_t *sizeStore = (size_t *) block;
	*sizeStore = size;
	block = sizeStore+1;

	GC::AllocBytes += _msize(block);
	return block;
}

void *M_Realloc(void *memblock, size_t size)
{
	if(memblock == NULL)
		return M_Malloc(size);

	if (memblock != NULL)
	{
		GC::AllocBytes -= _msize(memblock);
	}
	void *block = realloc(((size_t*) memblock)-1, size+sizeof(size_t));
	if (block == NULL)
	{
		I_FatalError("Could not realloc %zu bytes", size);
	}

	size_t *sizeStore = (size_t *) block;
	*sizeStore = size;
	block = sizeStore+1;

	GC::AllocBytes += _msize(block);
	return block;
}
#endif
#else
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

#if !M_ALLOC_GENERIC
void *M_Malloc_Dbg(size_t size, const char *file, int lineno)
{
	void *block = _malloc_dbg(size, _NORMAL_BLOCK, file, lineno);

	if (block == NULL)
		I_FatalError("Could not malloc %zu bytes", size);

	GC::AllocBytes += _msize(block);
	return block;
}

void *M_Realloc_Dbg(void *memblock, size_t size, const char *file, int lineno)
{
	if (memblock != NULL)
	{
		GC::AllocBytes -= _msize(memblock);
	}
	void *block = _realloc_dbg(memblock, size, _NORMAL_BLOCK, file, lineno);
	if (block == NULL)
	{
		I_FatalError("Could not realloc %zu bytes", size);
	}
	GC::AllocBytes += _msize(block);
	return block;
}
#else
void *M_Malloc_Dbg(size_t size, const char *file, int lineno)
{
	void *block = _malloc_dbg(size+sizeof(size_t), _NORMAL_BLOCK, file, lineno);

	if (block == NULL)
		I_FatalError("Could not malloc %zu bytes", size);

	size_t *sizeStore = (size_t *) block;
	*sizeStore = size;
	block = sizeStore+1;

	GC::AllocBytes += _msize(block);
	return block;
}

void *M_Realloc_Dbg(void *memblock, size_t size, const char *file, int lineno)
{
	if(memblock == NULL)
		return M_Malloc_Dbg(size, file, lineno);

	if (memblock != NULL)
	{
		GC::AllocBytes -= _msize(memblock);
	}
	void *block = _realloc_dbg(((size_t*) memblock)-1, size+sizeof(size_t), _NORMAL_BLOCK, file, lineno);

	if (block == NULL)
	{
		I_FatalError("Could not realloc %zu bytes", size);
	}

	size_t *sizeStore = (size_t *) block;
	*sizeStore = size;
	block = sizeStore+1;

	GC::AllocBytes += _msize(block);
	return block;
}
#endif
#endif

#if !M_ALLOC_GENERIC
void M_Free (void *block)
{
	if (block != NULL)
	{
		GC::AllocBytes -= _msize(block);
		free(block);
	}
}
#else
void M_Free (void *block)
{
	if(block != NULL)
	{
		GC::AllocBytes -= _msize(block);
		free(((size_t*) block)-1);
	}
}
#endif

