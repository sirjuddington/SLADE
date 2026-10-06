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
	string         fileHash(string_view path);
	string         systemPath(string_view path);
	string         sanitizeFilename(string_view filename);
} // namespace fileutil

class SFile : public SeekableData
{
public:
	enum class Mode
	{
		ReadOnly,
		Write,
		ReadWrite,
		Append
	};

	SFile() = default;
	SFile(string_view path, Mode mode = Mode::ReadOnly);
	~SFile() override { close(); }

	bool          isOpen() const { return handle_ != nullptr; }
	u64           currentPos() const override;
	u64           length() const { return handle_ ? size_ : 0; }
	u64           size() const override { return handle_ ? size_ : 0; }
	FILE*         handle() const { return handle_; }
	const string& path() const { return path_; }

	bool open(const string& path, Mode mode = Mode::ReadOnly);
	void close();

	bool seek(u64 offset) const override;
	bool seekFromStart(u64 offset) const override;
	bool seekFromEnd(u64 offset) const override;

	bool read(void* buffer, u64 count) const override;
	bool read(MemChunk& mc, u64 count) const;
	bool read(string& str, u64 count) const;

	bool write(const void* buffer, u64 count) override;
	bool writeI32(i32 value, bool big_endian = false) const;
	bool writeU32(u32 value, bool big_endian = false) const;
	bool writeI64(i64 value, bool big_endian = false) const;
	bool writeU64(u64 value, bool big_endian = false) const;
	bool writeStr(string_view str) const;

	u64 lastReadCount() const override { return last_read_count_; }

	string calculateHash() const;

private:
	FILE*       handle_          = nullptr;
	u64         size_            = 0;
	mutable u64 last_read_count_ = 0;
	string      path_;
};
} // namespace slade
