#pragma once

#include "Archive/ArchiveEntry.h"
#include "General/ListenerAnnouncer.h"
#include "Graphics/Translation.h"

class SImage;
class Tokenizer;

// Basic patch
class CTPatch
{
public:
	CTPatch();
	CTPatch(string name, int16_t offset_x = 0, int16_t offset_y = 0);
	CTPatch(CTPatch* copy);
	virtual ~CTPatch();

	string  getName() { return name_; }
	int16_t xOffset() { return offset_x_; }
	int16_t yOffset() { return offset_y_; }

	void setName(string name) { this->name_ = name; }
	void setOffsetX(int16_t offset) { offset_x_ = offset; }
	void setOffsetY(int16_t offset) { offset_y_ = offset; }

	virtual ArchiveEntry* getPatchEntry(Archive* parent = nullptr);

protected:
	string  name_;
	int16_t offset_x_;
	int16_t offset_y_;
};

// Extended patch (for TEXTURES)
class CTPatchEx : public CTPatch
{
public:
	enum class Type
	{
		Patch = 0,
		Graphic
	};

	CTPatchEx();
	CTPatchEx(string name, int16_t offset_x = 0, int16_t offset_y = 0, Type type = Type::Patch);
	CTPatchEx(CTPatch* copy);
	CTPatchEx(CTPatchEx* copy);
	~CTPatchEx();

	bool         flipX() { return flip_x_; }
	bool         flipY() { return flip_y_; }
	bool         useOffsets() { return use_offsets_; }
	int16_t      getRotation() { return rotation_; }
	rgba_t       getColour() { return colour_; }
	float        getAlpha() { return alpha_; }
	string       getStyle() { return style_; }
	uint8_t      getBlendType() { return blendtype_; }
	Translation& getTranslation() { return translation_; }

	void flipX(bool flip) { flip_x_ = flip; }
	void flipY(bool flip) { flip_y_ = flip; }
	void useOffsets(bool use) { use_offsets_ = use; }
	void setRotation(int16_t rot) { rotation_ = rot; }
	void setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { colour_.set(r, g, b, a); }
	void setAlpha(float a) { alpha_ = a; }
	void setStyle(string s) { style_ = s; }
	void setBlendType(uint8_t type) { blendtype_ = type; }

	ArchiveEntry* getPatchEntry(Archive* parent = nullptr) override;

	bool   parse(Tokenizer& tz, Type type = Type::Patch);
	string asText();

private:
	Type        type_;
	bool        flip_x_;
	bool        flip_y_;
	bool        use_offsets_;
	int16_t     rotation_;
	Translation translation_;
	rgba_t      colour_;
	float       alpha_;
	string      style_;
	uint8_t     blendtype_; // 0=none, 1=translation, 2=blend, 3=tint
};

class TextureXList;
class SImage;
class Palette;

class CTexture : public Announcer
{
	friend class TextureXList;

public:
	enum class Type
	{
		Texture = 0,
		Sprite,
		Graphic,
		WallTexture,
		Flat,
		HiRes
	};

	CTexture(bool extended = false);
	~CTexture();

	void copyTexture(CTexture* copy, bool keep_type = false);

	string   getName() { return name_; }
	uint16_t getWidth() { return width_; }
	uint16_t getHeight() { return height_; }
	double   getScaleX() { return scale_x_; }
	double   getScaleY() { return scale_y_; }
	int16_t  getOffsetX() { return offset_x_; }
	int16_t  getOffsetY() { return offset_y_; }
	bool     worldPanning() { return world_panning_; }
	string   getType() { return type_; }
	bool     isExtended() { return extended_; }
	bool     isOptional() { return optional_; }
	bool     noDecals() { return no_decals_; }
	bool     nullTexture() { return null_texture_; }
	size_t   nPatches() { return patches_.size(); }
	CTPatch* getPatch(size_t index);
	uint8_t  getState() { return state_; }
	int      getIndex();

	void setName(string name) { this->name_ = name; }
	void setWidth(uint16_t width) { this->width_ = width; }
	void setHeight(uint16_t height) { this->height_ = height; }
	void setScaleX(double scale) { this->scale_x_ = scale; }
	void setScaleY(double scale) { this->scale_y_ = scale; }
	void setScale(double x, double y)
	{
		this->scale_x_ = x;
		this->scale_y_ = y;
	}
	void setOffsetX(int16_t offset) { this->offset_x_ = offset; }
	void setOffsetY(int16_t offset) { this->offset_y_ = offset; }
	void setWorldPanning(bool wp) { this->world_panning_ = wp; }
	void setType(string type) { this->type_ = type; }
	void setExtended(bool ext) { this->extended_ = ext; }
	void setOptional(bool opt) { this->optional_ = opt; }
	void setNoDecals(bool nd) { this->no_decals_ = nd; }
	void setNullTexture(bool nt) { this->null_texture_ = nt; }
	void setState(uint8_t state) { this->state_ = state; }
	void setList(TextureXList* list) { this->in_list_ = list; }

	void clear();

	bool addPatch(string patch, int16_t offset_x = 0, int16_t offset_y = 0, int index = -1);
	bool removePatch(size_t index);
	bool removePatch(string patch);
	bool replacePatch(size_t index, string newpatch);
	bool duplicatePatch(size_t index, int16_t offset_x = 8, int16_t offset_y = 8);
	bool swapPatches(size_t p1, size_t p2);

	bool   parse(Tokenizer& tz, string type);
	bool   parseDefine(Tokenizer& tz);
	string asText();

	bool convertExtended();
	bool convertRegular();
	bool loadPatchImage(unsigned pindex, SImage& image, Archive* parent = nullptr, Palette* pal = nullptr);
	bool toImage(SImage& image, Archive* parent = nullptr, Palette* pal = nullptr, bool force_rgba = false);

	typedef std::unique_ptr<CTexture> UPtr;
	typedef std::shared_ptr<CTexture> SPtr;

private:
	// Basic info
	string           name_;
	uint16_t         width_;
	uint16_t         height_;
	double           scale_x_;
	double           scale_y_;
	bool             world_panning_;
	vector<CTPatch*> patches_;
	int              index_;

	// Extended (TEXTURES) info
	string  type_;
	bool    extended_;
	bool    defined_;
	bool    optional_;
	bool    no_decals_;
	bool    null_texture_;
	int16_t offset_x_;
	int16_t offset_y_;
	int16_t def_width_;
	int16_t def_height_;

	// Editor info
	uint8_t       state_;
	TextureXList* in_list_;
};
