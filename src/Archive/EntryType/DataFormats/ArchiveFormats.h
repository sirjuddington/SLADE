#pragma once

class WadDataFormat : public EntryDataFormat
{
public:
	WadDataFormat() : EntryDataFormat("archive_wad") {}
	~WadDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return WadArchive::isWadArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class ZipDataFormat : public EntryDataFormat
{
public:
	ZipDataFormat() : EntryDataFormat("archive_zip") {}
	~ZipDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return ZipArchive::isZipArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class LibDataFormat : public EntryDataFormat
{
public:
	LibDataFormat() : EntryDataFormat("archive_lib") {}
	~LibDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return LibArchive::isLibArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class DatDataFormat : public EntryDataFormat
{
public:
	DatDataFormat() : EntryDataFormat("archive_dat") {}
	~DatDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return DatArchive::isDatArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class ResDataFormat : public EntryDataFormat
{
public:
	ResDataFormat() : EntryDataFormat("archive_res") {}
	~ResDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return ResArchive::isResArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class PakDataFormat : public EntryDataFormat
{
public:
	PakDataFormat() : EntryDataFormat("archive_pak") {}
	~PakDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return PakArchive::isPakArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class BSPDataFormat : public EntryDataFormat
{
public:
	BSPDataFormat() : EntryDataFormat("archive_bsp") {}
	~BSPDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return BSPArchive::isBSPArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class Wad2DataFormat : public EntryDataFormat
{
public:
	Wad2DataFormat() : EntryDataFormat("archive_wad2") {}
	~Wad2DataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return Wad2Archive::isWad2Archive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class WadJDataFormat : public EntryDataFormat
{
public:
	WadJDataFormat() : EntryDataFormat("archive_wadj") {}
	~WadJDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return WadJArchive::isWadJArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class GrpDataFormat : public EntryDataFormat
{
public:
	GrpDataFormat() : EntryDataFormat("archive_grp") {}
	~GrpDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return GrpArchive::isGrpArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class RffDataFormat : public EntryDataFormat
{
public:
	RffDataFormat() : EntryDataFormat("archive_rff") {}
	~RffDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return RffArchive::isRffArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class GobDataFormat : public EntryDataFormat
{
public:
	GobDataFormat() : EntryDataFormat("archive_gob") {}
	~GobDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return GobArchive::isGobArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class LfdDataFormat : public EntryDataFormat
{
public:
	LfdDataFormat() : EntryDataFormat("archive_lfd") {}
	~LfdDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return LfdArchive::isLfdArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class ADatDataFormat : public EntryDataFormat
{
public:
	ADatDataFormat() : EntryDataFormat("archive_adat") {}
	~ADatDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return ADatArchive::isADatArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class HogDataFormat : public EntryDataFormat
{
public:
	HogDataFormat() : EntryDataFormat("archive_hog") {}
	~HogDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return HogArchive::isHogArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class WolfDataFormat : public EntryDataFormat
{
public:
	WolfDataFormat() : EntryDataFormat("archive_wolf") {}
	~WolfDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return WolfArchive::isWolfArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class GZipDataFormat : public EntryDataFormat
{
public:
	GZipDataFormat() : EntryDataFormat("archive_gzip") {}
	~GZipDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return GZipArchive::isGZipArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class BZip2DataFormat : public EntryDataFormat
{
public:
	BZip2DataFormat() : EntryDataFormat("archive_bz2") {}
	~BZip2DataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		return BZip2Archive::isBZip2Archive(mc) ? MATCH_TRUE : MATCH_FALSE;
	}
};

class TarDataFormat : public EntryDataFormat
{
public:
	TarDataFormat() : EntryDataFormat("archive_tar") {}
	~TarDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return TarArchive::isTarArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class DiskDataFormat : public EntryDataFormat
{
public:
	DiskDataFormat() : EntryDataFormat("archive_disk") {}
	~DiskDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override { return PakArchive::isPakArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};

class PodArchiveDataFormat : public EntryDataFormat
{
public:
	PodArchiveDataFormat() : EntryDataFormat("archive_pod") {}
	~PodArchiveDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		return PodArchive::isPodArchive(mc) ? MATCH_PROBABLY : MATCH_FALSE;
	}
};

class ChasmBinArchiveDataFormat : public EntryDataFormat
{
public:
	ChasmBinArchiveDataFormat() : EntryDataFormat("archive_chasm_bin") {}

	int isThisFormat(const MemChunk& mc) override
	{
		return ChasmBinArchive::isChasmBinArchive(mc) ? MATCH_TRUE : MATCH_FALSE;
	}
};

class SinArchiveDataFormat : public EntryDataFormat
{
public:
	SinArchiveDataFormat() : EntryDataFormat("archive_sin") {}

	int isThisFormat(const MemChunk& mc) override { return SiNArchive::isSiNArchive(mc) ? MATCH_TRUE : MATCH_FALSE; }
};
