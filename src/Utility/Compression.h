#pragma once

namespace slade::compression
{
bool genericInflate(const MemChunk& in, MemChunk& out, int windowbits, const char* function);
bool genericDeflate(const MemChunk& in, MemChunk& out, int level, int windowbits, const char* function);
bool gzipInflate(const MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool gzipDeflate(const MemChunk& in, MemChunk& out, int level = -1);
bool zipInflate(const MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool zipDeflate(const MemChunk& in, MemChunk& out, int level = -1);
bool zlibInflate(const MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool zlibDeflate(const MemChunk& in, MemChunk& out, int level = -1);
bool zipExplode(MemChunk& in, MemChunk& out, size_t size, int flags);
bool zipUnshrink(MemChunk& in, MemChunk& out, size_t maxsize);
bool bzip2Decompress(const MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool bzip2Compress(const MemChunk& in, MemChunk& out);
bool lzmaDecompress(const MemChunk& in, MemChunk& out, size_t size);
} // namespace slade::compression
