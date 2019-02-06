#pragma once

namespace ColourConfiguration
{
struct Colour
{
	bool     exists = false;
	bool     custom = false;
	wxString name;
	wxString group;
	ColRGBA  colour;
};

ColRGBA colour(const wxString& name);
Colour  colDef(const wxString& name);
void    setColour(const wxString& name, int red = -1, int green = -1, int blue = -1, int alpha = -1, int blend = -1);

double lineHilightWidth();
double lineSelectionWidth();
double flatAlpha();
void   setLineHilightWidth(double mult);
void   setLineSelectionWidth(double mult);
void   setFlatAlpha(double alpha);

bool readConfiguration(MemChunk& mc);
bool writeConfiguration(MemChunk& mc);
bool init();
void loadDefaults();

bool readConfiguration(const wxString& name);
void putConfigurationNames(vector<wxString>& names);

void putColourNames(vector<wxString>& list);
} // namespace ColourConfiguration
