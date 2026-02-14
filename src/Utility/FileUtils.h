#pragma once

#include "SeekableData.h"

namespace slade
{
namespace fileutil
{
	bool           fileExists(string_view path);
	bool           dirExists(string_view path);
	bool           validExecutable(string_view path);
	bool           removeFile(string_view path);
	bool           copyFile(string_view from, string_view to, bool overwrite = true);
	bool           readFileToString(const string& path, string& str);
	bool           writeStringToFile(const string& str, const string& path);
	bool           createDir(string_view path);
	bool           removeDir(string_view path);
	vector<string> allFilesInDir(string_view path, bool include_subdirs = false, bool include_dir_paths = false);
	time_t         fileModifiedTime(string_view path);
	string         findExecutable(string_view exe_name, string_view bundle_dir = {});
} // namespace fileutil

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
	SFile(string_view path, Mode mode = Mode::ReadOnly);
	~SFile() override { close(); }

	bool     isOpen() const { return handle_ != nullptr; }
	unsigned currentPos() const override;
	unsigned length() const { return handle_ ? size_ : 0; }
	unsigned size() const override { return handle_ ? size_ : 0; }

	bool open(const string& path, Mode mode = Mode::ReadOnly);
	void close();

	bool seek(unsigned offset) override;
	bool seekFromStart(unsigned offset) override;
	bool seekFromEnd(unsigned offset) override;

	bool read(void* buffer, unsigned count) override;
	bool read(MemChunk& mc, unsigned count);
	bool read(string& str, unsigned count) const;

	bool write(const void* buffer, unsigned count) override;
	bool writeStr(string_view str) const;

private:
	FILE*    handle_ = nullptr;
	unsigned size_   = 0;
};
} // namespace slade
