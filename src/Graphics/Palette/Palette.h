#pragma once
#include "Utility/Colour.h"

namespace slade
{
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
	Palette(const Palette& pal) : Palette(pal.colours_.size()) { copyPalette(&pal); }
	~Palette() = default;

	const vector<ColRGBA>& colours() const { return colours_; }
	ColRGBA                colour(uint8_t index) const { return colours_[index]; }
	short                  transIndex() const { return index_trans_; }

	bool loadMem(const MemChunk& mc);
	bool loadMem(const uint8_t* data, uint32_t size);
	bool loadMem(MemChunk& mc, Format format);
	bool loadFile(string_view filename, Format format = Format::Raw);
	bool saveMem(MemChunk& mc, Format format = Format::Raw, string_view name = "");
	bool saveFile(string_view filename, Format format = Format::Raw);

	void setColour(uint8_t index, const ColRGBA& col);
	void setColourR(uint8_t index, uint8_t val);
	void setColourG(uint8_t index, uint8_t val);
	void setColourB(uint8_t index, uint8_t val);
	void setColourA(uint8_t index, uint8_t val) { colours_[index].a = val; }
	void setTransIndex(short index) { index_trans_ = index; }

	void   copyPalette(const Palette* copy);
	short  findColour(const ColRGBA& colour) const;
	short  nearestColour(const ColRGBA& colour, ColourMatch match = ColourMatch::Default) const;
	size_t countColours() const;
	void   applyTranslation(Translation* trans);

	// Advanced palette modification
	void colourise(const ColRGBA& col, int start, int end);
	void tint(const ColRGBA& col, float amount, int start, int end);
	void saturate(float amount, int start, int end);
	void illuminate(float amount, int start, int end);
	void shift(float amount, int start, int end);
	void invert(int start, int end);
	void setGradient(uint8_t startIndex, uint8_t endIndex, const ColRGBA& startCol, const ColRGBA& endCol);

	// For automated palette generation
	void idtint(int r, int g, int b, int shift, int steps);

private:
	vector<ColRGBA> colours_;
	vector<ColHSL>  colours_hsl_;
	vector<ColLAB>  colours_lab_;
	short           index_trans_;

	double colourDiff(const ColRGBA& rgb, const ColHSL& hsl, const ColLAB& lab, int index, ColourMatch match) const;
};
} // namespace slade
