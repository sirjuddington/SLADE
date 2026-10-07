#pragma once

#include "SeekableData.h"

namespace slade
{
class SFile;

class MemChunk : public SeekableData
{
public:
	MemChunk() = default;
	MemChunk(u64 size);
	MemChunk(const u8* data, u64 size);
	MemChunk(const MemChunk& copy) = default;
	~MemChunk() override           = default;

	u8&       operator[](int a) { return data_[a]; }
	const u8& operator[](int a) const { return data_[a]; }

	// Accessors
	const u8* data() const { return data_.data(); }
	u8*       data() { return data_.data(); }

	// SeekableData
	u64  size() const override { return data_.size(); }
	u64  currentPos() const override { return cur_ptr_; }
	bool seek(u64 offset) const override { return seek(offset, SEEK_CUR); }
	bool seekFromStart(u64 offset) const override { return seek(offset, SEEK_SET); }
	bool seekFromEnd(u64 offset) const override { return seek(offset, SEEK_END); }
	bool read(void* buffer, u64 count) const override;
	bool write(const void* buffer, u64 count) override;

	bool hasData() const;
	bool empty() const { return !hasData(); }

	bool clear();
	bool reSize(u64 new_size, bool preserve_data = true);

	// Data import
	bool importFile(string_view filename, u64 offset = 0, u64 len = 0);
	bool importFileStreamWx(wxFile& file, u64 len = 0);
	bool importFileStream(const SFile& file, u64 len = 0);
	bool importMem(const u8* start, u64 len);
	bool importMem(const MemChunk& other) { return importMem(other.data(), static_cast<u64>(other.data_.size())); }

	// Data export
	bool exportFile(string_view filename, u64 start = 0, u64 size = 0) const;
	bool exportMemChunk(MemChunk& mc, u64 start = 0, u64 size = 0) const;

	// General reading/writing
	bool write(u64 offset, const void* data, u64 size, bool expand);
	bool read(u64 offset, void* buf, u64 size) const;

	// C-style reading/writing
	bool write(const void* data, u64 size, u64 start);
	bool read(void* buf, u64 size, u64 start) const;
	bool seek(u64 offset, u64 start) const;

	// Extended C-style reading/writing
	bool readMC(MemChunk& mc, u64 size) const;

	// Misc
	bool   fillData(u8 val);
	u32    crc() const;
	string hash() const;
	string asString(u64 offset = 0, u64 length = 0) const;
	u8*    releaseData();

	// Platform-independent functions to read values in little (L##) or big (B##) endian
	u16 readL16(u64 i) const { return data_[i] + (data_[i + 1] << 8); }
	u32 readL24(u64 i) const { return data_[i] + (data_[i + 1] << 8) + (data_[i + 2] << 16); }
	u32 readL32(u64 i) const { return (data_[i] + (data_[i + 1] << 8) + (data_[i + 2] << 16) + (data_[i + 3] << 24)); }
	u16 readB16(u64 i) const { return data_[i + 1] + (data_[i] << 8); }
	u32 readB24(u64 i) const { return data_[i + 2] + (data_[i + 1] << 8) + (data_[i] << 16); }
	u32 readB32(u64 i) const { return data_[i + 3] + (data_[i + 2] << 8) + (data_[i + 1] << 16) + (data_[i] << 24); }

protected:
	std::vector<u8> data_;
	mutable u64     cur_ptr_ = 0;
};
} // namespace slade
