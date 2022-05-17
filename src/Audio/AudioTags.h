#pragma once

namespace slade::audio
{
wxString getID3Tag(const MemChunk& mc);
wxString getOggComments(const MemChunk& mc);
wxString getFlacComments(const MemChunk& mc);
wxString getITComments(const MemChunk& mc);
wxString getModComments(const MemChunk& mc);
wxString getS3MComments(const MemChunk& mc);
wxString getXMComments(const MemChunk& mc);
wxString getWavInfo(const MemChunk& mc);
wxString getVocInfo(const MemChunk& mc);
wxString getSunInfo(const MemChunk& mc);
wxString getRmidInfo(const MemChunk& mc);
wxString getAiffInfo(const MemChunk& mc);
size_t   checkForTags(const MemChunk& mc);
} // namespace slade::audio
