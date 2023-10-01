#pragma once

#include "OpenGL/OpenGL.h"
#include "Utility/Colour.h"

namespace slade::colourconfig
{
struct Colour
{
	bool    exists = false;
	bool    custom = false;
	string  name;
	string  group;
	ColRGBA colour;
	bool    blend_additive;

	gl::Blend blendMode() const { return blend_additive ? gl::Blend::Additive : gl::Blend::Normal; }
};

ColRGBA       colour(const string& name);
const Colour& colDef(const string& name);
void          setColour(
			 const string& name,
			 int           red            = -1,
			 int           green          = -1,
			 int           blue           = -1,
			 int           alpha          = -1,
			 bool          blend_additive = false);

double lineHilightWidth();
double lineSelectionWidth();
double flatAlpha();
void   setLineHilightWidth(double mult);
void   setLineSelectionWidth(double mult);
void   setFlatAlpha(double alpha);

bool readConfiguration(const MemChunk& mc);
bool writeConfiguration(MemChunk& mc);
bool init();
void loadDefaults();

bool readConfiguration(string_view name);
void putConfigurationNames(vector<string>& names);

void putColourNames(vector<string>& list);
} // namespace slade::colourconfig
