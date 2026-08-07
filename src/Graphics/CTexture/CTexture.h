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

	virtual bool isExtended() const { return false; }

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

	bool isExtended() const override { return true; }

	bool         flipX() const { return flip_x_; }
	bool         flipY() const { return flip_y_; }
	bool         useOffsets() const { return use_offsets_; }
	int16_t      rotation() const { return rotation_; }
	ColRGBA      colour() const { return colour_; }
	float        alpha() const { return alpha_; }
	string       style() const { return style_; }
	BlendType    blendType() const { return blendtype_; }
	Translation* translation() const { return translation_.get(); }
	float        tintAmount() const { return tint_amount_; }
	bool         hasFlag(string_view flag) const;

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
	void setTranslation(string_view trans_str);
	void setTintAmount(float amount) { tint_amount_ = amount; }
	void setFlag(string_view flag, bool on = true);

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
	float                   alpha_       = 1.f;
	string                  style_       = "Copy";
	BlendType               blendtype_   = BlendType::None; // 0=none, 1=translation, 2=blend, 3=tint
	float                   tint_amount_ = 0.f;
};

class CTexture
{
	friend class TextureXList;

public:
	enum class Type : u8
	{
		Texture = 0,
		Sprite,
		Graphic,
		WallTexture,
		Flat,
		HiRes
	};

	enum class State : u8
	{
		Unmodified = 0,
		Modified   = 1,
		New        = 2
	};

	enum class Flag : u8
	{
		WorldPanning = 1,
		Optional     = 2,
		NoDecals     = 4,
		NullTexture  = 8,
		NoTrim       = 16
	};

	CTexture(bool extended = false) : extended_{ extended } {}
	CTexture(string_view name, bool extended = false) : name_{ name }, extended_{ extended } {}
	~CTexture();

	void copyTexture(const CTexture& tex, bool keep_type = false, bool patches = true);

	const vector<unique_ptr<CTPatch>>& patches() const { return patches_; }

	const string&    name() const { return name_; }
	Vec2<uint16_t>   size() const { return size_; }
	uint16_t         width() const { return size_.x; }
	uint16_t         height() const { return size_.y; }
	double           scaleX() const { return scale_.x; }
	double           scaleY() const { return scale_.y; }
	Vec2d            scale() const { return scale_; }
	Vec2d            scaleFactor() const;
	int16_t          offsetX() const { return offset_.x; }
	int16_t          offsetY() const { return offset_.y; }
	const Vec2<i16>& offset() const { return offset_; }
	bool             worldPanning() const { return flags_ & static_cast<u8>(Flag::WorldPanning); }
	const string&    type() const { return type_; }
	Type             typeEnum() const;
	bool             isExtended() const { return extended_; }
	bool             isOptional() const { return flags_ & static_cast<u8>(Flag::Optional); }
	bool             noDecals() const { return flags_ & static_cast<u8>(Flag::NoDecals); }
	bool             nullTexture() const { return flags_ & static_cast<u8>(Flag::NullTexture); }
	bool             noTrim() const { return flags_ & static_cast<u8>(Flag::NoTrim); }
	size_t           nPatches() const { return patches_.size(); }
	CTPatch*         patch(size_t index) const;
	State            state() const { return state_; }
	int              index() const;
	int              patchIndex(const CTPatch* patch) const;
	TextureXList*    list() const { return in_list_; }

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
	void setWorldPanning(bool wp);
	void setType(string_view type) { type_ = type; }
	void setType(Type type);
	void setExtended(bool ext) { extended_ = ext; }
	void setOptional(bool opt);
	void setNoDecals(bool nd);
	void setNullTexture(bool nt);
	void setNoTrim(bool nt);
	void setState(State state);
	void setList(TextureXList* list) { in_list_ = list; }
	bool setFlag(Flag flag, bool on = true);

	void clear();

	bool addPatch(string_view patch, int16_t offset_x = 0, int16_t offset_y = 0, int index = -1);
	bool removePatch(size_t index);
	bool removePatches(const vector<unsigned>& indices);
	bool removePatch(string_view patch);
	bool replacePatch(size_t index, string_view newpatch) const;
	bool replacePatches(const vector<unsigned>& indices, string_view newpatch) const;
	bool duplicatePatch(size_t index, int16_t offset_x = 8, int16_t offset_y = 8);
	bool duplicatePatches(const vector<unsigned>& indices, int16_t offset_x = 8, int16_t offset_y = 8);
	bool swapPatches(size_t p1, size_t p2);
	bool replacePatches(vector<unique_ptr<CTPatch>>& new_patches);

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
	bool toImage(
		SImage&        image,
		Archive*       parent     = nullptr,
		const Palette* pal        = nullptr,
		bool           force_rgba = false,
		bool           offsets    = true) const;

private:
	// Basic info
	string                      name_;
	Vec2<uint16_t>              size_  = { 0, 0 };
	Vec2d                       scale_ = { 1., 1. };
	u8                          flags_ = 0;
	vector<unique_ptr<CTPatch>> patches_;
	int                         index_ = -1;

	// Extended (TEXTURES) info
	string         type_     = "Texture";
	bool           extended_ = false;
	bool           defined_  = false;
	Vec2<int16_t>  offset_   = { 0, 0 };
	Vec2<uint16_t> def_size_ = { 0, 0 };

	// Editor info
	State         state_   = State::Unmodified;
	TextureXList* in_list_ = nullptr;
};
} // namespace slade
