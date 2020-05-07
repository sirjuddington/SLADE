#pragma once

namespace slade::audio
{
wxString getID3Tag(MemChunk& mc);
wxString getOggComments(MemChunk& mc);
wxString getFlacComments(MemChunk& mc);
wxString getITComments(MemChunk& mc);
wxString getModComments(MemChunk& mc);
wxString getS3MComments(MemChunk& mc);
wxString getXMComments(MemChunk& mc);
wxString getWavInfo(MemChunk& mc);
wxString getVocInfo(MemChunk& mc);
wxString getSunInfo(MemChunk& mc);
wxString getRmidInfo(MemChunk& mc);
wxString getAiffInfo(MemChunk& mc);
size_t   checkForTags(MemChunk& mc);
} // namespace slade::audio
