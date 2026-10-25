#pragma once

#include "OpenGL/OpenGL.h"

namespace slade::colourconfig
{
enum class Group
{
	MapPreview,
	MapImage,
	MapEditor
};

enum class MapPreview
{
	Background,
	Line1S,
	Line2S,
	LineSpecial,
	LineMacro,
	Thing,

	Count
};

enum class MapImage
{
	Background,
	Line1S,
	Line2S,
	LineSpecial,
	LineMacro,

	Count
};

enum class MapEditor
{
	Background,
	Grid,
	Grid64,
	LineNormal,
	LineSpecial,
	LineInvalid,
	Vertex,
	Highlight,
	Selection,
	Tagged,
	Tagging,
	ThingPath,
	ThingPathDragon,
	LineDraw,
	Moving,
	ObjectEdit,
	SelBoxFill,
	SelBoxOutline,
	OverlayBackground,
	OverlayForeground,
	EditorMessage,
	EditorMessageOutline,
	Crosshair3D,
	Highlight3D,
	Selection3D,
	OverlayBackground3D,
	OverlayForeground3D,

	Count
};

struct Colour
{
	string  name;
	ColRGBA colour;
	bool    blend_additive;
	string  id;

	Colour() = default;
	Colour(string_view id, string_view name, ColRGBA colour = ColRGBA::WHITE, bool blend_additive = false);

	gl::Blend blendMode() const;

	void set(u8 r, u8 g, u8 b, u8 a = 255, bool blend_additive = false);
};

// TODO: Figure out what to do with this stuff, maybe move map editor stuff out
//       to a separate 'map editor theme' system
double lineHilightWidth();
double lineSelectionWidth();
double flatAlpha();
void   setLineHilightWidth(double mult);
void   setLineSelectionWidth(double mult);
void   setFlatAlpha(double alpha);

const vector<Colour>& allColours(Group group);

ColRGBA colour(MapPreview preview);
ColRGBA colour(MapImage image);
ColRGBA colour(MapEditor editor);

const Colour& colDef(MapPreview preview);
const Colour& colDef(MapImage image);
const Colour& colDef(MapEditor editor);

bool init();

void loadDefaultConfiguration(Group group, vector<Colour>& colours);
void applyDefaultConfiguration(Group group);

void putConfigurationNames(Group group, vector<string>& names);
bool loadConfiguration(Group group, string_view name, vector<Colour>& colours);
bool applyConfiguration(Group group, string_view name);

bool loadCustomConfiguration(Group group, vector<Colour>& colours);
bool applyCustomConfiguration(Group group);
bool setCustomConfiguration(Group group, vector<Colour>& colours, bool apply = true);
void saveCustomConfiguration();

struct CCfgSignals
{
	sigslot::signal<Group> colour_config_changed;
};
CCfgSignals& signals();
} // namespace slade::colourconfig

// Convenience namespace alias
namespace ccfg = slade::colourconfig;
