#pragma once

namespace Compression
{
bool genericInflate(MemChunk& in, MemChunk& out, int windowbits, const char* function);
bool genericDeflate(MemChunk& in, MemChunk& out, int level, int windowbits, const char* function);
bool gzipInflate(MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool gzipDeflate(MemChunk& in, MemChunk& out, int level = -1);
bool zipInflate(MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool zipDeflate(MemChunk& in, MemChunk& out, int level = -1);
bool zlibInflate(MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool zlibDeflate(MemChunk& in, MemChunk& out, int level = -1);
bool zipExplode(MemChunk& in, MemChunk& out, size_t size, int flags);
bool zipUnshrink(MemChunk& in, MemChunk& out, size_t maxsize);
bool bzip2Decompress(MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool bzip2Compress(MemChunk& in, MemChunk& out);
bool lzmaDecompress(MemChunk& in, MemChunk& out, size_t size);
} // namespace Compression
