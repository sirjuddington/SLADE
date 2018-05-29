#pragma once

class Translation;

class Palette
{
public:
	enum class Format
	{
	    Raw,
	    Image,
	    CSV,
	    JASC,
	    GIMP,
	};
	enum class ColourMatch
	{
	    Default,
	    Old,
	    RGB,
	    HSL,
	    C76,
	    C94,
	    C2K,
	    Stop,
	};

	Palette(unsigned size = 256);
	~Palette();

	rgba_t	colour(uint8_t index) { return colours_[index]; }
	short	transIndex() { return index_trans_; }

	bool	loadMem(MemChunk& mc);
	bool	loadMem(const uint8_t* data, uint32_t size);
	bool	loadMem(MemChunk& mc, Format format);
	bool	loadFile(const string& filename, Format format = Format::Raw);
	bool	saveMem(MemChunk& mc, Format format = Format::Raw, const string& name = "");
	bool	saveFile(const string& filename, Format format = Format::Raw);

	void	setColour (uint8_t index, rgba_t  col);
	void	setColourR(uint8_t index, uint8_t val);
	void	setColourG(uint8_t index, uint8_t val);
	void	setColourB(uint8_t index, uint8_t val);
	void	setColourA(uint8_t index, uint8_t val)	{ colours_[index].a = val; }
	void	setTransIndex(short index)				{ index_trans_ = index; }
	
	void	copyPalette(Palette* copy);
	short	findColour(rgba_t colour);
	short	nearestColour(rgba_t colour, ColourMatch match = ColourMatch::Default);
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

	typedef std::unique_ptr<Palette> UPtr;

private:
	vector<rgba_t>	colours_;
	vector<hsl_t>	colours_hsl_;
	vector<lab_t>	colours_lab_;
	short			index_trans_;

	double	colourDiff(rgba_t& rgb, hsl_t& hsl, lab_t& lab, int index, ColourMatch match);
};
