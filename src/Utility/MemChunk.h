#pragma once

#include "SeekableData.h"

namespace slade
{
class SFile;

class MemChunk : public SeekableData
{
public:
	MemChunk() = default;
	MemChunk(u32 size);
	MemChunk(const u8* data, u32 size);
	MemChunk(const MemChunk& copy) = default;
	~MemChunk() override           = default;

	u8&       operator[](int a) { return data_[a]; }
	const u8& operator[](int a) const { return data_[a]; }

	// Accessors
	const u8* data() const { return data_.data(); }
	u8*       data() { return data_.data(); }

	// SeekableData
	unsigned size() const override { return static_cast<unsigned>(data_.size()); }
	unsigned currentPos() const override { return cur_ptr_; }
	bool     seek(unsigned offset) const override { return seek(offset, SEEK_CUR); }
	bool     seekFromStart(unsigned offset) const override { return seek(offset, SEEK_SET); }
	bool     seekFromEnd(unsigned offset) const override { return seek(offset, SEEK_END); }
	bool     read(void* buffer, unsigned count) const override;
	bool     write(const void* buffer, unsigned count) override;

	bool hasData() const;
	bool empty() const { return !hasData(); }

	bool clear();
	bool reSize(u32 new_size, bool preserve_data = true);

	// Data import
	bool importFile(string_view filename, u32 offset = 0, u32 len = 0);
	bool importFileStreamWx(wxFile& file, u32 len = 0);
	bool importFileStream(const SFile& file, unsigned len = 0);
	bool importMem(const u8* start, u32 len);
	bool importMem(const MemChunk& other) { return importMem(other.data(), static_cast<u32>(other.data_.size())); }

	// Data export
	bool exportFile(string_view filename, u32 start = 0, u32 size = 0) const;
	bool exportMemChunk(MemChunk& mc, u32 start = 0, u32 size = 0) const;

	// General reading/writing
	bool write(unsigned offset, const void* data, unsigned size, bool expand);
	bool read(unsigned offset, void* buf, unsigned size) const;

	// C-style reading/writing
	bool write(const void* data, u32 size, u32 start);
	bool read(void* buf, u32 size, u32 start) const;
	bool seek(u32 offset, u32 start) const;

	// Extended C-style reading/writing
	bool readMC(MemChunk& mc, u32 size) const;

	// Misc
	bool   fillData(u8 val);
	u32    crc() const;
	string hash() const;
	string asString(u32 offset = 0, u32 length = 0) const;
	u8*    releaseData();

	// Platform-independent functions to read values in little (L##) or big (B##) endian
	u16 readL16(unsigned i) const { return data_[i] + (data_[i + 1] << 8); }
	u32 readL24(unsigned i) const { return data_[i] + (data_[i + 1] << 8) + (data_[i + 2] << 16); }
	u32 readL32(unsigned i) const
	{
		return (data_[i] + (data_[i + 1] << 8) + (data_[i + 2] << 16) + (data_[i + 3] << 24));
	}
	u16 readB16(unsigned i) const { return data_[i + 1] + (data_[i] << 8); }
	u32 readB24(unsigned i) const { return data_[i + 2] + (data_[i + 1] << 8) + (data_[i] << 16); }
	u32 readB32(unsigned i) const
	{
		return data_[i + 3] + (data_[i + 2] << 8) + (data_[i + 1] << 16) + (data_[i] << 24);
	}

protected:
	std::vector<u8> data_;
	mutable u32     cur_ptr_ = 0;
};
} // namespace slade
