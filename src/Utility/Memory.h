#pragma once

namespace slade::memory
{
// Platform-independent functions to read litte-endian values from 8-bit arrays
inline uint16_t readL16(const uint8_t* data, unsigned ofs)
{
	return data[ofs] + (data[ofs + 1] << 8);
}
inline uint32_t readL24(const uint8_t* data, unsigned ofs)
{
	return data[ofs] + (data[ofs + 1] << 8) + (data[ofs + 2] << 16);
}
inline uint32_t readL32(const uint8_t* data, unsigned ofs)
{
	return data[ofs] + (data[ofs + 1] << 8) + (data[ofs + 2] << 16) + (data[ofs + 3] << 24);
}

// Platform-independent functions to read big-endian values from 8-bit arrays
inline uint16_t readB16(const uint8_t* data, unsigned ofs)
{
	return data[ofs + 1] + (data[ofs] << 8);
}
inline uint32_t readB24(const uint8_t* data, unsigned ofs)
{
	return data[ofs + 2] + (data[ofs + 1] << 8) + (data[ofs] << 16);
}
inline uint32_t readB32(const uint8_t* data, unsigned ofs)
{
	return data[ofs + 3] + (data[ofs + 2] << 8) + (data[ofs + 1] << 16) + (data[ofs] << 24);
}
} // namespace slade::memory
