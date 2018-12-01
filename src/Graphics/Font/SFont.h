#pragma once

#include "OpenGL/GLTexture.h"

class SFontChar
{
	friend class SFont;

public:
	SFontChar();
	~SFontChar();

private:
	uint16_t width_;
	uint16_t height_;
	Recti   tex_bounds_;
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

	SFont();
	~SFont();

	int lineHeight() { return line_height_; }

	// Font reading
	bool loadFont0(MemChunk& mc);
	bool loadFont1(MemChunk& mc);
	bool loadFont2(MemChunk& mc);
	bool loadFontM(MemChunk& mc);
	bool loadBMF(MemChunk& mc);

	// Rendering
	void drawCharacter(char c, ColRGBA colour = COL_WHITE);
	void drawString(string str, ColRGBA colour = COL_WHITE, Align align = Align::Left);

	// Static
	static SFont& vgaFont();
	static SFont& sladeFont();

private:
	SFontChar* characters_[256];
	GLTexture  texture_;
	int        line_height_;
	int        spacing_;

	// Global fonts
	static SFont font_vga_;
	static SFont font_slade_;
};
