#pragma once

namespace slade
{
// ColHSL: Represents a colour in HSL format, generally used for calculations
struct ColHSL
{
	double h = 0., s = 0., l = 0.;
	double alpha = 1.;

	ColHSL() = default;
	ColHSL(double h, double s, double l, double a = 1.) : h{ h }, s{ s }, l{ l }, alpha{ a } {}

	ColRGBA asRGB() const;
};

// ColLAB: Represents a colour in CIE-L*a*b format, generally used for calculations
struct ColLAB
{
	double l = 0., a = 0., b = 0.;
	double alpha = 1.;

	ColLAB() = default;
	ColLAB(double l, double a, double b, double alpha = 1.) : l{ l }, a{ a }, b{ b }, alpha{ alpha } {}
};

namespace colour
{
	ColRGBA greyscale(const ColRGBA& colour);

	// Colourspace conversion
	ColHSL  rgbToHsl(const ColRGBA& rgb);
	ColLAB  rgbToLab(const ColRGBA& rgb);
	ColRGBA hslToRgb(const ColHSL& hsl);

	// String conversion
	enum class StringFormat
	{
		RGB,  // rgb(r, g, b)
		RGBA, // rgba(r, g, b, a)
		HEX,  // #RRGGBB
		HEXA, // #RRGGBBAA
		ZDoom // "RR GG BB"
	};
	string   toString(const ColRGBA& colour, StringFormat format = StringFormat::HEX);
	wxColour toWx(const ColRGBA& colour);
	ColRGBA  fromString(string_view str);
} // namespace colour
} // namespace slade
