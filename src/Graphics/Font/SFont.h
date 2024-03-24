#pragma once

#include "Utility/ColRGBA.h"
#include <array>

namespace slade
{
class SFontChar
{
	friend class SFont;

public:
	SFontChar()  = default;
	~SFontChar() = default;

private:
	uint16_t width_  = 0;
	uint16_t height_ = 0;
	Recti    tex_bounds_;
};

class SFont
{
public:
	enum class Align
	{
		Left,
		Right,
		Center
	};

	SFont()  = default;
	~SFont() = default;

	int lineHeight() const { return line_height_; }

	// Font reading
	bool loadFont0(MemChunk& mc);
	bool loadFont1(MemChunk& mc);
	bool loadFont2(MemChunk& mc);
	bool loadFontM(const MemChunk& mc);
	bool loadBMF(MemChunk& mc);

	// Rendering
	void drawCharacter(char c, ColRGBA colour = ColRGBA::WHITE) const;
	void drawString(string_view str, ColRGBA colour = ColRGBA::WHITE, Align align = Align::Left) const;

	// Static
	static SFont& vgaFont();
	static SFont& sladeFont();

private:
	std::array<SFontChar, 256> characters_;
	unsigned                   texture_;
	int                        line_height_;
	int                        spacing_;

	// Global fonts
	static SFont font_vga_;
	static SFont font_slade_;
};
} // namespace slade
