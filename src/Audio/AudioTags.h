#pragma once

namespace slade::audio
{
string getID3Tag(const MemChunk& mc);
string getOggComments(const MemChunk& mc);
string getFlacComments(const MemChunk& mc);
string getITComments(const MemChunk& mc);
string getModComments(const MemChunk& mc);
string getS3MComments(const MemChunk& mc);
string getXMComments(const MemChunk& mc);
string getWavInfo(const MemChunk& mc);
string getVocInfo(const MemChunk& mc);
string getSunInfo(const MemChunk& mc);
string getRmidInfo(const MemChunk& mc);
string getAiffInfo(const MemChunk& mc);
size_t checkForTags(const MemChunk& mc);
} // namespace slade::audio
