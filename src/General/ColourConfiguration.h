#pragma once

#include "JsonFwd.h"
#include "OpenGL/OpenGL.h"

namespace slade::colourconfig
{
struct Colour
{
	string  name;
	ColRGBA colour;
	bool    blend_additive;
	string  id;

	Colour() = default;
	Colour(string_view id, string_view name, ColRGBA colour, bool blend_additive = false) :
		name{ name },
		colour{ colour },
		blend_additive{ blend_additive },
		id{ id }
	{
	}

	gl::Blend blendMode() const { return blend_additive ? gl::Blend::Additive : gl::Blend::Normal; }
};

double lineHilightWidth();
double lineSelectionWidth();
double flatAlpha();
void   setLineHilightWidth(double mult);
void   setLineSelectionWidth(double mult);
void   setFlatAlpha(double alpha);

void writeConfiguration(string_view json_file);
bool init();
void loadDefaults(string_view group = {});

bool readConfiguration(string_view name, string_view group);
void putConfigurationNames(vector<string>& names, string_view group);

const vector<Colour>& allColours();

enum class ColourId
{
	MapPreviewBackground,
	MapPreviewLine1S,
	MapPreviewLine2S,
	MapPreviewLineSpecial,
	MapPreviewLineMacro,
	MapPreviewThing,

	MapImageBackground,
	MapImageLine1S,
	MapImageLine2S,
	MapImageLineSpecial,
	MapImageLineMacro,

	MapBackground,
	MapGrid,
	MapGrid64,
	MapLineNormal,
	MapLineSpecial,
	MapLineInvalid,
	MapVertex,
	MapHighlight,
	MapSelection,
	MapTagged,
	MapTagging,
	MapThingPath,
	MapThingPathDragon,
	MapLineDraw,
	MapMoving,
	MapObjectEdit,
	MapSelBoxFill,
	MapSelBoxOutline,
	MapOverlayBackground,
	MapOverlayForeground,
	MapEditorMessage,
	MapEditorMessageOutline,
	Map3DCrosshair,
	Map3DHighlight,
	Map3DSelection,
	Map3DOverlayBackground,
	Map3DOverlayForeground,

	_Count
};
ColRGBA       colour(ColourId id);
const Colour& colDef(ColourId id);
ColRGBA       mapPreviewColour(ColourId id);
void          setColour(
			 ColourId       id,
			 int            red            = -1,
			 int            green          = -1,
			 int            blue           = -1,
			 int            alpha          = -1,
			 optional<bool> blend_additive = {});
} // namespace slade::colourconfig

// Convenience namespace alias
namespace ccfg = slade::colourconfig;
