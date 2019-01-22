#pragma once

#include "MapObject.h"

class MapSide : public MapObject
{
	friend class SLADEMap;
	friend class MapLine;
	friend class SideList;

public:
	static const string TEX_NONE;

	// UDMF property names
	static const string PROP_SECTOR;
	static const string PROP_TEXUPPER;
	static const string PROP_TEXMIDDLE;
	static const string PROP_TEXLOWER;
	static const string PROP_OFFSETX;
	static const string PROP_OFFSETY;

	MapSide(
		MapSector*    sector     = nullptr,
		const string& tex_upper  = TEX_NONE,
		const string& tex_middle = TEX_NONE,
		const string& tex_lower  = TEX_NONE,
		Vec2i         tex_offset = { 0, 0 });
	MapSide(MapSector* sector, ParseTreeNode* udmf_def);
	~MapSide() = default;

	void copy(MapObject* c) override;

	bool isOk() const { return !!sector_; }

	MapSector*    sector() const { return sector_; }
	MapLine*      parentLine() const { return parent_; }
	const string& texUpper() const { return tex_upper_; }
	const string& texMiddle() const { return tex_middle_; }
	const string& texLower() const { return tex_lower_; }
	short         texOffsetX() const { return tex_offset_.x; }
	short         texOffsetY() const { return tex_offset_.y; }
	Vec2i         texOffset() const { return tex_offset_; }
	uint8_t       light();

	void setSector(MapSector* sector);
	void changeLight(int amount);

	int    intProperty(const string& key) override;
	void   setIntProperty(const string& key, int value) override;
	string stringProperty(const string& key) override;
	void   setStringProperty(const string& key, const string& value) override;
	bool   scriptCanModifyProp(const string& key) override;

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(string& def) override;

private:
	// Basic data
	MapSector* sector_     = nullptr;
	MapLine*   parent_     = nullptr;
	string     tex_upper_  = "-";
	string     tex_middle_ = "-";
	string     tex_lower_  = "-";
	Vec2i      tex_offset_ = { 0, 0 };

	void setTexUpper(const string& tex);
	void setTexMiddle(const string& tex);
	void setTexLower(const string& tex);
};
