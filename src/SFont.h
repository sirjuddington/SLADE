
#ifndef __SFONT_H__
#define __SFONT_H__

#include "GLTexture.h"

// Some defines
#define	SF_ALIGN_LEFT	0
#define SF_ALIGN_RIGHT	1
#define SF_ALIGN_CENTER	2

class SFontChar
{
	friend class SFont;
private:
	uint16_t	width;
	uint16_t	height;
	rect_t		tex_bounds;

public:
	SFontChar();
	~SFontChar();
};

class SFont
{
private:
	SFontChar*	characters[256];
	GLTexture	texture;
	int			line_height;
	int			spacing;

	// Global fonts
	static SFont	font_vga;
	static SFont	font_slade;

public:
	SFont();
	~SFont();

	int		lineHeight() { return line_height; }

	// Font reading
	bool	loadFont0(MemChunk& mc);
	bool	loadFont1(MemChunk& mc);
	bool	loadFont2(MemChunk& mc);
	bool	loadFontM(MemChunk& mc);
	bool	loadBMF(MemChunk& mc);

	// Rendering
	void	drawCharacter(char c, rgba_t colour = COL_WHITE);
	void	drawString(string str, rgba_t colour = COL_WHITE, uint8_t align = SF_ALIGN_LEFT);

	// Static
	static SFont&	vgaFont();
	static SFont&	sladeFont();
};

#endif//__SFONT_H__
