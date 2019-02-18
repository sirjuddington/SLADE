#pragma once

#include "MapObject.h"

class MapSide : public MapObject
{
	friend class SLADEMap;
	friend class MapLine;
	friend class SideList;

public:
	inline static const std::string TEX_NONE = "-";

	// UDMF property names
	inline static const std::string PROP_SECTOR    = "sector";
	inline static const std::string PROP_TEXUPPER  = "texturetop";
	inline static const std::string PROP_TEXMIDDLE = "texturemiddle";
	inline static const std::string PROP_TEXLOWER  = "texturebottom";
	inline static const std::string PROP_OFFSETX   = "offsetx";
	inline static const std::string PROP_OFFSETY   = "offsety";

	MapSide(
		MapSector*       sector     = nullptr,
		std::string_view tex_upper  = TEX_NONE,
		std::string_view tex_middle = TEX_NONE,
		std::string_view tex_lower  = TEX_NONE,
		Vec2i            tex_offset = { 0, 0 });
	MapSide(MapSector* sector, ParseTreeNode* udmf_def);
	~MapSide() = default;

	void copy(MapObject* c) override;

	bool isOk() const { return !!sector_; }

	MapSector*         sector() const { return sector_; }
	MapLine*           parentLine() const { return parent_; }
	const std::string& texUpper() const { return tex_upper_; }
	const std::string& texMiddle() const { return tex_middle_; }
	const std::string& texLower() const { return tex_lower_; }
	short              texOffsetX() const { return tex_offset_.x; }
	short              texOffsetY() const { return tex_offset_.y; }
	Vec2i              texOffset() const { return tex_offset_; }
	uint8_t            light();

	void setSector(MapSector* sector);
	void changeLight(int amount);
	void setTexUpper(std::string_view tex, bool modify = true);
	void setTexMiddle(std::string_view tex, bool modify = true);
	void setTexLower(std::string_view tex, bool modify = true);
	void setTexOffsetX(int offset);
	void setTexOffsetY(int offset);

	int         intProperty(std::string_view key) override;
	void        setIntProperty(std::string_view key, int value) override;
	std::string stringProperty(std::string_view key) override;
	void        setStringProperty(std::string_view key, std::string_view value) override;
	bool        scriptCanModifyProp(std::string_view key) override;

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(std::string& def) override;

private:
	// Basic data
	MapSector*  sector_     = nullptr;
	MapLine*    parent_     = nullptr;
	std::string tex_upper_  = "-";
	std::string tex_middle_ = "-";
	std::string tex_lower_  = "-";
	Vec2i       tex_offset_ = { 0, 0 };
};
