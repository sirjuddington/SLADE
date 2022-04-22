
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    Colour.cpp
// Description: Structs and functions for representing and converting colours
//              (RGBA, HSL and CIE-L*a*b)
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Colour.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
const ColRGBA ColRGBA::WHITE{ 255, 255, 255, 255 };
const ColRGBA ColRGBA::BLACK{ 0, 0, 0, 255 };
const ColRGBA ColRGBA::RED{ 255, 0, 0, 255 };
const ColRGBA ColRGBA::GREEN{ 0, 255, 0, 255 };
const ColRGBA ColRGBA::BLUE{ 0, 0, 255, 255 };
const ColRGBA ColRGBA::YELLOW{ 255, 255, 0, 255 };
const ColRGBA ColRGBA::PURPLE{ 255, 0, 255, 255 };
const ColRGBA ColRGBA::CYAN{ 0, 255, 255, 255 };


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Float, col_cie_tristim_x)
EXTERN_CVAR(Float, col_cie_tristim_z)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Converts an RGB colour [red,green,blue] to HSL colourspace [h,s,l]
// -----------------------------------------------------------------------------
void rgbToHsl(double red, double green, double blue, double& h, double& s, double& l)
{
	double v_min = std::min(red, std::min(green, blue));
	double v_max = std::max(red, std::max(green, blue));
	double delta = v_max - v_min;

	// Determine V
	l = (v_max + v_min) * 0.5;

	if (delta == 0)
		h = s = 0; // Grey (r==g==b)
	else
	{
		// Determine S
		if (l < 0.5)
			s = delta / (v_max + v_min);
		else
			s = delta / (2.0 - v_max - v_min);

		// Determine H
		if (red == v_max)
			h = (green - blue) / delta;
		else if (green == v_max)
			h = 2.0 + (blue - red) / delta;
		else if (blue == v_max)
			h = 4.0 + (red - green) / delta;

		h /= 6.0;

		if (h < 0)
			h += 1.0;
	}
}

// -----------------------------------------------------------------------------
// Converts an RGB colour [red,green,blue] to CIE-L*a*b colourspace
// Conversion formulas lazily taken from easyrgb.com.
// -----------------------------------------------------------------------------
#define NORMALIZERGB(a) a = 100 * (((a) > 0.04045) ? (pow((((a) + 0.055) / 1.055), 2.4)) : ((a) / 12.92))
#define NORMALIZEXYZ(a) a = (((a) > 0.008856) ? (pow(a, (1.0 / 3.0))) : ((7.787 * (a)) + (16.0 / 116.0)))
void rgbToLab(double red, double green, double blue, double& l, double& a, double& b)
{
	double x, y, z;

	// Step #1: convert RGB to CIE-XYZ
	NORMALIZERGB(red);
	NORMALIZERGB(green);
	NORMALIZERGB(blue);

	x = (red * 0.4124 + green * 0.3576 + blue * 0.1805) / col_cie_tristim_x;
	y = (red * 0.2126 + green * 0.7152 + blue * 0.0722) / 100.000; // y is always 100.00
	z = (red * 0.0193 + green * 0.1192 + blue * 0.9505) / col_cie_tristim_z;

	// Step #2: convert xyz to lab
	NORMALIZEXYZ(x);
	NORMALIZEXYZ(y);
	NORMALIZEXYZ(z);

	l = (116.0 * y) - 16;
	a = 500.0 * (x - y);
	b = 200.0 * (y - z);
}
#undef NORMALIZERGB
#undef NORMALIZEXYZ

void hslToRgb(double h, double s, double l, double& r, double& g, double& b)
{
	// No saturation means grey
	if (s == 0.)
	{
		r = g = b = l;
		return;
	}

	// Find the rough values at given H with mid L and max S.
	double hue    = (6. * h);
	auto   sector = static_cast<uint8_t>(hue);
	double factor = hue - sector;
	switch (sector)
	{
		// RGB 0xFF0000 to 0xFFFF00, increasingly green
	case 0:
		r = 1.;
		g = factor;
		b = 0.;
		break;
		// RGB 0xFFFF00 to 0x00FF00, decreasingly red
	case 1:
		r = 1. - factor;
		g = 1.;
		b = 0.;
		break;
		// RGB 0x00FF00 to 0x00FFFF, increasingly blue
	case 2:
		r = 0.;
		g = 1.;
		b = factor;
		break;
		// RGB 0x00FFFF to 0x0000FF, decreasingly green
	case 3:
		r = 0.;
		g = 1. - factor;
		b = 1.;
		break;
		// RGB 0x0000FF to 0xFF00FF, increasingly red
	case 4:
		r = factor;
		g = 0.;
		b = 1.;
		break;
		// RGB 0xFF00FF to 0xFF0000, decreasingly blue
	case 5:
		r = 1.;
		g = 0.;
		b = 1. - factor;
		break;
	}

	// Now apply desaturation
	double ds = (1. - s) * 0.5;
	r         = ds + (r * s);
	g         = ds + (g * s);
	b         = ds + (b * s);

	// Finally apply luminosity
	double dl = l * 2.;
	double sr, sg, sb, sl;
	if (dl > 1.)
	{
		// Make brighter
		sl = dl - 1.;
		sr = sl * (1. - r);
		r += sr;
		sg = sl * (1. - g);
		g += sg;
		sb = sl * (1. - b);
		b += sb;
	}
	else if (dl < 1.)
	{
		// Make darker
		sl = 1. - dl;
		sr = sl * r;
		r -= sr;
		sg = sl * g;
		g -= sg;
		sb = sl * b;
		b -= sb;
	}

	// Clamping (shouldn't actually be needed)
	if (r > 1.)
		r = 1.;
	if (r < 0.)
		r = 0.;
	if (g > 1.)
		g = 1.;
	if (g < 0.)
		g = 0.;
	if (b > 1.)
		b = 1.;
	if (b < 0.)
		b = 0.;
}
} // namespace

// -----------------------------------------------------------------------------
//
// ColRGBA Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Converts the colour from RGB to HSL colourspace
// -----------------------------------------------------------------------------
ColHSL ColRGBA::asHSL() const
{
	ColHSL ret;
	ret.alpha = static_cast<double>(a) / 255.0;
	rgbToHsl(dr(), dg(), db(), ret.h, ret.s, ret.l);
	return ret;
}

// -----------------------------------------------------------------------------
// Converts the colour from RGB to CIE-L*a*b colourspace
// -----------------------------------------------------------------------------
ColLAB ColRGBA::asLAB() const
{
	ColLAB ret;
	rgbToLab(dr(), dg(), db(), ret.l, ret.a, ret.b);
	ret.alpha = static_cast<double>(a) / 255.0;
	return ret;
}

// -----------------------------------------------------------------------------
// Sets the colour from [h][s][l] values
// -----------------------------------------------------------------------------
void ColRGBA::fromHSL(double h, double s, double l)
{
	double dr, dg, db;
	hslToRgb(h, s, l, dr, dg, db);

	// Now convert from 0f--1f to 0i--255i, rounding up
	r = static_cast<uint8_t>(dr * 255. + 0.499999999);
	g = static_cast<uint8_t>(dg * 255. + 0.499999999);
	b = static_cast<uint8_t>(db * 255. + 0.499999999);
}

// -----------------------------------------------------------------------------
// Sets the colour from another HSL colour
// -----------------------------------------------------------------------------
void ColRGBA::fromHSL(const ColHSL& hsl, bool take_alpha)
{
	fromHSL(hsl.h, hsl.s, hsl.l);

	if (take_alpha)
		a = static_cast<uint8_t>(hsl.alpha * 255.);
}

// -----------------------------------------------------------------------------
// Returns a string representation of the colour, in the requested [format]
// -----------------------------------------------------------------------------
string ColRGBA::toString(StringFormat format) const
{
	switch (format)
	{
	case StringFormat::RGB: return fmt::format("RGB({}, {}, {})", r, g, b);
	case StringFormat::RGBA: return fmt::format("RGBA({}, {}, {}, {})", r, g, b, a);
	case StringFormat::HEX: return fmt::format("#{:X}{:X}{:X}", r, g, b);
	case StringFormat::ZDoom: return fmt::format("\"{:X} {:X} {:X}\"", r, g, b);
	default: return {};
	}
}



// -----------------------------------------------------------------------------
//
// ColHSL Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Converts the colour from HSL to RGB colourspace
// -----------------------------------------------------------------------------
ColRGBA ColHSL::asRGB() const
{
	ColRGBA ret(0, 0, 0, static_cast<uint8_t>(alpha * 255.), -1);

	double r, g, b;
	hslToRgb(h, s, l, r, g, b);

	// Now convert from 0f--1f to 0i--255i, rounding up
	ret.r = static_cast<uint8_t>(r * 255. + 0.499999999);
	ret.g = static_cast<uint8_t>(g * 255. + 0.499999999);
	ret.b = static_cast<uint8_t>(b * 255. + 0.499999999);

	return ret;
}
