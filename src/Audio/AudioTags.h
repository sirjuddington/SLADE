#ifndef __AUDIOTAGS_H__
#define	__AUDIOTAGS_H__

#include "Main.h"

namespace Audio
{
	string		getID3Tag(MemChunk& mc);
	string		getOggComments(MemChunk& mc);
	string		getFlacComments(MemChunk& mc);
	string		getITComments(MemChunk& mc);
	string		getModComments(MemChunk& mc);
	string		getS3MComments(MemChunk& mc);
	string		getXMComments(MemChunk& mc);
	string		getWavInfo(MemChunk& mc);
	string		getVocInfo(MemChunk& mc);
	string		getSunInfo(MemChunk& mc);
	string		getRmidInfo(MemChunk& mc);
	string		getAiffInfo(MemChunk& mc);
}

#endif // __AUDIOTAGS_H__