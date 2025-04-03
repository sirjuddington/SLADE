#pragma once

namespace slade
{
class Translation;
class SIFormat;

class SImage
{
	friend class SIFormat;

public:
	enum class Type
	{
		PalMask,  // 2 bytes per pixel: palette index and alpha value
		RGBA,     // 4 bytes per pixel: RGBA
		AlphaMap, // 1 byte per pixel: alpha
		Unknown
	};

	enum class BlendType
	{
		Normal,          // Normal blend
		Add,             // Additive blend
		Subtract,        // Subtractive blend
		ReverseSubtract, // Reverse-subtractive blend
		Modulate,        // 'Modulate' blend
	};

	enum class AlphaSource
	{
		// Alpha map generation sources
		Brightness = 0,
		Alpha,
	};

	// Simple struct to hold pixel drawing properties
	struct DrawProps
	{
		BlendType blend; // The blending mode
		float     alpha;
		bool      src_alpha; // Whether to respect source pixel alpha

		DrawProps()
		{
			blend     = BlendType::Normal;
			alpha     = 1.0f;
			src_alpha = true;
		}
	};

	struct Info
	{
		int    width;
		int    height;
		Type   colformat;
		string format;
		int    numimages;
		int    imgindex;
		int    offset_x;
		int    offset_y;
		bool   has_palette;

		Info()
		{
			width = height = offset_x = offset_y = imgindex = 0;
			colformat                                       = Type::RGBA;
			numimages                                       = 1;
			has_palette                                     = false;
		}
	};

	SImage();
	SImage(Type type);
	SImage(const SImage& img);
	virtual ~SImage();

	bool isValid() const { return (width_ > 0 && height_ > 0 && data_.data()); }

	Type      type() const { return type_; }
	bool      putRGBAData(MemChunk& mc, const Palette* pal = nullptr) const;
	bool      putRGBData(MemChunk& mc, const Palette* pal = nullptr) const;
	bool      putIndexedData(MemChunk& mc) const;
	bool      putAlphaData(MemChunk& mc) const;
	int       width() const { return width_; }
	int       height() const { return height_; }
	int       index() const { return imgindex_; }
	int       size() const { return numimages_; }
	bool      hasPalette() const { return !!palette_; }
	Palette*  palette() const { return palette_.get(); }
	Vec2i     offset() const { return { offset_x_, offset_y_ }; }
	unsigned  stride() const;
	uint8_t   bpp() const;
	ColRGBA   pixelAt(unsigned x, unsigned y, const Palette* pal = nullptr) const;
	uint8_t   pixelIndexAt(unsigned x, unsigned y) const;
	SIFormat* format() const { return format_; }
	Info      info() const;

	void setXOffset(int offset);
	void setYOffset(int offset);
	void setOffsets(const Vec2i& offsets);
	void setPalette(const Palette* pal);

	void setWidth(int w);
	void setHeight(int h);

	// Misc
	void   clear();
	void   clear(Type new_type);
	void   create(int width, int height, Type type, const Palette* pal = nullptr, int index = 0, int numimages = 1);
	void   create(const Info& info, const Palette* pal = nullptr);
	void   fillAlpha(uint8_t alpha = 0);
	short  findUnusedColour(short preferred = -1) const;
	size_t countColours() const;
	void   shrinkPalette(Palette* pal = nullptr) const;
	bool   copyImage(const SImage* image);

	// Image format reading
	bool open(const MemChunk& data, int index = 0, string_view type_hint = "");
	bool loadFont0(const uint8_t* gfx_data, int size);
	bool loadFont1(const uint8_t* gfx_data, int size);
	bool loadFont2(const uint8_t* gfx_data, int size);
	bool loadFontM(const uint8_t* gfx_data, int size);
	bool loadBMF(const uint8_t* gfx_data, int size);
	bool loadWolfFont(const uint8_t* gfx_data, int size);
	bool loadJediFNT(const uint8_t* gfx_data, int size);
	bool loadJediFONT(const uint8_t* gfx_data, int size);
	bool loadJaguarSprite(const uint8_t* header, int hdr_size, const uint8_t* gfx_data, int size);
	bool loadJaguarTexture(const uint8_t* gfx_data, int size, int i_width, int i_height);

	// Conversion stuff
	bool convertRGBA(const Palette* pal = nullptr);
	bool convertPaletted(const Palette* pal_target, const Palette* pal_current = nullptr);
	bool convertAlphaMap(AlphaSource alpha_source = AlphaSource::Brightness, const Palette* pal = nullptr);
	bool maskFromColour(ColRGBA colour, const Palette* pal = nullptr);
	bool maskFromBrightness(const Palette* pal = nullptr);
	bool cutoffMask(uint8_t threshold);

	// Image modification
	bool setPixel(int x, int y, ColRGBA colour, const Palette* pal = nullptr);
	bool setPixel(int x, int y, uint8_t pal_index, uint8_t alpha = 255);
	bool imgconv();
	bool rotate(int angle);
	bool mirror(bool vert);
	bool crop(long x1, long y1, long x2, long y2);
	bool resize(int nwidth, int nheight);
	bool setImageData(const vector<uint8_t>& ndata, int nwidth, int nheight, Type ntype);
	bool setImageData(const uint8_t* ndata, unsigned ndata_size, int nwidth, int nheight, Type ntype);
	bool applyTranslation(const Translation* tr, const Palette* pal = nullptr, bool truecolor = false);
	bool applyTranslation(string_view tr, const Palette* pal = nullptr, bool truecolor = false);
	bool drawPixel(int x, int y, ColRGBA colour, const DrawProps& properties, const Palette* pal);
	bool drawImage(
		const SImage&    img,
		int              x,
		int              y,
		const DrawProps& properties,
		const Palette*   pal_src  = nullptr,
		const Palette*   pal_dest = nullptr);
	bool colourise(ColRGBA colour, const Palette* pal = nullptr, int start = -1, int stop = -1);
	bool tint(ColRGBA colour, float amount, const Palette* pal = nullptr, int start = -1, int stop = -1);
	bool adjust();
	bool mirrorpad();

	// Generation
	void generateCheckeredPattern(
		int            square_size,
		const ColRGBA& col1,
		const ColRGBA& col2,
		const Palette* pal = nullptr);

	// Signals
	struct Signals
	{
		sigslot::signal<int, int> offsets_changed;
		sigslot::signal<>         image_changed;
	};
	Signals& signals() { return signals_; }

private:
	int                 width_  = 0;
	int                 height_ = 0;
	MemChunk            data_;
	MemChunk            mask_;
	Type                type_ = Type::RGBA;
	unique_ptr<Palette> palette_;
	int                 offset_x_ = 0;
	int                 offset_y_ = 0;
	SIFormat*           format_   = nullptr;
	Signals             signals_;

	// For multi-image files
	int imgindex_  = 0;
	int numimages_ = 1;

	// Internal functions
	void           clearData(bool clear_mask = true);
	const Palette* paletteToUse(const Palette* pal) const;
	Palette*       paletteToUse(Palette* pal) const;
};
} // namespace slade
