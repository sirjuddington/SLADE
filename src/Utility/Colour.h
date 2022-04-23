#pragma once

namespace slade
{
struct ColHSL;
struct ColLAB;

// ColRGBA: A 32-bit colour definition
struct ColRGBA
{
	uint8_t r = 0, g = 0, b = 0, a = 0;
	short   index = -1; // -1 = not indexed

	// Constructors
	ColRGBA() = default;
	ColRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, char blend = -1, short index = -1) :
		r{ r }, g{ g }, b{ b }, a{ a }, index{ index }
	{
	}
	ColRGBA(const ColRGBA& c) = default;
	explicit ColRGBA(const wxColour& c) : r{ c.Red() }, g{ c.Green() }, b{ c.Blue() }, a{ c.Alpha() } {}

	// Functions
	void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, char blend = -1, short index = -1)
	{
		this->r     = r;
		this->g     = g;
		this->b     = b;
		this->a     = a;
		this->index = index;
	}

	void set(const ColRGBA& colour)
	{
		r     = colour.r;
		g     = colour.g;
		b     = colour.b;
		a     = colour.a;
		index = colour.index;
	}

	void set(const wxColour& colour)
	{
		r = colour.Red();
		g = colour.Green();
		b = colour.Blue();
		a = colour.Alpha();
	}

	float fr() const { return (float)r / 255.0f; }
	float fg() const { return (float)g / 255.0f; }
	float fb() const { return (float)b / 255.0f; }
	float fa() const { return (float)a / 255.0f; }

	double dr() const { return (double)r / 255.0; }
	double dg() const { return (double)g / 255.0; }
	double db() const { return (double)b / 255.0; }
	double da() const { return (double)a / 255.0; }

	bool equals(const ColRGBA& rhs, bool check_alpha = false, bool check_index = false) const
	{
		bool col_equal = (r == rhs.r && g == rhs.g && b == rhs.b);

		if (check_index)
			col_equal &= (this->index == rhs.index);
		if (check_alpha)
			return col_equal && (a == rhs.a);
		else
			return col_equal;
	}

	// Amplify/fade colour components by given amounts
	ColRGBA amp(int R, int G, int B, int A) const
	{
		int nr = r + R;
		int ng = g + G;
		int nb = b + B;
		int na = a + A;

		if (nr > 255)
			nr = 255;
		if (nr < 0)
			nr = 0;
		if (ng > 255)
			ng = 255;
		if (ng < 0)
			ng = 0;
		if (nb > 255)
			nb = 255;
		if (nb < 0)
			nb = 0;
		if (na > 255)
			na = 255;
		if (na < 0)
			na = 0;

		return { (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, (uint8_t)na, -1 };
	}

	// Amplify/fade colour components by factors
	ColRGBA ampf(float fr, float fg, float fb, float fa) const
	{
		int nr = (int)(r * fr);
		int ng = (int)(g * fg);
		int nb = (int)(b * fb);
		int na = (int)(a * fa);

		if (nr > 255)
			nr = 255;
		if (nr < 0)
			nr = 0;
		if (ng > 255)
			ng = 255;
		if (ng < 0)
			ng = 0;
		if (nb > 255)
			nb = 255;
		if (nb < 0)
			nb = 0;
		if (na > 255)
			na = 255;
		if (na < 0)
			na = 0;

		return { (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, (uint8_t)na, -1 };
	}

	void write(uint8_t* ptr) const
	{
		if (ptr)
		{
			ptr[0] = r;
			ptr[1] = g;
			ptr[2] = b;
			ptr[3] = a;
		}
	}

	// Returns a copy of this colour as greyscale (using 'common' component coefficients)
	ColRGBA greyscale() const
	{
		uint8_t l = r * 0.3 + g * 0.59 + b * 0.11;
		return { l, l, l, a };
	}

	ColHSL asHSL() const;
	ColLAB asLAB() const;
	void   fromHSL(double h, double s, double l);
	void   fromHSL(const ColHSL& hsl, bool take_alpha = false);

	// String conversion
	enum class StringFormat
	{
		RGB,  // RGB(r, g, b)
		RGBA, // RGBA(r, g, b, a)
		HEX,  // #rrggbb
		ZDoom // "rr gg bb"
	};
	string   toString(StringFormat format = StringFormat::HEX) const;
	wxColour toWx() const { return { r, g, b, a }; }

	// Some basic colours
	static const ColRGBA WHITE;
	static const ColRGBA BLACK;
	static const ColRGBA RED;
	static const ColRGBA GREEN;
	static const ColRGBA BLUE;
	static const ColRGBA YELLOW;
	static const ColRGBA PURPLE;
	static const ColRGBA CYAN;
};

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
} // namespace slade
