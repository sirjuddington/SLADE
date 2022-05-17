#pragma once

namespace slade
{
class ArchiveEntry;

namespace conversion
{
	bool wavToDoomSnd(const MemChunk& in, MemChunk& out);
	bool spkSndToWav(const MemChunk& in, MemChunk& out, bool audioT = false);
	bool doomSndToWav(const MemChunk& in, MemChunk& out);
	bool wolfSndToWav(const MemChunk& in, MemChunk& out);
	bool jagSndToWav(const MemChunk& in, MemChunk& out);
	bool vocToWav(const MemChunk& in, MemChunk& out);
	bool bloodToWav(const ArchiveEntry* in, MemChunk& out);
	bool auSndToWav(const MemChunk& in, MemChunk& out);
	bool musToMidi(const MemChunk& in, MemChunk& out);
	bool zmusToMidi(const MemChunk& in, MemChunk& out, int subsong = 0, int* num_tracks = nullptr);
	bool gmidToMidi(const MemChunk& in, MemChunk& out);
	bool rmidToMidi(const MemChunk& in, MemChunk& out);
	bool voxToKvx(const MemChunk& in, MemChunk& out);
	bool addImfHeader(const MemChunk& in, MemChunk& out);
} // namespace conversion
} // namespace slade
