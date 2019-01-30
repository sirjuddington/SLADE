
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
const ColRGBA ColRGBA::WHITE{ 255, 255, 255, 255, 0 };
const ColRGBA ColRGBA::BLACK{ 0, 0, 0, 255, 0 };
const ColRGBA ColRGBA::RED{ 255, 0, 0, 255, 0 };
const ColRGBA ColRGBA::GREEN{ 0, 255, 0, 255, 0 };
const ColRGBA ColRGBA::BLUE{ 0, 0, 255, 255, 0 };
const ColRGBA ColRGBA::YELLOW{ 255, 255, 0, 255, 0 };
const ColRGBA ColRGBA::PURPLE{ 255, 0, 255, 255, 0 };
const ColRGBA ColRGBA::CYAN{ 0, 255, 255, 255, 0 };


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Float, col_cie_tristim_x)
EXTERN_CVAR(Float, col_cie_tristim_z)


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
	auto red   = dr();
	auto green = dg();
	auto blue  = db();

	ColHSL ret;
	double v_min = std::min(red, std::min(green, blue));
	double v_max = std::max(red, std::max(green, blue));
	double delta = v_max - v_min;

	// Determine V
	ret.l = (v_max + v_min) * 0.5;

	if (delta == 0)
		ret.h = ret.s = 0; // Grey (r==g==b)
	else
	{
		// Determine S
		if (ret.l < 0.5)
			ret.s = delta / (v_max + v_min);
		else
			ret.s = delta / (2.0 - v_max - v_min);

		// Determine H
		if (red == v_max)
			ret.h = (green - blue) / delta;
		else if (green == v_max)
			ret.h = 2.0 + (blue - red) / delta;
		else if (blue == v_max)
			ret.h = 4.0 + (red - green) / delta;

		ret.h /= 6.0;

		if (ret.h < 0)
			ret.h += 1.0;
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Converts the colour from RGB to CIE-L*a*b colourspace
// Conversion formulas lazily taken from easyrgb.com.
// -----------------------------------------------------------------------------
#define NORMALIZERGB(a) a = 100 * ((a > 0.04045) ? (pow(((a + 0.055) / 1.055), 2.4)) : (a / 12.92))
#define NORMALIZEXYZ(a) a = ((a > 0.008856) ? (pow(a, (1.0 / 3.0))) : ((7.787 * a) + (16.0 / 116.0)))
ColLAB ColRGBA::asLAB() const
{
	auto   red   = dr();
	auto   green = dg();
	auto   blue  = db();
	double x, y, z;
	ColLAB ret;

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

	ret.l = (116.0 * y) - 16;
	ret.a = 500.0 * (x - y);
	ret.b = 200.0 * (y - z);

	return ret;
}
#undef NORMALIZERGB
#undef NORMALIZEXYZ


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
	ColRGBA ret(0, 0, 0, 255, -1);

	// No saturation means grey
	if (s == 0.)
	{
		ret.r = ret.g = ret.b = (uint8_t)(255. * l);
		return ret;
	}

	// Find the rough values at given H with mid L and max S.
	double  hue    = (6. * h);
	uint8_t sector = (uint8_t)hue;
	double  factor = hue - sector;
	double  dr, dg, db;
	switch (sector)
	{
		// RGB 0xFF0000 to 0xFFFF00, increasingly green
	case 0:
		dr = 1.;
		dg = factor;
		db = 0.;
		break;
		// RGB 0xFFFF00 to 0x00FF00, decreasingly red
	case 1:
		dr = 1. - factor;
		dg = 1.;
		db = 0.;
		break;
		// RGB 0x00FF00 to 0x00FFFF, increasingly blue
	case 2:
		dr = 0.;
		dg = 1.;
		db = factor;
		break;
		// RGB 0x00FFFF to 0x0000FF, decreasingly green
	case 3:
		dr = 0.;
		dg = 1. - factor;
		db = 1.;
		break;
		// RGB 0x0000FF to 0xFF00FF, increasingly red
	case 4:
		dr = factor;
		dg = 0.;
		db = 1.;
		break;
		// RGB 0xFF00FF to 0xFF0000, decreasingly blue
	case 5:
		dr = 1.;
		dg = 0.;
		db = 1. - factor;
		break;
	}

	// Now apply desaturation
	double ds = (1. - s) * 0.5;
	dr        = ds + (dr * s);
	dg        = ds + (dg * s);
	db        = ds + (db * s);

	// Finally apply luminosity
	double dl = l * 2.;
	double sr, sg, sb, sl;
	if (dl > 1.)
	{
		// Make brighter
		sl = dl - 1.;
		sr = sl * (1. - dr);
		dr += sr;
		sg = sl * (1. - dg);
		dg += sg;
		sb = sl * (1. - db);
		db += sb;
	}
	else if (dl < 1.)
	{
		// Make darker
		sl = 1. - dl;
		sr = sl * dr;
		dr -= sr;
		sg = sl * dg;
		dg -= sg;
		sb = sl * db;
		db -= sb;
	}

	// Clamping (shouldn't actually be needed)
	if (dr > 1.)
		dr = 1.;
	if (dr < 0.)
		dr = 0.;
	if (dg > 1.)
		dg = 1.;
	if (dg < 0.)
		dg = 0.;
	if (db > 1.)
		db = 1.;
	if (db < 0.)
		db = 0.;

	// Now convert from 0f--1f to 0i--255i, rounding up
	ret.r = (uint8_t)(dr * 255. + 0.499999999);
	ret.g = (uint8_t)(dg * 255. + 0.499999999);
	ret.b = (uint8_t)(db * 255. + 0.499999999);

	return ret;
}
