
#ifndef __CONVERSIONS_H__
#define __CONVERSIONS_H__

class ArchiveEntry;

namespace Conversions
{
	bool	wavToDoomSnd(MemChunk& in, MemChunk& out);
	bool	doomSndToWav(MemChunk& in, MemChunk& out);
	bool	wolfSndToWav(MemChunk& in, MemChunk& out);
	bool	jagSndToWav(MemChunk& in, MemChunk& out);
	bool	vocToWav(MemChunk& in, MemChunk& out);
	bool	bloodToWav(ArchiveEntry* in, MemChunk& out);
	bool	musToMidi(MemChunk& in, MemChunk& out);
	bool	gmidToMidi(MemChunk& in, MemChunk& out);
};

#endif//__CONVERSIONS_H__
