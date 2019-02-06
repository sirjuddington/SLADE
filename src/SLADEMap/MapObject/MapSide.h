#pragma once

#include "MapObject.h"

class MapSide : public MapObject
{
	friend class SLADEMap;
	friend class MapLine;
	friend class SideList;

public:
	static const wxString TEX_NONE;

	// UDMF property names
	static const wxString PROP_SECTOR;
	static const wxString PROP_TEXUPPER;
	static const wxString PROP_TEXMIDDLE;
	static const wxString PROP_TEXLOWER;
	static const wxString PROP_OFFSETX;
	static const wxString PROP_OFFSETY;

	MapSide(
		MapSector*      sector     = nullptr,
		const wxString& tex_upper  = TEX_NONE,
		const wxString& tex_middle = TEX_NONE,
		const wxString& tex_lower  = TEX_NONE,
		Vec2i           tex_offset = { 0, 0 });
	MapSide(MapSector* sector, ParseTreeNode* udmf_def);
	~MapSide() = default;

	void copy(MapObject* c) override;

	bool isOk() const { return !!sector_; }

	MapSector*      sector() const { return sector_; }
	MapLine*        parentLine() const { return parent_; }
	const wxString& texUpper() const { return tex_upper_; }
	const wxString& texMiddle() const { return tex_middle_; }
	const wxString& texLower() const { return tex_lower_; }
	short           texOffsetX() const { return tex_offset_.x; }
	short           texOffsetY() const { return tex_offset_.y; }
	Vec2i           texOffset() const { return tex_offset_; }
	uint8_t         light();

	void setSector(MapSector* sector);
	void changeLight(int amount);
	void setTexUpper(const wxString& tex, bool modify = true);
	void setTexMiddle(const wxString& tex, bool modify = true);
	void setTexLower(const wxString& tex, bool modify = true);
	void setTexOffsetX(int offset);
	void setTexOffsetY(int offset);

	int      intProperty(const wxString& key) override;
	void     setIntProperty(const wxString& key, int value) override;
	wxString stringProperty(const wxString& key) override;
	void     setStringProperty(const wxString& key, const wxString& value) override;
	bool     scriptCanModifyProp(const wxString& key) override;

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(wxString& def) override;

private:
	// Basic data
	MapSector* sector_     = nullptr;
	MapLine*   parent_     = nullptr;
	wxString   tex_upper_  = "-";
	wxString   tex_middle_ = "-";
	wxString   tex_lower_  = "-";
	Vec2i      tex_offset_ = { 0, 0 };
};
