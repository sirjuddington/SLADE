#pragma once

#include "General/ListenerAnnouncer.h"
#include "Graphics/Palette/Palette.h"

class Translation;
class SIFormat;

class SImage : public Announcer
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

	SImage(Type type = Type::RGBA);
	virtual ~SImage();

	bool isValid() { return (width_ > 0 && height_ > 0 && data_); }

	Type      type() { return type_; }
	bool      putRGBAData(MemChunk& mc, Palette* pal = nullptr);
	bool      putRGBData(MemChunk& mc, Palette* pal = nullptr);
	bool      putIndexedData(MemChunk& mc);
	int       width() { return width_; }
	int       height() { return height_; }
	int       index() { return imgindex_; }
	int       size() { return numimages_; }
	bool      hasPalette() { return has_palette_; }
	Palette*  palette() { return &palette_; }
	Vec2i     offset() { return Vec2i(offset_x_, offset_y_); }
	unsigned  stride();
	uint8_t   bpp();
	ColRGBA   pixelAt(unsigned x, unsigned y, Palette* pal = nullptr);
	uint8_t   pixelIndexAt(unsigned x, unsigned y);
	SIFormat* format() { return format_; }
	Info      info();

	void setXOffset(int offset);
	void setYOffset(int offset);
	void setPalette(Palette* pal)
	{
		palette_.copyPalette(pal);
		has_palette_ = true;
	}

	void setWidth(int w);
	void setHeight(int h);

	// Misc
	void   clear();
	void   create(int width, int height, Type type, Palette* pal = nullptr, int index = 0, int numimages = 1);
	void   create(Info info, Palette* pal = nullptr);
	void   fillAlpha(uint8_t alpha = 0);
	short  findUnusedColour();
	bool   validFlatSize();
	size_t countColours();
	void   shrinkPalette(Palette* pal = nullptr);
	bool   copyImage(SImage* image);

	// Image format reading
	bool open(MemChunk& data, int index = 0, string type_hint = "");
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
	bool convertRGBA(Palette* pal = nullptr);
	bool convertPaletted(Palette* pal_target, Palette* pal_current = nullptr);
	bool convertAlphaMap(AlphaSource alpha_source = AlphaSource::Brightness, Palette* pal = nullptr);
	bool maskFromColour(ColRGBA colour, Palette* pal = nullptr);
	bool maskFromBrightness(Palette* pal = nullptr);
	bool cutoffMask(uint8_t threshold);

	// Image modification
	bool setPixel(int x, int y, ColRGBA colour, Palette* pal = nullptr);
	bool setPixel(int x, int y, uint8_t pal_index, uint8_t alpha = 255);
	bool imgconv();
	bool rotate(int angle);
	bool mirror(bool vert);
	bool crop(long x1, long y1, long x2, long y2);
	bool resize(int nwidth, int nheight);
	bool setImageData(uint8_t* ndata, int nwidth, int nheight, Type ntype);
	bool applyTranslation(Translation* tr, Palette* pal = nullptr, bool truecolor = false);
	bool applyTranslation(string tr, Palette* pal = nullptr, bool truecolor = false);
	bool drawPixel(int x, int y, ColRGBA colour, DrawProps& properties, Palette* pal);
	bool drawImage(
		SImage&    img,
		int        x,
		int        y,
		DrawProps& properties,
		Palette*   pal_src  = nullptr,
		Palette*   pal_dest = nullptr);
	bool colourise(ColRGBA colour, Palette* pal = nullptr, int start = -1, int stop = -1);
	bool tint(ColRGBA colour, float amount, Palette* pal = nullptr, int start = -1, int stop = -1);
	bool adjust();
	bool mirrorpad();

private:
	int       width_;
	int       height_;
	uint8_t*  data_;
	uint8_t*  mask_;
	Type      type_;
	Palette   palette_;
	bool      has_palette_;
	int       offset_x_;
	int       offset_y_;
	SIFormat* format_;

	// For multi-image files
	int imgindex_;
	int numimages_;

	// Internal functions
	void clearData(bool clear_mask = true);
};
