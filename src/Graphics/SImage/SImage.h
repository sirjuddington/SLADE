
#ifndef __SIMAGE_H__
#define	__SIMAGE_H__

#include "Graphics/Palette/Palette.h"
#include "General/ListenerAnnouncer.h"

enum SIType
{
	PALMASK,	// 2 bytes per pixel: palette index and alpha value
	RGBA,		// 4 bytes per pixel: RGBA
	ALPHAMAP,	// 1 byte per pixel: alpha
};

enum SIBlendType
{
	NORMAL,				// Normal blend
	ADD,				// Additive blend
	SUBTRACT,			// Subtractive blend
	REVERSE_SUBTRACT,	// Reverse-subtractive blend
	MODULATE,			// 'Modulate' blend
};

// Simple struct to hold pixel drawing properties
struct si_drawprops_t
{
	SIBlendType	blend;		// The blending mode
	float		alpha;
	bool		src_alpha;	// Whether to respect source pixel alpha

	si_drawprops_t() { blend = NORMAL; alpha = 1.0f; src_alpha = true; }
};

class Translation;
class SIFormat;

class SImage : public Announcer
{
	friend class SIFormat;
private:
	int			width;
	int			height;
	uint8_t*	data;
	uint8_t*	mask;
	SIType		type;
	Palette8bit	palette;
	bool		has_palette;
	int			offset_x;
	int			offset_y;
	SIFormat*	format;

	// For multi-image files
	int			imgindex;
	int			numimages;

	// Internal functions
	void	clearData(bool clear_mask = true);

public:
	enum
	{
		// Alpha map generation sources
		BRIGHTNESS = 0,
		ALPHA,
	};

	struct info_t
	{
		int		width;
		int		height;
		int		colformat;
		string	format;
		int		numimages;
		int		imgindex;
		int		offset_x;
		int		offset_y;
		bool	has_palette;

		info_t()
		{
			width = height = offset_x = offset_y = imgindex = 0;
			colformat = RGBA;
			numimages = 1;
			has_palette = false;
		}
	};

	SImage(SIType type = RGBA);
	virtual ~SImage();

	bool			isValid() { return (width > 0 && height > 0 && data); }

	SIType			getType() { return type; }
	bool			getRGBAData(MemChunk& mc, Palette8bit* pal = NULL);
	bool			getRGBData(MemChunk& mc, Palette8bit* pal = NULL);
	bool			getIndexedData(MemChunk& mc);
	int				getWidth() { return width; }
	int				getHeight() { return height; }
	int				getIndex() { return imgindex; }
	int				getSize() { return numimages; }
	bool			hasPalette() { return has_palette; }
	Palette8bit*	getPalette() { return &palette; }
	point2_t		offset() { return point2_t(offset_x, offset_y); }
	unsigned		getStride();
	uint8_t			getBpp();
	rgba_t			getPixel(unsigned x, unsigned y, Palette8bit* pal = NULL);
	uint8_t			getPixelIndex(unsigned x, unsigned y);
	SIFormat*		getFormat() { return format; }
	info_t			getInfo();

	void			setXOffset(int offset);
	void			setYOffset(int offset);
	void			setPalette(Palette8bit* pal) { palette.copyPalette(pal); has_palette = true; }

	void			setWidth(int w);
	void			setHeight(int h);

	// Misc
	void	clear();
	void	create(int width, int height, SIType type, Palette8bit* pal = NULL, int index = 0, int numimages = 1);
	void	create(info_t info, Palette8bit* pal = NULL);
	void	fillAlpha(uint8_t alpha = 0);
	short	findUnusedColour();
	bool	validFlatSize();
	size_t	countColours();
	void	shrinkPalette(Palette8bit* pal = NULL);
	bool	copyImage(SImage* image);

	// Image format reading
	bool	open(MemChunk& data, int index = 0, string type_hint = "");
	bool	loadFont0(const uint8_t* gfx_data, int size);
	bool	loadFont1(const uint8_t* gfx_data, int size);
	bool	loadFont2(const uint8_t* gfx_data, int size);
	bool	loadFontM(const uint8_t* gfx_data, int size);
	bool	loadBMF(const uint8_t* gfx_data, int size);
	bool	loadWolfFont(const uint8_t* gfx_data, int size);
	bool	loadJediFNT(const uint8_t* gfx_data, int size);
	bool	loadJediFONT(const uint8_t* gfx_data, int size);
	bool	loadJaguarSprite(const uint8_t* header, int hdr_size, const uint8_t* gfx_data, int size);
	bool	loadJaguarTexture(const uint8_t* gfx_data, int size, int i_width, int i_height);

	// Conversion stuff
	bool	convertRGBA(Palette8bit* pal = NULL);
	bool	convertPaletted(Palette8bit* pal_target, Palette8bit* pal_current = NULL);
	bool	convertAlphaMap(int alpha_source = BRIGHTNESS, Palette8bit* pal = NULL);
	bool	maskFromColour(rgba_t colour, Palette8bit* pal = NULL);
	bool	maskFromBrightness(Palette8bit* pal = NULL);
	bool	cutoffMask(uint8_t threshold);

	// Image modification
	bool	setPixel(int x, int y, rgba_t colour, Palette8bit* pal = NULL);
	bool	setPixel(int x, int y, uint8_t pal_index, uint8_t alpha = 255);
	bool	imgconv();
	bool	rotate(int angle);
	bool	mirror(bool vert);
	bool	crop(long x1, long y1, long x2, long y2);
	bool	resize(int nwidth, int nheight);
	bool	setImageData(uint8_t* ndata, int nwidth, int nheight, SIType ntype);
	bool	applyTranslation(Translation* tr, Palette8bit* pal = NULL, bool truecolor = false);
	bool	applyTranslation(string tr, Palette8bit* pal = NULL);
	bool	drawPixel(int x, int y, rgba_t colour, si_drawprops_t& properties, Palette8bit* pal);
	bool	drawImage(SImage& img, int x, int y, si_drawprops_t& properties, Palette8bit* pal_src = NULL, Palette8bit* pal_dest = NULL);
	bool	colourise(rgba_t colour, Palette8bit* pal = NULL, int start = -1, int stop = -1);
	bool	tint(rgba_t colour, float amount, Palette8bit* pal = NULL, int start = -1, int stop = -1);
	bool	adjust();
	bool	mirrorpad();
};

#endif //__SIMAGE_H__
