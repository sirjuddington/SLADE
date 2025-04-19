#pragma once

namespace slade::audio
{
string getID3Tag(MemChunk& mc);
string getOggComments(MemChunk& mc);
string getFlacComments(MemChunk& mc);
string getITComments(MemChunk& mc);
string getModComments(MemChunk& mc);
string getS3MComments(MemChunk& mc);
string getXMComments(MemChunk& mc);
string getWavInfo(MemChunk& mc);
string getVocInfo(MemChunk& mc);
string getSunInfo(MemChunk& mc);
string getRmidInfo(MemChunk& mc);
string getAiffInfo(MemChunk& mc);
size_t checkForTags(MemChunk& mc);
} // namespace slade::audio
