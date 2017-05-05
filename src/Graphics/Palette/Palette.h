
#ifndef __PALETTE_H__
#define	__PALETTE_H__

class Translation;

class Palette8bit
{
private:
	rgba_t	colours[256];
	hsl_t	colours_hsl[256];
	lab_t	colours_lab[256];
	short	index_trans;

	double	colourDiff(rgba_t& rgb, hsl_t& hsl, lab_t& lab, int index, int match);

public:
	enum PaletteFormats
	{
	    FORMAT_RAW,
	    FORMAT_IMAGE,
	    FORMAT_CSV,
	    FORMAT_JASC,
	    FORMAT_GIMP,
	};
	enum ColorMatching
	{
	    MATCH_DEFAULT,
	    MATCH_OLD,
	    MATCH_RGB,
	    MATCH_HSL,
	    MATCH_C76,
	    MATCH_C94,
	    MATCH_C2K,
	    MATCH_STOP,
	};

	Palette8bit();
	~Palette8bit();

	rgba_t	colour(uint8_t index) { return colours[index]; }
	short	transIndex() { return index_trans; }

	bool	loadMem(MemChunk& mc);
	bool	loadMem(const uint8_t* data, uint32_t size);
	bool	loadMem(MemChunk& mc, int format);
	bool	loadFile(string filename, int format = FORMAT_RAW);
	bool	saveMem(MemChunk& mc, int format = FORMAT_RAW, string name = "");
	bool	saveFile(string filename, int format = FORMAT_RAW);

	void	setColour (uint8_t index, rgba_t  col);
	void	setColourR(uint8_t index, uint8_t val);
	void	setColourG(uint8_t index, uint8_t val);
	void	setColourB(uint8_t index, uint8_t val);
	void	setColourA(uint8_t index, uint8_t val)	{ colours[index].a = val; }
	void	setTransIndex(short index)				{ index_trans = index; }
	
	void	copyPalette(Palette8bit* copy);
	short	findColour(rgba_t colour);
	short	nearestColour(rgba_t colour, int match = MATCH_DEFAULT);
	size_t	countColours();
	void	applyTranslation(Translation* trans);

	// Advanced palette modification
	void	colourise(rgba_t col, int start, int end);
	void	tint(rgba_t col, float amount, int start, int end);
	void	saturate(float amount, int start, int end);
	void	illuminate(float amount, int start, int end);
	void	shift(float amount, int start, int end);
	void	invert(int start, int end);
	void    setGradient(uint8_t startIndex, uint8_t endIndex, rgba_t startCol, rgba_t endCol);

	// For automated palette generation
	void	idtint(int r, int g, int b, int shift, int steps);

	typedef std::unique_ptr<Palette8bit> UPtr;
};

#endif //__PALETTE_H__
