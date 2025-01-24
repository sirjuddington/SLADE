#pragma once

#include "MapObject.h"

namespace slade
{
class MapSide : public MapObject
{
	friend class SLADEMap;
	friend class MapLine;
	friend class SideList;

public:
	inline static const string TEX_NONE = "-";

	// UDMF property names
	inline static const string PROP_SECTOR    = "sector";
	inline static const string PROP_TEXUPPER  = "texturetop";
	inline static const string PROP_TEXMIDDLE = "texturemiddle";
	inline static const string PROP_TEXLOWER  = "texturebottom";
	inline static const string PROP_OFFSETX   = "offsetx";
	inline static const string PROP_OFFSETY   = "offsety";

	MapSide(
		MapSector*   sector     = nullptr,
		string_view  tex_upper  = TEX_NONE,
		string_view  tex_middle = TEX_NONE,
		string_view  tex_lower  = TEX_NONE,
		const Vec2i& tex_offset = { 0, 0 });
	MapSide(MapSector* sector, const ParseTreeNode* udmf_def);
	MapSide(MapSector* sector, MapSide* copy_side);
	~MapSide() override = default;

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
	void setTexUpper(string_view tex, bool modify = true);
	void setTexMiddle(string_view tex, bool modify = true);
	void setTexLower(string_view tex, bool modify = true);
	void setTexOffsetX(int offset);
	void setTexOffsetY(int offset);

	int    intProperty(string_view key) const override;
	void   setIntProperty(string_view key, int value) override;
	string stringProperty(string_view key) const override;
	void   setStringProperty(string_view key, string_view value) override;
	bool   scriptCanModifyProp(string_view key) const override;

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
};
} // namespace slade
