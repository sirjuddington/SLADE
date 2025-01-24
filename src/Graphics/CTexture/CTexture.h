#pragma once

namespace slade
{
class SImage;
class Tokenizer;
class Translation;
class TextureXList;

// Basic patch
class CTPatch
{
public:
	CTPatch() = default;
	CTPatch(string_view name, int16_t offset_x = 0, int16_t offset_y = 0);
	CTPatch(const CTPatch& copy) = default;
	virtual ~CTPatch();

	const string& name() const { return name_; }
	Vec2<int16_t> offset() const { return offset_; }
	int16_t       xOffset() const { return offset_.x; }
	int16_t       yOffset() const { return offset_.y; }

	void setName(string_view name) { name_ = name; }
	void setOffset(const Vec2<int16_t>& offset) { offset_ = offset; }
	void setOffsetX(int16_t offset) { offset_.x = offset; }
	void setOffsetY(int16_t offset) { offset_.y = offset; }

	virtual ArchiveEntry* patchEntry(Archive* parent = nullptr);

protected:
	string        name_;
	Vec2<int16_t> offset_ = { 0, 0 };
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

	enum class BlendType
	{
		None = 0,
		Translation,
		Blend,
		Tint
	};

	CTPatchEx() = default;
	CTPatchEx(string_view name, int16_t offset_x = 0, int16_t offset_y = 0, Type type = Type::Patch);
	CTPatchEx(const CTPatch& copy);
	CTPatchEx(const CTPatchEx& copy);
	~CTPatchEx() override;

	bool         flipX() const { return flip_x_; }
	bool         flipY() const { return flip_y_; }
	bool         useOffsets() const { return use_offsets_; }
	int16_t      rotation() const { return rotation_; }
	ColRGBA      colour() const { return colour_; }
	float        alpha() const { return alpha_; }
	string       style() const { return style_; }
	BlendType    blendType() const { return blendtype_; }
	Translation* translation() const { return translation_.get(); }

	void setFlipX(bool flip) { flip_x_ = flip; }
	void setFlipY(bool flip) { flip_y_ = flip; }
	void setUseOffsets(bool use) { use_offsets_ = use; }
	void setRotation(int16_t rot) { rotation_ = rot; }
	void setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { colour_.set(r, g, b, a); }
	void setColour(const ColRGBA& colour) { colour_ = colour; }
	void setAlpha(float a) { alpha_ = a; }
	void setStyle(string_view style) { style_ = style; }
	void setBlendType(BlendType type) { blendtype_ = type; }
	void setTranslation(const Translation& translation);

	bool hasTranslation() const;

	ArchiveEntry* patchEntry(Archive* parent = nullptr) override;

	bool   parse(Tokenizer& tz, Type type = Type::Patch);
	string asText();

private:
	Type                    type_        = Type::Patch;
	bool                    flip_x_      = false;
	bool                    flip_y_      = false;
	bool                    use_offsets_ = false;
	int16_t                 rotation_    = 0;
	unique_ptr<Translation> translation_;
	ColRGBA                 colour_;
	float                   alpha_     = 1.f;
	string                  style_     = "Copy";
	BlendType               blendtype_ = BlendType::None; // 0=none, 1=translation, 2=blend, 3=tint
};

class CTexture
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
	CTexture(string_view name, bool extended = false) : name_{ name }, extended_{ extended } {}
	~CTexture();

	void copyTexture(const CTexture& tex, bool keep_type = false);

	const vector<unique_ptr<CTPatch>>& patches() const { return patches_; }

	const string&  name() const { return name_; }
	Vec2<uint16_t> size() const { return size_; }
	uint16_t       width() const { return size_.x; }
	uint16_t       height() const { return size_.y; }
	double         scaleX() const { return scale_.x; }
	double         scaleY() const { return scale_.y; }
	Vec2d          scale() const { return scale_; }
	Vec2d          scaleFactor() const;
	int16_t        offsetX() const { return offset_.x; }
	int16_t        offsetY() const { return offset_.y; }
	bool           worldPanning() const { return world_panning_; }
	const string&  type() const { return type_; }
	bool           isExtended() const { return extended_; }
	bool           isOptional() const { return optional_; }
	bool           noDecals() const { return no_decals_; }
	bool           nullTexture() const { return null_texture_; }
	size_t         nPatches() const { return patches_.size(); }
	CTPatch*       patch(size_t index) const;
	uint8_t        state() const { return state_; }
	int            index() const;

	void setName(string_view name) { name_ = name; }
	void setSize(const Vec2<uint16_t>& size) { size_ = size; }
	void setWidth(uint16_t width) { size_.x = width; }
	void setHeight(uint16_t height) { size_.y = height; }
	void setScaleX(double scale) { scale_.x = scale; }
	void setScaleY(double scale) { scale_.y = scale; }
	void setScale(const Vec2d& scale) { scale_ = scale; }
	void setOffset(const Vec2<int16_t>& offset) { offset_ = offset; }
	void setOffsetX(int16_t offset) { offset_.x = offset; }
	void setOffsetY(int16_t offset) { offset_.y = offset; }
	void setWorldPanning(bool wp) { world_panning_ = wp; }
	void setType(string_view type) { type_ = type; }
	void setExtended(bool ext) { extended_ = ext; }
	void setOptional(bool opt) { optional_ = opt; }
	void setNoDecals(bool nd) { no_decals_ = nd; }
	void setNullTexture(bool nt) { null_texture_ = nt; }
	void setState(uint8_t state) { state_ = state; }
	void setList(TextureXList* list) { in_list_ = list; }

	void clear();

	bool addPatch(string_view patch, int16_t offset_x = 0, int16_t offset_y = 0, int index = -1);
	bool removePatch(size_t index);
	bool removePatch(string_view patch);
	bool replacePatch(size_t index, string_view newpatch);
	bool duplicatePatch(size_t index, int16_t offset_x = 8, int16_t offset_y = 8);
	bool swapPatches(size_t p1, size_t p2);

	bool   parse(Tokenizer& tz, string_view type);
	bool   parseDefine(Tokenizer& tz);
	string asText();

	bool convertExtended();
	bool convertRegular();
	bool loadPatchImage(
		unsigned       pindex,
		SImage&        image,
		Archive*       parent     = nullptr,
		const Palette* pal        = nullptr,
		bool           force_rgba = false) const;
	bool toImage(SImage& image, Archive* parent = nullptr, const Palette* pal = nullptr, bool force_rgba = false);

	// Signals
	struct Signals
	{
		sigslot::signal<CTexture&> patches_modified;
	};
	Signals& signals() { return signals_; }

private:
	// Basic info
	string                      name_;
	Vec2<uint16_t>              size_          = { 0, 0 };
	Vec2d                       scale_         = { 1., 1. };
	bool                        world_panning_ = false;
	vector<unique_ptr<CTPatch>> patches_;
	int                         index_ = -1;

	// Extended (TEXTURES) info
	string         type_         = "Texture";
	bool           extended_     = false;
	bool           defined_      = false;
	bool           optional_     = false;
	bool           no_decals_    = false;
	bool           null_texture_ = false;
	Vec2<int16_t>  offset_       = { 0, 0 };
	Vec2<uint16_t> def_size_     = { 0, 0 };

	// Editor info
	uint8_t       state_   = 0;
	TextureXList* in_list_ = nullptr;

	// Signals
	Signals signals_;
};
} // namespace slade
