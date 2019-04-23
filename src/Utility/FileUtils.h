#pragma once

#include "SeekableData.h"

namespace FileUtil
{
bool                fileExists(std::string_view path);
bool                dirExists(std::string_view path);
bool                removeFile(std::string_view path);
bool                copyFile(std::string_view from, std::string_view to);
bool                readFileToString(const std::string& path, std::string& str);
bool                writeStringToFile(std::string& str, const std::string& path);
bool                createDir(std::string_view path);
vector<std::string> allFilesInDir(std::string_view path, bool include_subdirs = false, bool include_dir_paths = false);
time_t              fileModifiedTime(std::string_view path);
} // namespace FileUtil

class SFile : public SeekableData
{
public:
	enum class Mode
	{
		ReadOnly,
		Write,
		ReadWite,
		Append
	};

	SFile() = default;
	SFile(std::string_view path, Mode mode = Mode::ReadOnly);
	~SFile() { close(); }

	bool     isOpen() const { return handle_ != nullptr; }
	unsigned currentPos() const override;
	unsigned length() const { return handle_ ? stat_.st_size : 0; }
	unsigned size() const override { return handle_ ? stat_.st_size : 0; }

	bool open(const std::string& path, Mode mode = Mode::ReadOnly);
	void close();

	bool seek(unsigned offset) override;
	bool seekFromStart(unsigned offset) override;
	bool seekFromEnd(unsigned offset) override;

	bool read(void* buffer, unsigned count) override;
	bool read(MemChunk& mc, unsigned count);
	bool read(std::string& str, unsigned count) const;

	bool write(const void* buffer, unsigned count) override;
	bool writeStr(std::string_view str) const;

private:
	FILE*       handle_ = nullptr;
	struct stat stat_;
};
