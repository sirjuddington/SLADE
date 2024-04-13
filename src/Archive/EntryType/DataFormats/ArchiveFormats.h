#pragma once

#include "Archive/ArchiveFormatHandler.h"

class ArchiveDataFormat : public EntryDataFormat
{
public:
	ArchiveDataFormat(string_view id, ArchiveFormat format, int match_probability = MATCH_TRUE) :
		EntryDataFormat(id),
		format_{ format },
		match_probability_{ match_probability }
	{
	}
	~ArchiveDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		return archive::isFormat(mc, format_) ? match_probability_ : MATCH_FALSE;
	}

private:
	ArchiveFormat format_;
	int           match_probability_ = MATCH_TRUE;
};

class WadDataFormat : public ArchiveDataFormat
{
public:
	WadDataFormat() : ArchiveDataFormat("archive_wad", ArchiveFormat::Wad) {}
};

class ZipDataFormat : public ArchiveDataFormat
{
public:
	ZipDataFormat() : ArchiveDataFormat("archive_zip", ArchiveFormat::Zip) {}
};

class LibDataFormat : public ArchiveDataFormat
{
public:
	LibDataFormat() : ArchiveDataFormat("archive_lib", ArchiveFormat::Lib) {}
};

class DatDataFormat : public ArchiveDataFormat
{
public:
	DatDataFormat() : ArchiveDataFormat("archive_dat", ArchiveFormat::Dat) {}
};

class ResDataFormat : public ArchiveDataFormat
{
public:
	ResDataFormat() : ArchiveDataFormat("archive_res", ArchiveFormat::Res) {}
};

class PakDataFormat : public ArchiveDataFormat
{
public:
	PakDataFormat() : ArchiveDataFormat("archive_pak", ArchiveFormat::Pak) {}
};

class BSPDataFormat : public ArchiveDataFormat
{
public:
	BSPDataFormat() : ArchiveDataFormat("archive_bsp", ArchiveFormat::Bsp) {}
};

class Wad2DataFormat : public ArchiveDataFormat
{
public:
	Wad2DataFormat() : ArchiveDataFormat("archive_wad2", ArchiveFormat::Wad2) {}
};

class WadJDataFormat : public ArchiveDataFormat
{
public:
	WadJDataFormat() : ArchiveDataFormat("archive_wadj", ArchiveFormat::WadJ) {}
};

class GrpDataFormat : public ArchiveDataFormat
{
public:
	GrpDataFormat() : ArchiveDataFormat("archive_grp", ArchiveFormat::Grp) {}
};

class RffDataFormat : public ArchiveDataFormat
{
public:
	RffDataFormat() : ArchiveDataFormat("archive_rff", ArchiveFormat::Rff) {}
};

class GobDataFormat : public ArchiveDataFormat
{
public:
	GobDataFormat() : ArchiveDataFormat("archive_gob", ArchiveFormat::Gob) {}
};

class LfdDataFormat : public ArchiveDataFormat
{
public:
	LfdDataFormat() : ArchiveDataFormat("archive_lfd", ArchiveFormat::Lfd) {}
};

class ADatDataFormat : public ArchiveDataFormat
{
public:
	ADatDataFormat() : ArchiveDataFormat("archive_adat", ArchiveFormat::ADat) {}
};

class HogDataFormat : public ArchiveDataFormat
{
public:
	HogDataFormat() : ArchiveDataFormat("archive_hog", ArchiveFormat::Hog) {}
};

class WolfDataFormat : public ArchiveDataFormat
{
public:
	WolfDataFormat() : ArchiveDataFormat("archive_wolf", ArchiveFormat::Wolf) {}
};

class GZipDataFormat : public ArchiveDataFormat
{
public:
	GZipDataFormat() : ArchiveDataFormat("archive_gzip", ArchiveFormat::GZip) {}
};

class BZip2DataFormat : public ArchiveDataFormat
{
public:
	BZip2DataFormat() : ArchiveDataFormat("archive_bz2", ArchiveFormat::Bz2) {}
};

class TarDataFormat : public ArchiveDataFormat
{
public:
	TarDataFormat() : ArchiveDataFormat("archive_tar", ArchiveFormat::Tar) {}
};

class DiskDataFormat : public ArchiveDataFormat
{
public:
	DiskDataFormat() : ArchiveDataFormat("archive_disk", ArchiveFormat::Disk) {}
};

class PodArchiveDataFormat : public ArchiveDataFormat
{
public:
	PodArchiveDataFormat() : ArchiveDataFormat("archive_pod", ArchiveFormat::Pod, MATCH_PROBABLY) {}
};

class ChasmBinArchiveDataFormat : public ArchiveDataFormat
{
public:
	ChasmBinArchiveDataFormat() : ArchiveDataFormat("archive_chasm_bin", ArchiveFormat::ChasmBin) {}
};

class SinArchiveDataFormat : public ArchiveDataFormat
{
public:
	SinArchiveDataFormat() : ArchiveDataFormat("archive_sin", ArchiveFormat::SiN) {}
};
