#pragma once

namespace Compression
{
bool GenericInflate(MemChunk& in, MemChunk& out, int windowbits, const char* function);
bool GenericDeflate(MemChunk& in, MemChunk& out, int level, int windowbits, const char* function);
bool GZipInflate(MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool GZipDeflate(MemChunk& in, MemChunk& out, int level = -1);
bool ZipInflate(MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool ZipDeflate(MemChunk& in, MemChunk& out, int level = -1);
bool ZlibInflate(MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool ZlibDeflate(MemChunk& in, MemChunk& out, int level = -1);
bool ZipExplode(MemChunk& in, MemChunk& out, size_t size, int flags);
bool ZipUnshrink(MemChunk& in, MemChunk& out, size_t maxsize);
bool BZip2Decompress(MemChunk& in, MemChunk& out, size_t maxsize = 0);
bool BZip2Compress(MemChunk& in, MemChunk& out);
bool LZMADecompress(MemChunk& in, MemChunk& out, size_t size);
} // namespace Compression
