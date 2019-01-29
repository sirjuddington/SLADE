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
	CTPatch() = default;
	CTPatch(const string& name, int16_t offset_x = 0, int16_t offset_y = 0);
	virtual ~CTPatch() = default;

	string  name() const { return name_; }
	int16_t xOffset() const { return offset_x_; }
	int16_t yOffset() const { return offset_y_; }

	void setName(const string& name) { name_ = name; }
	void setOffsetX(int16_t offset) { offset_x_ = offset; }
	void setOffsetY(int16_t offset) { offset_y_ = offset; }

	virtual ArchiveEntry* patchEntry(Archive* parent = nullptr);

	typedef std::unique_ptr<CTPatch> UPtr;

protected:
	string  name_;
	int16_t offset_x_ = 0;
	int16_t offset_y_ = 0;
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

	CTPatchEx() = default;
	CTPatchEx(const string& name, int16_t offset_x = 0, int16_t offset_y = 0, Type type = Type::Patch);
	CTPatchEx(const CTPatch& copy) : CTPatch{ copy } {}
	CTPatchEx(const CTPatchEx& copy);
	~CTPatchEx() = default;

	bool         flipX() const { return flip_x_; }
	bool         flipY() const { return flip_y_; }
	bool         useOffsets() const { return use_offsets_; }
	int16_t      rotation() const { return rotation_; }
	ColRGBA      colour() const { return colour_; }
	float        alpha() const { return alpha_; }
	string       style() const { return style_; }
	uint8_t      blendType() const { return blendtype_; }
	Translation& translation() { return translation_; }

	void flipX(bool flip) { flip_x_ = flip; }
	void flipY(bool flip) { flip_y_ = flip; }
	void useOffsets(bool use) { use_offsets_ = use; }
	void setRotation(int16_t rot) { rotation_ = rot; }
	void setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { colour_.set(r, g, b, a); }
	void setAlpha(float a) { alpha_ = a; }
	void setStyle(const string& style) { style_ = style; }
	void setBlendType(uint8_t type) { blendtype_ = type; }

	ArchiveEntry* patchEntry(Archive* parent = nullptr) override;

	bool   parse(Tokenizer& tz, Type type = Type::Patch);
	string asText();

private:
	Type        type_        = Type::Patch;
	bool        flip_x_      = false;
	bool        flip_y_      = false;
	bool        use_offsets_ = false;
	int16_t     rotation_    = 0;
	Translation translation_;
	ColRGBA     colour_;
	float       alpha_     = 1.f;
	string      style_     = "Copy";
	uint8_t     blendtype_ = 0; // 0=none, 1=translation, 2=blend, 3=tint
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

	CTexture(bool extended = false) : extended_{ extended } {}
	CTexture(const string& name, bool extended = false) : name_{ name }, extended_{ extended } {}
	~CTexture() = default;

	void copyTexture(const CTexture& tex, bool keep_type = false);

	string   name() const { return name_; }
	uint16_t width() const { return width_; }
	uint16_t height() const { return height_; }
	double   scaleX() const { return scale_.x; }
	double   scaleY() const { return scale_.y; }
	Vec2d    scale() const { return scale_; }
	int16_t  offsetX() const { return offset_x_; }
	int16_t  offsetY() const { return offset_y_; }
	bool     worldPanning() const { return world_panning_; }
	string   type() const { return type_; }
	bool     isExtended() const { return extended_; }
	bool     isOptional() const { return optional_; }
	bool     noDecals() const { return no_decals_; }
	bool     nullTexture() const { return null_texture_; }
	size_t   nPatches() const { return patches_.size(); }
	CTPatch* patch(size_t index) const;
	uint8_t  state() const { return state_; }
	int      index() const;

	void setName(const string& name) { name_ = name; }
	void setWidth(uint16_t width) { width_ = width; }
	void setHeight(uint16_t height) { height_ = height; }
	void setScaleX(double scale) { scale_.x = scale; }
	void setScaleY(double scale) { scale_.y = scale; }
	void setScale(const Vec2d& scale) { scale_ = scale; }
	void setOffsetX(int16_t offset) { offset_x_ = offset; }
	void setOffsetY(int16_t offset) { offset_y_ = offset; }
	void setWorldPanning(bool wp) { world_panning_ = wp; }
	void setType(const string& type) { type_ = type; }
	void setExtended(bool ext) { extended_ = ext; }
	void setOptional(bool opt) { optional_ = opt; }
	void setNoDecals(bool nd) { no_decals_ = nd; }
	void setNullTexture(bool nt) { null_texture_ = nt; }
	void setState(uint8_t state) { state_ = state; }
	void setList(TextureXList* list) { in_list_ = list; }

	void clear();

	bool addPatch(const string& patch, int16_t offset_x = 0, int16_t offset_y = 0, int index = -1);
	bool removePatch(size_t index);
	bool removePatch(const string& patch);
	bool replacePatch(size_t index, const string& newpatch);
	bool duplicatePatch(size_t index, int16_t offset_x = 8, int16_t offset_y = 8);
	bool swapPatches(size_t p1, size_t p2);

	bool   parse(Tokenizer& tz, const string& type);
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
	string                name_;
	uint16_t              width_         = 0;
	uint16_t              height_        = 0;
	Vec2d                 scale_         = { 1., 1. };
	bool                  world_panning_ = false;
	vector<CTPatch::UPtr> patches_;
	int                   index_ = -1;

	// Extended (TEXTURES) info
	string  type_;
	bool    extended_     = false;
	bool    defined_      = false;
	bool    optional_     = false;
	bool    no_decals_    = false;
	bool    null_texture_ = false;
	int16_t offset_x_     = 0;
	int16_t offset_y_     = 0;
	int16_t def_width_    = 0;
	int16_t def_height_   = 0;

	// Editor info
	uint8_t       state_   = 0;
	TextureXList* in_list_ = nullptr;
};
