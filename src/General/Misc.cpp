
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Misc.cpp
 * Description: Misc functions that don't necessarily belong
 *              anywhere else (generally stuff that involves multiple
 *              unrelated classes)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "General/Misc.h"
#include "Graphics/SImage/SImage.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryDataFormat.h"
#include "Archive/Formats/WadArchive.h"
#include "Archive/Formats/ZipArchive.h"
#include "General/Console/Console.h"
#include "Graphics/SImage/SIFormat.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include "External/zlib/zlib.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, size_as_string, true, CVAR_SAVE)
CVAR(Bool, percent_encoding, false, CVAR_SAVE)
EXTERN_CVAR(Float, col_cie_tristim_x)
EXTERN_CVAR(Float, col_cie_tristim_z)
namespace Misc
{
	vector<winf_t>	window_info;
}


/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

/* Misc::loadImageFromEntry
 * Loads an image from <entry> into <image>. Returns false if the
 * given entry wasn't a valid image, true otherwise
 *******************************************************************/
bool Misc::loadImageFromEntry(SImage* image, ArchiveEntry* entry, int index)
{
	if (!entry)
		return false;

	// Detect entry type if it isn't already
	if (entry->getType() == EntryType::unknownType())
		EntryType::detectEntryType(entry);

	// Check for format "image" property
	if (!entry->getType()->extraProps().propertyExists("image"))
	{
		Global::error = "Entry type is not a valid image";
		return false;
	}

	// Get image format hint from type, if any
	string format_hint = "";
	if (entry->getType()->extraProps().propertyExists("image_format"))
		format_hint = entry->getType()->extraProps()["image_format"].getStringValue();

	// Font formats are still manually loaded for now
	string format = entry->getType()->getFormat();
	if (S_CMPNOCASE(format, "font_doom_alpha"))
		return image->loadFont0(entry->getData(), entry->getSize());
	else if (S_CMPNOCASE(format, "font_zd_console"))
		return image->loadFont1(entry->getData(), entry->getSize());
	else if (S_CMPNOCASE(format, "font_zd_big"))
		return image->loadFont2(entry->getData(), entry->getSize());
	else if (S_CMPNOCASE(format, "font_bmf"))
		return image->loadBMF(entry->getData(), entry->getSize());
	else if (S_CMPNOCASE(format, "font_mono"))
		return image->loadFontM(entry->getData(), entry->getSize());
	else if (S_CMPNOCASE(format, "font_wolf"))
		return image->loadWolfFont(entry->getData(), entry->getSize());
	else if (S_CMPNOCASE(format, "font_jedi_fnt"))
		return image->loadJediFNT(entry->getData(), entry->getSize());
	else if (S_CMPNOCASE(format, "font_jedi_font"))
		return image->loadJediFONT(entry->getData(), entry->getSize());
	// Jaguar Doom sprite and texture formats are a bit complicated, so
	// they need manual loading as well rather than the SIFormat system
	else if (S_CMPNOCASE(format, "img_jaguar_sprite"))
	{
		Archive* parent = entry->getParent(); if (parent == NULL) return false;
		ArchiveEntry* data = parent->getEntry(parent->entryIndex(entry) + 1);
		if (data && S_CMPNOCASE(data->getName(), "."))
			return image->loadJaguarSprite(entry->getData(), entry->getSize(), data->getData(), data->getSize());
		else return false;
	}
	else if (S_CMPNOCASE(format, "img_jaguar_texture"))
	{
		Archive* parent = entry->getParent(); if (parent == NULL) return false;
		ArchiveEntry* texture1 = parent->getEntry("TEXTURE1"); if (texture1 == NULL) return false;
		point2_t dimensions = findJaguarTextureDimensions(texture1, entry->getName(true));
		return image->loadJaguarTexture(entry->getData(), entry->getSize(), dimensions.x, dimensions.y);
	}

	// Firstly try SIFormat system
	if (image->open(entry->getMCData(), index, format_hint))
		return true;

	// Raw images are a special case (not reliably possible to detect just from data)
	if (format == "img_raw" && SIFormat::rawFormat()->isThisFormat(entry->getMCData()))
		return SIFormat::rawFormat()->loadImage(*image, entry->getMCData());

	// Lastly, try detecting/loading via FreeImage
	else if (SIFormat::generalFormat()->isThisFormat(entry->getMCData()))
		return SIFormat::generalFormat()->loadImage(*image, entry->getMCData());

	// Unknown image type
	Global::error = "Entry is not a known image format";
	return false;
}

/* Misc::detectPaletteHack
 * Detects the few known cases where a picture does not use PLAYPAL
 * as its default palette.
 *******************************************************************/
int	Misc::detectPaletteHack(ArchiveEntry* entry)
{
	if (entry == NULL || entry->getType() == NULL)
		return PAL_NOHACK;
	else if (entry->getType()->getFormat() == "img_doom_arah"	&& entry->getName() == "TITLEPIC")
		return PAL_ALPHAHACK;	// Doom Alpha 0.2
	else if (entry->getType()->getFormat() == "img_doom_snea"	&& entry->getName() == "TITLEPIC")
		return PAL_ALPHAHACK;	// Doom Alpha 0.4 and 0.5
	else if (entry->getType()->getFormat() == "img_raw"			&& entry->getName() == "E2END")
		return PAL_HERETICHACK;	// Heretic
	else if (entry->getType()->getFormat() == "img_doom_arah"	&& entry->getName() == "shadowpage")
		return PAL_SHADOWHACK;	// Shadowcaster
	else if (entry->getType()->getFormat() == "img_rott"		&& entry->getName() == "NICOLAS")
		return PAL_ROTTNHACK;	// Rise of the Triad
	else if (entry->getType()->getFormat() == "img_rott"		&& entry->getName() == "FINLDOOR")
		return PAL_ROTTDHACK;	// Rise of the Triad
	else if (entry->getType()->getFormat() == "img_rott"		&& entry->getName() == "FINLFIRE")
		return PAL_ROTTFHACK;	// Rise of the Triad
	else if ((entry->getType()->getFormat() == "img_rott"		&& entry->getName() == "AP_TITL")
	      || (entry->getType()->getFormat() == "img_rottraw"	&& entry->getName() == "AP_WRLD"))
		return PAL_ROTTAHACK;	// Rise of the Triad
	else if (entry->getType()->getFormat() == "img_wolfpic"		&& entry->getName().Matches("IDG*"))
		return PAL_SODIDHACK;	// Spear of Destiny team screens
	else if (entry->getType()->getFormat() == "img_wolfpic"		&& entry->getName().Matches("TIT*"))
		return PAL_SODTITLEHACK;// Spear of Destiny title screens
	else if (entry->getType()->getFormat() == "img_wolfpic"		&& entry->getName().Matches("END*"))
	{
		long endscreen;			// Spear of Destiny ending screens (extra-hacky!)
		if (entry->getName().Right(3).ToLong(&endscreen))
			return PAL_SODENDHACK + endscreen - 81;
	}

	// Default:
	return PAL_NOHACK;
}


/* Misc::loadPaletteFromArchive
 * Writes palette information from the PLAYPAL entry in <archive> to
 * <pal>. Returns false if PLAYPAL entry was missing or invalid,
 * true otherwise
 *******************************************************************/
bool Misc::loadPaletteFromArchive(Palette8bit* pal, Archive* archive, int lump)
{
	// Check parameters
	if (!pal || !archive)
		return false;

	// Find PLAYPAL entry
	bool sixbit = false;
	ArchiveEntry* playpal = NULL;
	if (lump == PAL_ALPHAHACK)
		playpal = archive->getEntry("TITLEPAL", true);
	else if (lump == PAL_HERETICHACK)
		playpal = archive->getEntry("E2PAL", true);
	else if (lump == PAL_SHADOWHACK)
		playpal = archive->getEntry("shadowpage+1", true), sixbit = true;
	else if (lump == PAL_ROTTNHACK)
		playpal = archive->getEntry("NICPAL", true);
	else if (lump == PAL_ROTTDHACK)
		playpal = archive->getEntry("FINDRPAL", true);
	else if (lump == PAL_ROTTFHACK)
		playpal = archive->getEntry("FINFRPAL", true);
	else if (lump == PAL_ROTTAHACK)
		playpal = archive->getEntry("AP_PAL", true);
	else if (lump == PAL_SODIDHACK)
		playpal = archive->getEntry("PAL00163", true), sixbit = true;
	else if (lump == PAL_SODTITLEHACK)
		playpal = archive->getEntry("PAL00153", true), sixbit = true;
	else if (lump >= PAL_SODENDHACK)
	{
		int endscreen = lump - PAL_SODENDHACK;
		endscreen += 154;
		string palname = S_FMT("PAL%05d", endscreen);
		playpal = archive->getEntry(palname, true);
		sixbit = true;
	}
	if (!playpal || playpal->getSize() < 768)
	{
		// Search archive for any palette
		Archive::search_options_t opt;

		// Search "PLAYPAL" first
		opt.match_type = EntryType::getType("palette");
		opt.match_name = "PLAYPAL";
		opt.search_subdirs = true;
		playpal = archive->findFirst(opt);

		// Otherwise any palette will do
		if (!playpal)
		{
			opt.match_name = "";
			playpal = archive->findFirst(opt);
		}
	}
	if (!playpal || playpal->getSize() < 768)
		playpal = archive->getEntry("PAL", true);

	// Check it was found
	if (!playpal || playpal->getSize() < 768)
		return false;

	// Check it is the correct size
	if (playpal->getSize() < 256*3)
		return false;

	// Read palette colours
	const uint8_t* playpal_dat = playpal->getData(true);
	uint8_t max = 0;
	for (int a = 0; a < 768; ++a)
		if (playpal_dat[a] > max)
			max = playpal_dat[a];
	if (max < 64) sixbit = true;

	int c = 0;
	for (int a = 0; a < 256; ++a)
	{
		uint8_t r = playpal_dat[c++];
		uint8_t g = playpal_dat[c++];
		uint8_t b = playpal_dat[c++];
		if (sixbit)
		{
			r = (r<<2) | (r>>4);
			g = (g<<2) | (g>>4);
			b = (b<<2) | (b>>4);
		}
		pal->setColour(a, rgba_t(r, g,  b, 255));
	}

	return true;
}

/* Misc::sizeAsString
 * Converts <size> to a string representing it as a 'bytes' size, ie
 * "1.24kb", "4.00mb". Sizes under 1kb aren't given an appendage
 *******************************************************************/
string Misc::sizeAsString(uint32_t size)
{
	if (size < 1024 || !size_as_string)
	{
		return S_FMT("%d", size);
	}
	else if (size < 1024*1024)
	{
		double kb = (double)size / 1024;
		return S_FMT("%1.2fkb", kb);
	}
	else
	{
		double mb = (double)size / (1024*1024);
		return S_FMT("%1.2fmb", mb);
	}
}

/* Misc::lumpNameToFileName
 * Sanitizes a wad lump name for exporting as a file name.
 * ZDoom merely substitutes \ to ^, but Doomsday requires
 * percent encoding of every non-alphanumeric character.
 *******************************************************************/
string Misc::lumpNameToFileName(string lump)
{
	if (percent_encoding)
	{
		// Doomsday: everything but [a-zA-Z0-9._ ~-]
		string file;
		int chr;
		for (size_t a = 0; a < lump.Len(); ++a)
		{
			chr = lump[a];
			if ((chr < 'a' || chr > 'z') && (chr < 'A' || chr > 'Z') && (chr < '0' || chr > '9')
			        && chr != '-' && chr != '.' && chr != '_' && chr != '~')
			{
				file += S_FMT("%%%02X", chr);
			}
			else file += S_FMT("%c", chr);
		}
		return file;
	}
	else
	{
		// ZDoom
		lump.Replace(StringUtils::SLASH_BACK, StringUtils::CARET);
		lump.Replace(StringUtils::SLASH_FORWARD, StringUtils::CARET);
	}
	return lump;
}

/* Misc::fileNameToLumpName
 * Turns a file name into a lump name
 *******************************************************************/
string Misc::fileNameToLumpName(string file)
{
	if (percent_encoding)
	{
		string lump;
		for (size_t a = 0; a < file.Len(); ++a)
		{
			if (file[a] == '%' && file.Len() > a+2)
			{
				string code = file.Mid(a+1, 2);
				unsigned long percent;
				if (!code.ToULong(&percent, 16)) percent = 0;
				lump += S_FMT("%c", percent);
				a+=2;
			}
			else lump += S_FMT("%c", file[a]);
		}
		return lump;
	}
	else
	{
		// ZDoom
		file.Replace(StringUtils::CARET, StringUtils::SLASH_BACK);
	}
	return file;
}

/* Misc::massRenameFilter
 * Creates a mass rename filter string from [names]
 *******************************************************************/
string Misc::massRenameFilter(wxArrayString& names)
{
	// Check any names were given
	if (names.size() == 0)
		return "";

	// Init filter string
	string filter = names[0];

	// Go through names
	for (unsigned a = 1; a < names.size(); a++)
	{
		// If the filter string is shorter than this name, extend it with *s
		while (filter.size() < names[a].size())
			filter += '*';

		// Check each character
		for (unsigned c = 0; c < names[a].size(); c++)
		{
			// Skip if current filter character is *
			if (filter[c] == '*')
				continue;

			// Check for character mismatch
			if (filter[c] != names[a][c])
				filter[c] = '*';
		}
	}

	return filter;
}

/* Misc::doMassRename
 * Performs a mass rename on [names] using the filter [name_filter].
 * Any * in the filter means that character should not be changed
 *******************************************************************/
void Misc::doMassRename(wxArrayString& names, string name_filter)
{
	// Go through names
	for (unsigned a = 0; a < names.size(); a++)
	{
		string& name = names[a];

		// If this name is shorter than the filter string, extend it with spaces
		//while (name.size() < name_filter.size())
		//	name += " ";

		// If the filter string is shorter than the name, just truncate the name
		if (name_filter.size() < name.size())
			name.Truncate(name_filter.size());

		// Go through filter characters
		for (unsigned c = 0; c < name_filter.size(); c++)
		{
			// Check character
			if (name_filter[c] == '*')
				continue;					// Skip if *
			else
			{
				// First check that we aren't past the end of the name
				if (c >= name.size())
				{
					// If we are, pad it with spaces
					while (name.size() <= c)
						name += " ";
				}

				// Replace character
				name[c] = name_filter[c];
			}
		}
	}
}

/* Misc::rgbToHsl
 * Converts a colour from RGB to HSL colourspace
 *******************************************************************/
hsl_t Misc::rgbToHsl(double r, double g, double b)
{
	hsl_t ret;
	double v_min = MIN(r, MIN(g, b));
	double v_max = MAX(r, MAX(g, b));
	double delta = v_max - v_min;

	// Determine V
	ret.l = (v_max + v_min) * 0.5;

	if (delta == 0)
		ret.h = ret.s = 0;		// Grey (r==g==b)
	else
	{
		// Determine S
		if (ret.l < 0.5)
			ret.s = delta / (v_max + v_min);
		else
			ret.s = delta / (2.0 - v_max - v_min);

		// Determine H
		if (r == v_max)
			ret.h = (g - b) / delta;
		else if (g == v_max)
			ret.h = 2.0 + (b - r) / delta;
		else if (b == v_max)
			ret.h = 4.0 + (r - g) / delta;

		ret.h /= 6.0;

		if (ret.h < 0)
			ret.h += 1.0;
	}

	return ret;
}
hsl_t Misc::rgbToHsl(rgba_t rgba)
{
	return Misc::rgbToHsl(rgba.dr(), rgba.dg(), rgba.db());
}

/* Misc::hslToRgb
 * Converts a colour from HSL to RGB colourspace
 *******************************************************************/
rgba_t Misc::hslToRgb(double h, double s, double l)
{
	rgba_t ret(0, 0, 0, 255, -1);

	// No saturation means grey
	if (s == 0.)
	{
		ret.r = ret.g = ret.b = (uint8_t) (255. * l);
		return ret;
	}

	// Find the rough values at given H with mid L and max S.
	double hue = (6. * h);
	uint8_t sector = (uint8_t) hue;
	double factor = hue - sector;
	double dr, dg, db;
	switch (sector)
	{
		// RGB 0xFF0000 to 0xFFFF00, increasingly green
	case 0: dr = 1.; dg = factor; db = 0.; break;
		// RGB 0xFFFF00 to 0x00FF00, decreasingly red
	case 1: dr = 1. - factor; dg = 1.; db = 0.; break;
		// RGB 0x00FF00 to 0x00FFFF, increasingly blue
	case 2: dr = 0.; dg = 1.; db = factor; break;
		// RGB 0x00FFFF to 0x0000FF, decreasingly green
	case 3: dr = 0.; dg = 1. - factor; db = 1.; break;
		// RGB 0x0000FF to 0xFF00FF, increasingly red
	case 4: dr = factor; dg = 0.; db = 1.; break;
		// RGB 0xFF00FF to 0xFF0000, decreasingly blue
	case 5: dr = 1.; dg = 0.; db = 1. - factor; break;
	}

	// Now apply desaturation
	double ds = (1. - s) * 0.5;
	dr = ds + (dr * s);
	dg = ds + (dg * s);
	db = ds + (db * s);

	// Finally apply luminosity
	double dl = l * 2.;
	double sr, sg, sb, sl;
	if (dl > 1.)
	{
		// Make brighter
		sl = dl - 1.;
		sr = sl * (1. - dr); dr += sr;
		sg = sl * (1. - dg); dg += sg;
		sb = sl * (1. - db); db += sb;
	}
	else if (dl < 1.)
	{
		// Make darker
		sl = 1. - dl;
		sr = sl * dr; dr -= sr;
		sg = sl * dg; dg -= sg;
		sb = sl * db; db -= sb;
	}

	// Clamping (shouldn't actually be needed)
	if (dr > 1.) dr = 1.; if (dr < 0.) dr = 0.;
	if (dg > 1.) dg = 1.; if (dg < 0.) dg = 0.;
	if (db > 1.) db = 1.; if (db < 0.) db = 0.;

	// Now convert from 0f--1f to 0i--255i, rounding up
	ret.r = (uint8_t) (dr * 255. + 0.499999999);
	ret.g = (uint8_t) (dg * 255. + 0.499999999);
	ret.b = (uint8_t) (db * 255. + 0.499999999);

	return ret;
}
rgba_t Misc::hslToRgb(hsl_t hsl)
{
	return Misc::hslToRgb(hsl.h, hsl.s, hsl.l);
}

/* Misc::rgbToLab
 * Converts a colour from RGB to CIE-L*a*b colourspace
 * Conversion formulas lazily taken from easyrgb.com.
 *******************************************************************/
#define NORMALIZERGB(a) a = 100 * ((a > 0.04045) ? (pow(((a + 0.055) / 1.055), 2.4)) : (a / 12.92))
#define NORMALIZEXYZ(a) a = ((a > 0.008856) ? (pow(a, (1.0/3.0))) : ((7.787*a)+(16.0/116.0)))
lab_t Misc::rgbToLab(double r, double g, double b)
{
	double x, y, z;
	lab_t ret;

	// Step #1: convert RGB to CIE-XYZ
	NORMALIZERGB(r);
	NORMALIZERGB(g);
	NORMALIZERGB(b);

	x = (r * 0.4124 + g * 0.3576 + b * 0.1805) / col_cie_tristim_x;
	y = (r * 0.2126 + g * 0.7152 + b * 0.0722) / 100.000;			// y is always 100.00
	z = (r * 0.0193 + g * 0.1192 + b * 0.9505) / col_cie_tristim_z;

	// Step #2: convert xyz to lab
	NORMALIZEXYZ(x);
	NORMALIZEXYZ(y);
	NORMALIZEXYZ(z);

	ret.l = (116.0 * y) - 16;
	ret.a = 500.0 * (x - y);
	ret.b = 200.0 * (y - z);

	return ret;
}
lab_t Misc::rgbToLab(rgba_t rgba)
{
	return Misc::rgbToLab(rgba.dr(), rgba.dg(), rgba.db());
}
#undef NORMALIZERGB
#undef NORMALIZEXYZ

// CRC-32 stuff

/* Table of CRCs of all 8-bit messages. */
uint32_t crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void)
{
	uint32_t c;
	int n, k;

	for (n = 0; n < 256; n++)
	{
		c = (uint32_t) n;

		for (k = 0; k < 8; k++)
		{
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}

		crc_table[n] = c;
	}

	crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
should be initialized to all 1's, and the transmitted value
is the 1's complement of the final running CRC (see the
crc() routine below)). */
uint32_t update_crc(uint32_t crc, const uint8_t* buf, uint32_t len)
{
	uint32_t c = crc;

	if (!crc_table_computed)
		make_crc_table();

	for (uint32_t n = 0; n < len; n++)
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);

	return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
uint32_t Misc::crc(const uint8_t* buf, uint32_t len)
{
	return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}


/* Misc::findJaguarTextureDimensions
 * Find the given name in a texture lump and returns a point2_t
 * which contains the dimensions. In case the texture is not found,
 * the dimensions returned are null.
 *******************************************************************/
point2_t Misc::findJaguarTextureDimensions(ArchiveEntry* entry, string name)
{
	point2_t dimensions;
	dimensions.x = 0;
	dimensions.y = 0;

	// Theoretical minimum size for a Jaguar TEXTURE1 lump
	if (entry->getSize() < 40)
		return dimensions;

	const uint8_t* data = entry->getData();
	size_t numtex = READ_L32(data, 0);

	// 4 bytes for the offset, plus 32 byte for the texture definition itself
	// so a total of 36 bytes per texture; plus four for the texture count
	if (entry->getSize() < 36 * numtex + 4)
		return dimensions;

	// Check that the offset to the first texture comes right after the offset block
	int offset = READ_L32(data, 4);
	if (offset != 4 * numtex + 4)
		return dimensions;

	char texture[9]; texture[8] = 0;

	for (size_t t = 0; t < numtex; ++t, offset+=32)
	{
		memcpy(texture, data+offset, 8);
		if (S_CMPNOCASE(name, texture))
		{
			// We have our texture! Let's get the width and heigth and get out of here
			dimensions.x = READ_L16(data, offset + 12);
			dimensions.y = READ_L16(data, offset + 14);
			return dimensions;
		}
	}
	// We didn't find the texture
	return dimensions;
}

Misc::winf_t Misc::getWindowInfo(string id)
{
	for (unsigned a = 0; a < window_info.size(); a++)
	{
		if (window_info[a].id == id)
			return window_info[a];
	}

	return winf_t("", -1, -1, -1, -1);
}

void Misc::setWindowInfo(string id, int width, int height, int left, int top)
{
	if (id.IsEmpty())
		return;

	for (unsigned a = 0; a < window_info.size(); a++)
	{
		if (window_info[a].id == id)
		{
			if (width >= -1)	window_info[a].width = width;
			if (height >= -1)	window_info[a].height = height;
			if (left >= -1)		window_info[a].left = left;
			if (top >= -1)		window_info[a].top = top;
			return;
		}
	}

	window_info.push_back(winf_t(id, width, height, left, top));
}

void Misc::readWindowInfo(Tokenizer* tz)
{
	// Read definitions
	string token = tz->getToken();
	while (token != "}" && !tz->atEnd())
	{
		string id = token;
		int width = tz->getInteger();
		int height = tz->getInteger();
		int left = tz->getInteger();
		int top = tz->getInteger();
		setWindowInfo(id, width, height, left, top);
		token = tz->getToken();
	}
}

void Misc::writeWindowInfo(wxFile& file)
{
	for (unsigned a = 0; a < window_info.size(); a++)
		file.Write(S_FMT("\t%s %d %d %d %d\n", window_info[a].id, window_info[a].width, window_info[a].height, window_info[a].left, window_info[a].top));
}
