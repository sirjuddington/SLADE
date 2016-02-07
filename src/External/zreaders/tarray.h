/*
** tarray.h
** Templated, automatically resizing array
**
**---------------------------------------------------------------------------
** Copyright 1998-2007 Randy Heit
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

#ifndef __TARRAY_H__
#define __TARRAY_H__

#include <stdlib.h>
#include <assert.h>
#include <new>

#if !defined(_WIN32)
#include <inttypes.h>		// for intptr_t
#elif !defined(_MSC_VER)
#include <stdint.h>			// for mingw
#endif

#include "m_alloc.h"

//class FArchive;

// TArray -------------------------------------------------------------------

// T is the type stored in the array.
// TT is the type returned by operator().
template <class T, class TT=T>
class TArray
{
//	template<class U, class UU> friend FArchive &operator<< (FArchive &arc, TArray<U,UU> &self);

public:
	////////
	// This is a dummy constructor that does nothing. The purpose of this
	// is so you can create a global TArray in the data segment that gets
	// used by code before startup without worrying about the constructor
	// resetting it after it's already been used. You MUST NOT use it for
	// heap- or stack-allocated TArrays.
	enum ENoInit
	{
		NoInit
	};
	TArray (ENoInit dummy)
	{
	}
	////////
	TArray ()
	{
		Most = 0;
		Count = 0;
		Array = NULL;
	}
	TArray (int max)
	{
		Most = max;
		Count = 0;
		Array = (T *)M_Malloc (sizeof(T)*max);
	}
	TArray (const TArray<T> &other)
	{
		DoCopy (other);
	}
	TArray<T> &operator= (const TArray<T> &other)
	{
		if (&other != this)
		{
			if (Array != NULL)
			{
				if (Count > 0)
				{
					DoDelete (0, Count-1);
				}
				M_Free (Array);
			}
			DoCopy (other);
		}
		return *this;
	}
	~TArray ()
	{
		if (Array)
		{
			if (Count > 0)
			{
				DoDelete (0, Count-1);
			}
			M_Free (Array);
			Array = NULL;
			Count = 0;
			Most = 0;
		}
	}
	// Return a reference to an element
	T &operator[] (size_t index) const
	{
		return Array[index];
	}
	// Returns the value of an element
	TT operator() (size_t index) const
	{
		return Array[index];
	}
	// Returns a reference to the last element
	T &Last() const
	{
		return Array[Count-1];
	}

	unsigned int Push (const T &item)
	{
		Grow (1);
		::new((void*)&Array[Count]) T(item);
		return Count++;
	}
	bool Pop ()
	{
		if (Count > 0)
		{
			Array[--Count].~T();
			return true;
		}
		return false;
	}
	bool Pop (T &item)
	{
		if (Count > 0)
		{
			item = Array[--Count];
			Array[Count].~T();
			return true;
		}
		return false;
	}
	void Delete (unsigned int index)
	{
		if (index < Count)
		{
			Array[index].~T();
			if (index < --Count)
			{
				memmove (&Array[index], &Array[index+1], sizeof(T)*(Count - index));
			}
		}
	}

	void Delete (unsigned int index, int deletecount)
	{
		if (index + deletecount > Count)
		{
			deletecount = Count - index;
		}
		if (deletecount > 0)
		{
			for (int i = 0; i < deletecount; i++)
			{
				Array[index + i].~T();
			}
			Count -= deletecount;
			if (index < Count)
			{
				memmove (&Array[index], &Array[index+deletecount], sizeof(T)*(Count - index));
			}
		}
	}

	// Inserts an item into the array, shifting elements as needed
	void Insert (unsigned int index, const T &item)
	{
		if (index >= Count)
		{
			// Inserting somewhere past the end of the array, so we can
			// just add it without moving things.
			Resize (index + 1);
			::new ((void *)&Array[index]) T(item);
		}
		else
		{
			// Inserting somewhere in the middle of the array,
			// so make room for it
			Resize (Count + 1);

			// Now move items from the index and onward out of the way
			memmove (&Array[index+1], &Array[index], sizeof(T)*(Count - index - 1));

			// And put the new element in
			::new ((void *)&Array[index]) T(item);
		}
	}
	void ShrinkToFit ()
	{
		if (Most > Count)
		{
			Most = Count;
			if (Most == 0)
			{
				if (Array != NULL)
				{
					M_Free (Array);
					Array = NULL;
				}
			}
			else
			{
				DoResize ();
			}
		}
	}
	// Grow Array to be large enough to hold amount more entries without
	// further growing.
	void Grow (unsigned int amount)
	{
		if (Count + amount > Most)
		{
			const unsigned int choicea = Count + amount;
			const unsigned int choiceb = Most = (Most >= 16) ? Most + Most / 2 : 16;
			Most = (choicea > choiceb ? choicea : choiceb);
			DoResize ();
		}
	}
	// Resize Array so that it has exactly amount entries in use.
	void Resize (unsigned int amount)
	{
		if (Count < amount)
		{
			// Adding new entries
			Grow (amount - Count);
			for (unsigned int i = Count; i < amount; ++i)
			{
				::new((void *)&Array[i]) T;
			}
		}
		else if (Count != amount)
		{
			// Deleting old entries
			DoDelete (amount, Count - 1);
		}
		Count = amount;
	}
	// Reserves amount entries at the end of the array, but does nothing
	// with them.
	unsigned int Reserve (unsigned int amount)
	{
		Grow (amount);
		unsigned int place = Count;
		Count += amount;
		for (unsigned int i = place; i < Count; ++i)
		{
			::new((void *)&Array[i]) T;
		}
		return place;
	}
	unsigned int Size () const
	{
		return Count;
	}
	unsigned int Max () const
	{
		return Most;
	}
	void Clear ()
	{
		if (Count > 0)
		{
			DoDelete (0, Count-1);
			Count = 0;
		}
	}
private:
	T *Array;
	unsigned int Most;
	unsigned int Count;

	void DoCopy (const TArray<T> &other)
	{
		Most = Count = other.Count;
		if (Count != 0)
		{
			Array = (T *)M_Malloc (sizeof(T)*Most);
			for (unsigned int i = 0; i < Count; ++i)
			{
				::new(&Array[i]) T(other.Array[i]);
			}
		}
		else
		{
			Array = NULL;
		}
	}

	void DoResize ()
	{
		size_t allocsize = sizeof(T)*Most;
		Array = (T *)M_Realloc (Array, allocsize);
	}

	void DoDelete (unsigned int first, unsigned int last)
	{
		assert (last != ~0u);
		for (unsigned int i = first; i <= last; ++i)
		{
			Array[i].~T();
		}
	}
};

// TDeletingArray -----------------------------------------------------------
// An array that deletes its elements when it gets deleted.
template<class T, class TT=T>
class TDeletingArray : public TArray<T, TT>
{
public:
	~TDeletingArray<T, TT> ()
	{
		for (unsigned int i = 0; i < TArray<T,TT>::Size(); ++i)
		{
			if ((*this)[i] != NULL) 
				delete (*this)[i];
		}
	}
};

// TAutoGrowArray -----------------------------------------------------------
// An array with accessors that automatically grow the array as needed.
// It can still be used as a normal TArray if needed. ACS uses this for
// world and global arrays.

template <class T, class TT=T>
class TAutoGrowArray : public TArray<T, TT>
{
public:
	T GetVal (unsigned int index)
	{
		if (index >= this->Size())
		{
			return 0;
		}
		return (*this)[index];
	}
	void SetVal (unsigned int index, T val)
	{
		if ((int)index < 0) return;	// These always result in an out of memory condition.

		if (index >= this->Size())
		{
			this->Resize (index + 1);
		}
		(*this)[index] = val;
	}
};

// TMap ---------------------------------------------------------------------
// An associative array, similar in concept to the STL extension
// class hash_map. It is implemented using Lua's table algorithm:
/*
** Hash uses a mix of chained scatter table with Brent's variation.
** A main invariant of these tables is that, if an element is not
** in its main position (i.e. the `original' position that its hash gives
** to it), then the colliding element is in its own main position.
** Hence even when the load factor reaches 100%, performance remains good.
*/
/******************************************************************************
* Copyright (C) 1994-2006 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

typedef unsigned int hash_t;

template<class KT> struct THashTraits
{
	// Returns the hash value for a key.
	hash_t Hash(const KT key) { return (hash_t)(intptr_t)key; }

	// Compares two keys, returning zero if they are the same.
	int Compare(const KT left, const KT right) { return left != right; }
};

template<class VT> struct TValueTraits
{
	// Initializes a value for TMap. If a regular constructor isn't
	// good enough, you can override it.
	void Init(VT &value)
	{
		::new(&value) VT;
	}
};

template<class KT, class VT, class MapType> class TMapIterator;
template<class KT, class VT, class MapType> class TMapConstIterator;

template<class KT, class VT, class HashTraits=THashTraits<KT>, class ValueTraits=TValueTraits<VT> >
class TMap
{
	template<class KTa, class VTa, class MTa> friend class TMapIterator;
	template<class KTb, class VTb, class MTb> friend class TMapConstIterator;

public:
	typedef class TMap<KT, VT, HashTraits, ValueTraits> MyType;
	typedef class TMapIterator<KT, VT, MyType> Iterator;
	typedef class TMapConstIterator<KT, VT, MyType> ConstIterator;
	typedef struct { const KT Key; VT Value; } Pair;
	typedef const Pair ConstPair;

	TMap() { NumUsed = 0; SetNodeVector(1); }
	TMap(hash_t size) { NumUsed = 0; SetNodeVector(size); }
	~TMap() { ClearNodeVector(); }

	TMap(const TMap &o)
	{
		NumUsed = 0;
		SetNodeVector(o.CountUsed());
		CopyNodes(o.Nodes, o.Size);
	}

	TMap &operator= (const TMap &o)
	{
		NumUsed = 0;
		ClearNodeVector();
		SetNodeVector(o.CountUsed());
		CopyNodes(o.Nodes, o.Size);
		return *this;
	}

	//=======================================================================
	//
	// Clear
	//
	// Empties out the table and resizes it with room for count entries.
	//
	//=======================================================================

	void Clear(hash_t count=1)
	{
		ClearNodeVector();
		SetNodeVector(count);
	}

	//=======================================================================
	//
	// CountUsed
	//
	// Returns the number of entries in use in the table.
	//
	//=======================================================================

	hash_t CountUsed() const
	{
#ifdef _DEBUG
		hash_t used = 0;
		hash_t ct = Size;
		for (Node *n = Nodes; ct-- > 0; ++n)
		{
			if (!n->IsNil())
			{
				++used;
			}
		}
		assert (used == NumUsed);
#endif
		return NumUsed;
	}

	//=======================================================================
	//
	// operator[]
	//
	// Returns a reference to the value associated with a particular key,
	// creating the pair if the key isn't already in the table.
	//
	//=======================================================================

	VT &operator[] (const KT key)
	{
		return GetNode(key)->Pair.Value;
	}

	const VT &operator[] (const KT key) const
	{
		return GetNode(key)->Pair.Value;
	}

	//=======================================================================
	//
	// CheckKey
	//
	// Returns a pointer to the value associated with a particular key, or
	// NULL if the key isn't in the table.
	//
	//=======================================================================

	VT *CheckKey (const KT key)
	{
		Node *n = FindKey(key);
		return n != NULL ? &n->Pair.Value : NULL;
	}

	const VT *CheckKey (const KT key) const
	{
		const Node *n = FindKey(key);
		return n != NULL ? &n->Pair.Value : NULL;
	}

	//=======================================================================
	//
	// Insert
	//
	// Adds a key/value pair to the table if key isn't in the table, or
	// replaces the value for the existing pair if the key is in the table.
	//
	// This is functionally equivalent to (*this)[key] = value; but can be
	// slightly faster if the pair needs to be created because it doesn't run
	// the constructor on the value part twice.
	//
	//=======================================================================

	VT &Insert(const KT key, const VT &value)
	{
		Node *n = FindKey(key);
		if (n != NULL)
		{
			n->Pair.Value = value;
		}
		else
		{
			n = NewKey(key);
			::new(&n->Pair.Value) VT(value);
		}
		return n->Pair.Value;
	}

	//=======================================================================
	//
	// Remove
	//
	// Removes the key/value pair for a particular key if it is in the table.
	//
	//=======================================================================

	void Remove(const KT key)
	{
		DelKey(key);
	}

protected:
	struct IPair	// This must be the same as Pair above, but with a
	{				// non-const Key.
		KT Key;
		VT Value;
	};
	struct Node
	{
		Node *Next;
		IPair Pair;
		void SetNil()
		{
			Next = (Node *)1;
		}
		bool IsNil() const
		{
			return Next == (Node *)1;
		}
	};

	/* This is used instead of memcpy, because Node is likely to be small,
	 * such that the time spent calling a function would eclipse the time
	 * spent copying. */
	struct NodeSizedStruct { unsigned char Pads[sizeof(Node)]; };

	Node *Nodes;
	Node *LastFree;		/* any free position is before this position */
	hash_t Size;		/* must be a power of 2 */
	hash_t NumUsed;

	const Node *MainPosition(const KT k) const
	{
		HashTraits Traits;
		return &Nodes[Traits.Hash(k) & (Size - 1)];
	}

	Node *MainPosition(const KT k)
	{
		HashTraits Traits;
		return &Nodes[Traits.Hash(k) & (Size - 1)];
	}

	void SetNodeVector(hash_t size)
	{
		// Round size up to nearest power of 2
		for (Size = 1; Size < size; Size <<= 1)
		{ }
		Nodes = (Node *)M_Malloc(Size * sizeof(Node));
		LastFree = &Nodes[Size];	/* all positions are free */
		for (hash_t i = 0; i < Size; ++i)
		{
			Nodes[i].SetNil();
		}
	}

	void ClearNodeVector()
	{
		for (hash_t i = 0; i < Size; ++i)
		{
			if (!Nodes[i].IsNil())
			{
				Nodes[i].~Node();
			}
		}
		M_Free(Nodes);
		Nodes = NULL;
		Size = 0;
		LastFree = NULL;
		NumUsed = 0;
	}

	void Resize(hash_t nhsize)
	{
		hash_t i, oldhsize = Size;
		Node *nold = Nodes;
		/* create new hash part with appropriate size */
		SetNodeVector(nhsize);
		/* re-insert elements from hash part */
		NumUsed = 0;
		for (i = 0; i < oldhsize; ++i)
		{
			if (!nold[i].IsNil())
			{
				Node *n = NewKey(nold[i].Pair.Key);
				::new(&n->Pair.Value) VT(nold[i].Pair.Value);
				nold[i].~Node();
			}
		}
		M_Free(nold);
	}

	void Rehash()
	{
		Resize (Size << 1);
	}

	Node *GetFreePos()
	{
		while (LastFree-- > Nodes)
		{
			if (LastFree->IsNil())
			{
				return LastFree;
			}
		}
		return NULL;	/* could not find a free place */
	}

	/*
	** Inserts a new key into a hash table; first, check whether key's main 
	** position is free. If not, check whether colliding node is in its main 
	** position or not: if it is not, move colliding node to an empty place and 
	** put new key in its main position; otherwise (colliding node is in its main 
	** position), new key goes to an empty position. 
	**
	** The Value field is left unconstructed.
	*/
	Node *NewKey(const KT key)
	{
		Node *mp = MainPosition(key);
		if (!mp->IsNil())
		{
			Node *othern;
			Node *n = GetFreePos();		/* get a free place */
			if (n == NULL)				/* cannot find a free place? */
			{
				Rehash();				/* grow table */
				return NewKey(key);		/* re-insert key into grown table */
			}
			othern = MainPosition(mp->Pair.Key);
			if (othern != mp)			/* is colliding node out of its main position? */
			{	/* yes; move colliding node into free position */
				while (othern->Next != mp)	/* find previous */
				{
					othern = othern->Next;
				}
				othern->Next = n;		/* redo the chain with 'n' in place of 'mp' */
				CopyNode(n, mp); /* copy colliding node into free pos. (mp->Next also goes) */
				mp->Next = NULL;		/* now 'mp' is free */
			}
			else						/* colliding node is in its own main position */
			{							/* new node will go into free position */
				n->Next = mp->Next;		/* chain new position */
				mp->Next = n;
				mp = n;
			}
		}
		else
		{
			mp->Next = NULL;
		}
		++NumUsed;
		::new(&mp->Pair.Key) KT(key);
		return mp;
	}

	void DelKey(const KT key)
	{
		Node *mp = MainPosition(key), **mpp;
		HashTraits Traits;

		if (mp->IsNil())
		{
			/* the key is definitely not present, because there is nothing at its main position */
		}
		else if (!Traits.Compare(mp->Pair.Key, key)) /* the key is in its main position */
		{
			if (mp->Next != NULL)		/* move next node to its main position */
			{
				Node *n = mp->Next;
				mp->~Node();			/* deconstruct old node */
				CopyNode(mp, n);		/* copy next node */
				n->SetNil();			/* next node is now nil */
			}
			else
			{
				mp->~Node();
				mp->SetNil();			/* there is no chain, so main position is nil */
			}
			--NumUsed;
		}
		else	/* the key is either not present or not in its main position */
		{
			for (mpp = &mp->Next, mp = *mpp; mp != NULL && Traits.Compare(mp->Pair.Key, key); mpp = &mp->Next, mp = *mpp)
			{ }							/* look for the key */
			if (mp != NULL)				/* found it */
			{
				*mpp = mp->Next;		/* rechain so this node is skipped */
				mp->~Node();
				mp->SetNil();			/* because this node is now nil */
				--NumUsed;
			}
		}
	}

	Node *FindKey(const KT key)
	{
		HashTraits Traits;
		Node *n = MainPosition(key);
		while (n != NULL && !n->IsNil() && Traits.Compare(n->Pair.Key, key))
		{
			n = n->Next;
		}
		return n == NULL || n->IsNil() ? NULL : n;
	}

	const Node *FindKey(const KT key) const
	{
		HashTraits Traits;
		const Node *n = MainPosition(key);
		while (n != NULL && !n->IsNil() && Traits.Compare(n->Pair.Key, key))
		{
			n = n->Next;
		}
		return n == NULL || n->IsNil() ? NULL : n;
	}

	Node *GetNode(const KT key)
	{
		Node *n = FindKey(key);
		if (n != NULL)
		{
			return n;
		}
		n = NewKey(key);
		ValueTraits traits;
		traits.Init(n->Pair.Value);
		return n;
	}

	/* Perform a bit-wise copy of the node. Used when relocating a node in the table. */
	void CopyNode(Node *dst, const Node *src)
	{
		*(NodeSizedStruct *)dst = *(const NodeSizedStruct *)src;
	}

	/* Copy all nodes in the node vector to this table. */
	void CopyNodes(const Node *nodes, hash_t numnodes)
	{
		for (; numnodes-- > 0; ++nodes)
		{
			if (!nodes->IsNil())
			{
				Node *n = NewKey(nodes->Pair.Key);
				::new(&n->Pair.Value) VT(nodes->Pair.Value);
			}
		}
	}
};

// TMapIterator -------------------------------------------------------------
// A class to iterate over all the pairs in a TMap.

template<class KT, class VT, class MapType=TMap<KT,VT> >
class TMapIterator
{
public:
	TMapIterator(MapType &map)
		: Map(map), Position(0)
	{
	}

	//=======================================================================
	//
	// NextPair
	//
	// Returns false if there are no more entries in the table. Otherwise, it
	// returns true, and pair is filled with a pointer to the pair in the
	// table.
	//
	//=======================================================================

	bool NextPair(typename MapType::Pair *&pair)
	{
		if (Position >= Map.Size)
		{
			return false;
		}
		do
		{
			if (!Map.Nodes[Position].IsNil())
			{
				pair = reinterpret_cast<typename MapType::Pair *>(&Map.Nodes[Position].Pair);
				Position += 1;
				return true;
			}
		} while (++Position < Map.Size);
		return false;
	}

	//=======================================================================
	//
	// Reset
	//
	// Restarts the iteration so you can do it all over again.
	//
	//=======================================================================

	void Reset()
	{
		Position = 0;
	}

protected:
	MapType &Map;
	hash_t Position;
};

// TMapConstIterator --------------------------------------------------------
// Exactly the same as TMapIterator, but it works with a const TMap.

template<class KT, class VT, class MapType=TMap<KT,VT> >
class TMapConstIterator
{
public:
	TMapConstIterator(const MapType &map)
		: Map(map), Position(0)
	{
	}

	bool NextPair(typename MapType::ConstPair *&pair)
	{
		if (Position >= Map.Size)
		{
			return false;
		}
		do
		{
			if (!Map.Nodes[Position].IsNil())
			{
				pair = reinterpret_cast<typename MapType::Pair *>(&Map.Nodes[Position].Pair);
				Position += 1;
				return true;
			}
		} while (++Position < Map.Size);
		return false;
	}

protected:
	const MapType &Map;
	hash_t Position;
};

template <typename T, size_t N>
char ( &_ArraySizeHelper( T (&array)[N] ))[N];

#define countof( array ) (sizeof( _ArraySizeHelper( array ) ))

#endif //__TARRAY_H__
