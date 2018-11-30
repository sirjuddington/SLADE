#pragma once

class MemChunk
{
public:
	MemChunk(uint32_t size = 0);
	MemChunk(const uint8_t* data, uint32_t size);
	~MemChunk();

	uint8_t& operator[](int a) { return data_[a]; }

	// Accessors
	const uint8_t* getData() const { return data_; }
	uint32_t       getSize() const { return size_; }

	bool hasData();

	bool clear();
	bool reSize(uint32_t new_size, bool preserve_data = true);

	// Data import
	bool importFile(string filename, uint32_t offset = 0, uint32_t len = 0);
	bool importFileStream(wxFile& file, uint32_t len = 0);
	bool importMem(const uint8_t* start, uint32_t len);

	// Data export
	bool exportFile(string filename, uint32_t start = 0, uint32_t size = 0);
	bool exportMemChunk(MemChunk& mc, uint32_t start = 0, uint32_t size = 0);

	// C-style reading/writing
	bool     write(const void* data, uint32_t size);
	bool     write(const void* data, uint32_t size, uint32_t start);
	bool     read(void* buf, uint32_t size);
	bool     read(void* buf, uint32_t size, uint32_t start);
	bool     seek(uint32_t offset, uint32_t start);
	uint32_t currentPos() { return cur_ptr_; }

	// Extended C-style reading/writing
	bool readMC(MemChunk& mc, uint32_t size);

	// Misc
	bool     fillData(uint8_t val);
	uint32_t crc();

protected:
	uint8_t* data_;
	uint32_t cur_ptr_;
	uint32_t size_;

	uint8_t* allocData(uint32_t size, bool set_data = true);
};
