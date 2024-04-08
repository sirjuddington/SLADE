#pragma once

#include "Utility/StringPair.h"

namespace slade
{
enum class ArchiveFormat
{
	ADat = 0,
	Bsp,
	Bz2,
	ChasmBin,
	Dat,
	Dir,
	Disk,
	Gob,
	Grp,
	GZip,
	Hog,
	Lfd,
	Lib,
	Pak,
	Pod,
	Res,
	Rff,
	SiN,
	Tar,
	Wad,
	WadJ,
	Wad2,
	Wolf,
	Zip,

	Unknown
};

struct ArchiveFormatInfo
{
	string             id;
	string             name;
	bool               supports_dirs    = false;
	bool               names_extensions = true;
	int                max_name_length  = -1;
	string             entry_format;
	vector<StringPair> extensions;
	bool               prefer_uppercase      = false;
	bool               create                = false;
	bool               allow_duplicate_names = true;

	ArchiveFormatInfo() = default;
	ArchiveFormatInfo(string_view id) : id{ id }, name{ id } {}
};

namespace archive
{
	bool                       loadFormatInfo(const MemChunk& mc);
	vector<ArchiveFormatInfo>& allFormatsInfo();
	const ArchiveFormatInfo&   formatInfo(ArchiveFormat format);
	const ArchiveFormatInfo&   formatInfoFromId(string_view id);
	string                     formatId(ArchiveFormat format);
	ArchiveFormat              formatFromId(string_view format_id_string);
} // namespace archive
} // namespace slade
