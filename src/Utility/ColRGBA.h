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
		r{ r },
		g{ g },
		b{ b },
		a{ a },
		index{ index }
	{
	}
	ColRGBA(const ColRGBA& c) = default;
	explicit ColRGBA(const wxColour& c) : r{ c.Red() }, g{ c.Green() }, b{ c.Blue() }, a{ c.Alpha() } {}

	operator wxColour() const { return { r, g, b, a }; }

	// Functions
	void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, char blend = -1, short index = -1);
	void set(const ColRGBA& colour);
	void set(const wxColour& colour);

	float fr() const { return static_cast<float>(r) / 255.0f; }
	float fg() const { return static_cast<float>(g) / 255.0f; }
	float fb() const { return static_cast<float>(b) / 255.0f; }
	float fa() const { return static_cast<float>(a) / 255.0f; }

	double dr() const { return static_cast<double>(r) / 255.0; }
	double dg() const { return static_cast<double>(g) / 255.0; }
	double db() const { return static_cast<double>(b) / 255.0; }
	double da() const { return static_cast<double>(a) / 255.0; }

	bool equals(const ColRGBA& rhs, bool check_alpha = false, bool check_index = false) const;

	ColRGBA amp(int R, int G, int B, int A) const;
	ColRGBA ampf(float fr, float fg, float fb, float fa) const;
	void    write(uint8_t* ptr) const;

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
} // namespace slade
