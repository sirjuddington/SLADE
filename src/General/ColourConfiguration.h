#pragma once

namespace ColourConfiguration
{
struct Colour
{
	bool   exists = false;
	bool   custom = false;
	string name;
	string group;
	rgba_t colour;
};

rgba_t getColour(string name);
Colour getColDef(string name);
void   setColour(string name, int red = -1, int green = -1, int blue = -1, int alpha = -1, int blend = -1);

double getLineHilightWidth();
double getLineSelectionWidth();
double getFlatAlpha();
void   setLineHilightWidth(double mult);
void   setLineSelectionWidth(double mult);
void   setFlatAlpha(double alpha);

bool readConfiguration(MemChunk& mc);
bool writeConfiguration(MemChunk& mc);
bool init();
void loadDefaults();

bool readConfiguration(string name);
void getConfigurationNames(vector<string>& names);

void getColourNames(vector<string>& list);
} // namespace ColourConfiguration
