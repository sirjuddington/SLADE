#pragma once

class MemChunk
{
public:
	MemChunk() = default;
	MemChunk(uint32_t size);
	MemChunk(const uint8_t* data, uint32_t size);
	~MemChunk();

	uint8_t& operator[](int a) const { return data_[a]; }

	// Accessors
	const uint8_t* data() const { return data_; }
	uint8_t*       data() { return data_; }
	uint32_t       size() const { return size_; }

	bool hasData() const;

	bool clear();
	bool reSize(uint32_t new_size, bool preserve_data = true);

	// Data import
	bool importFile(std::string_view filename, uint32_t offset = 0, uint32_t len = 0);
	bool importFileStream(wxFile& file, uint32_t len = 0);
	bool importMem(const uint8_t* start, uint32_t len);
	bool importMem(const MemChunk& other) { return importMem(other.data_, other.size_); }

	// Data export
	bool exportFile(std::string_view filename, uint32_t start = 0, uint32_t size = 0) const;
	bool exportMemChunk(MemChunk& mc, uint32_t start = 0, uint32_t size = 0) const;

	// General reading/writing
	bool write(unsigned offset, const void* data, unsigned size, bool expand);
	bool read(unsigned offset, void* buf, unsigned size) const;

	// C-style reading/writing
	bool     write(const void* data, uint32_t size);
	bool     write(const void* data, uint32_t size, uint32_t start);
	bool     read(void* buf, uint32_t size);
	bool     read(void* buf, uint32_t size, uint32_t start);
	bool     seek(uint32_t offset, uint32_t start);
	uint32_t currentPos() const { return cur_ptr_; }

	// Extended C-style reading/writing
	bool readMC(MemChunk& mc, uint32_t size);

	// Misc
	bool     fillData(uint8_t val) const;
	uint32_t crc() const;

	// Platform-independent functions to read values in little (L##) or big (B##) endian
	uint16_t readL16(unsigned i) const { return data_[i] + (data_[i + 1] << 8); }
	uint32_t readL24(unsigned i) const { return data_[i] + (data_[i + 1] << 8) + (data_[i + 2] << 16); }
	uint32_t readL32(unsigned i) const
	{
		return (data_[i] + (data_[i + 1] << 8) + (data_[i + 2] << 16) + (data_[i + 3] << 24));
	}
	uint16_t readB16(unsigned i) const { return data_[i + 1] + (data_[i] << 8); }
	uint32_t readB24(unsigned i) const { return data_[i + 2] + (data_[i + 1] << 8) + (data_[i] << 16); }
	uint32_t readB32(unsigned i) const
	{
		return data_[i + 3] + (data_[i + 2] << 8) + (data_[i + 1] << 16) + (data_[i] << 24);
	}

protected:
	uint8_t* data_    = nullptr;
	uint32_t cur_ptr_ = 0;
	uint32_t size_    = 0;

	uint8_t* allocData(uint32_t size, bool set_data = true);
};
