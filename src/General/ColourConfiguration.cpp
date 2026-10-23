
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ColourConfiguration.cpp
// Description: Functions for handling colour configurations
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
#include "ColourConfiguration.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Console.h"
#include "UI/WxUtils.h"
#include "Utility/Colour.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::colourconfig
{
typedef std::map<string, Colour> ColourHashMap;

double         line_hilight_width   = 3.0;
double         line_selection_width = 4.0;
double         flat_alpha           = 1.0;
vector<Colour> cols_map_preview;
vector<Colour> cols_map_image;
vector<Colour> cols_map_editor;
} // namespace slade::colourconfig
CVAR(String, ccfg_map_preview, "", CVar::Flag::Save)
CVAR(String, ccfg_map_image, "", CVar::Flag::Save)
CVAR(String, ccfg_map_editor, "", CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace slade::colourconfig
{
// -----------------------------------------------------------------------------
// Converts a Colour struct to a JSON object
// -----------------------------------------------------------------------------
void to_json(json& j, const Colour& c)
{
	auto col_string = colour::toString(
		c.colour, c.colour.a == 255 ? colour::StringFormat::RGB : colour::StringFormat::RGBA);

	j = json{ { "colour", col_string } };

	if (c.blend_additive)
		j["blend_additive"] = true;
}

// -----------------------------------------------------------------------------
// Converts a JSON object to a Colour struct
// -----------------------------------------------------------------------------
void from_json(const json& j, Colour& c)
{
	auto col = j.at("colour");
	if (col.is_array() && col.size() == 4)
		c.colour.set(col[0].get<int>(), col[1].get<int>(), col[2].get<int>(), col[3].get<int>());
	else if (col.is_array() && col.size() == 3)
		c.colour.set(col[0].get<int>(), col[1].get<int>(), col[2].get<int>());
	else if (col.is_string())
		c.colour = colour::fromString(col.get<string>());

	if (j.contains("blend_additive"))
		j.at("blend_additive").get_to(c.blend_additive);
}

string groupSubdir(Group group)
{
	switch (group)
	{
	case Group::MapPreview: return "map_preview";
	case Group::MapImage:   return "map_image";
	case Group::MapEditor:  return "map_editor";
	default:                return "";
	}
}

Colour* colourFromId(string_view id, vector<Colour>& colours)
{
	for (auto& c : colours)
	{
		if (c.id == id)
			return &c;
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Reads a colour configuration from JSON [j]
// -----------------------------------------------------------------------------
bool readConfiguration(Json& j, vector<Colour>& colours, string_view source)
{
	// Colours
	if (j.contains("colours"))
	{
		for (auto& [id, colour] : j["colours"].items())
		{
			if (const auto col = colourFromId(id, colours); col)
				from_json(colour, *col);
			else
				log::warning(fmt::format(R"(Unknown colour id "{}" in configuration "{}")", id, source));
		}
	}

	// Theme
	if (j.contains("theme"))
	{
		if (j["theme"].contains("line_hilight_width"))
			j["theme"]["line_hilight_width"].get_to(line_hilight_width);
		if (j["theme"].contains("line_selection_width"))
			j["theme"]["line_selection_width"].get_to(line_selection_width);
		if (j["theme"].contains("flat_alpha"))
			j["theme"]["flat_alpha"].get_to(flat_alpha);
	}

	return true;
}

bool readConfiguration(string_view name, string_view subdir, vector<Colour>& colours)
{
	// Search resource pk3
	auto res = app::programResource();
	auto dir = res->dirAtPath(fmt::format("config/colours/{}", subdir));
	if (!dir)
		return false;

	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		if (strutil::equalCI(dir->entryAt(a)->nameNoExt(), name))
		{
			if (auto j = jsonutil::parse(dir->entryAt(a)->data()); !j.is_discarded())
				readConfiguration(j, colours, name);
			return true;
		}
	}

	return false;
}
} // namespace slade::colourconfig


// -----------------------------------------------------------------------------
//
// colourconfig Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::colourconfig
{

}

colourconfig::Colour::Colour(string_view id, string_view name, ColRGBA colour, bool blend_additive) :
	name{ name },
	colour{ colour },
	blend_additive{ blend_additive },
	id{ id }
{
}

gl::Blend colourconfig::Colour::blendMode() const
{
	return blend_additive ? gl::Blend::Additive : gl::Blend::Normal;
}

void colourconfig::Colour::set(u8 r, u8 g, u8 b, u8 a, bool blend_additive)
{
	colour.set(r, g, b, a);
	this->blend_additive = blend_additive;
}

// -----------------------------------------------------------------------------
// Returns the line hilight width multiplier
// -----------------------------------------------------------------------------
double colourconfig::lineHilightWidth()
{
	return line_hilight_width;
}

// -----------------------------------------------------------------------------
// Returns the line selection width multiplier
// -----------------------------------------------------------------------------
double colourconfig::lineSelectionWidth()
{
	return line_selection_width;
}

// -----------------------------------------------------------------------------
// Returns the flat alpha multiplier
// -----------------------------------------------------------------------------
double colourconfig::flatAlpha()
{
	return flat_alpha;
}

// -----------------------------------------------------------------------------
// Sets the line hilight width multiplier
// -----------------------------------------------------------------------------
void colourconfig::setLineHilightWidth(double mult)
{
	line_hilight_width = mult;
}

// -----------------------------------------------------------------------------
// Sets the line selection width multiplier
// -----------------------------------------------------------------------------
void colourconfig::setLineSelectionWidth(double mult)
{
	line_selection_width = mult;
}

// -----------------------------------------------------------------------------
// Sets the flat alpha multiplier
// -----------------------------------------------------------------------------
void colourconfig::setFlatAlpha(double alpha)
{
	flat_alpha = alpha;
}

const vector<colourconfig::Colour>& colourconfig::allColours(Group group)
{
	switch (group)
	{
	case Group::MapPreview: return cols_map_preview;
	case Group::MapImage:   return cols_map_image;
	case Group::MapEditor:  return cols_map_editor;
	default:                return cols_map_preview;
	}
}

ColRGBA colourconfig::colour(MapPreview preview)
{
	if (ccfg_map_preview.empty())
	{
		ColRGBA fg(wxSystemSettings().GetColour(wxSYS_COLOUR_WINDOWTEXT));
		ColRGBA bg(wxutil::systemPanelBGColour());

		static double factor_line2s = app::isDarkTheme() ? 0.5 : 0.6;

		// Default map preview colours (based on system theme)
		switch (preview)
		{
		case MapPreview::Background:  return bg;
		case MapPreview::Line1S:      return fg;
		case MapPreview::Line2S:      return colour::blend(fg, bg, factor_line2s);
		case MapPreview::LineSpecial: return ColRGBA{ 130, 140, 255 };
		case MapPreview::LineMacro:   return ColRGBA{ 255, 170, 130 };
		case MapPreview::Thing:       return ColRGBA{ 211, 168, 86 };
		default:                      return cols_map_preview[static_cast<size_t>(preview)].colour;
		}
	}

	return cols_map_preview[static_cast<size_t>(preview)].colour;
}

ColRGBA colourconfig::colour(MapImage image)
{
	return cols_map_image[static_cast<size_t>(image)].colour;
}

ColRGBA colourconfig::colour(MapEditor editor)
{
	return cols_map_editor[static_cast<size_t>(editor)].colour;
}

const colourconfig::Colour& colourconfig::colDef(MapPreview preview)
{
	return cols_map_preview[static_cast<size_t>(preview)];
}

const colourconfig::Colour& colourconfig::colDef(MapImage image)
{
	return cols_map_image[static_cast<size_t>(image)];
}

const colourconfig::Colour& colourconfig::colDef(MapEditor editor)
{
	return cols_map_editor[static_cast<size_t>(editor)];
}

bool colourconfig::init()
{
	// Init map preview colours
	cols_map_preview = {
		{ "background", "Background" },       { "line_1s", "Line (1-Sided)" },  { "line_2s", "Line (2-Sided)" },
		{ "line_special", "Line (Special)" }, { "line_macro", "Line (Macro)" }, { "thing", "Thing" },
	};

	// Init map image colours
	cols_map_image = {
		{ "background", "Background" },       { "line_1s", "Line (1-Sided)" },  { "line_2s", "Line (2-Sided)" },
		{ "line_special", "Line (Special)" }, { "line_macro", "Line (Macro)" },
	};

	// Init map editor colours
	cols_map_editor = {
		{ "background", "Background" },
		{ "grid", "Grid" },
		{ "grid_64", "Grid (64)" },
		{ "line_normal", "Line (Normal)" },
		{ "line_special", "Line (Special)" },
		{ "line_invalid", "Line (Invalid)" },
		{ "vertex", "Vertex" },
		{ "highlight", "Highlighted Item" },
		{ "selection", "Selected Item(s)" },
		{ "tagged", "Tagged Item(s)" },
		{ "tagging", "Tagging Item(s)" },
		{ "thing_path", "Pathed Thing Line" },
		{ "thing_path_dragon", "Pathed Thing Line (Dragon)" },
		{ "line_draw", "Line Drawing" },
		{ "moving", "Moving Item(s)" },
		{ "object_edit", "Object Edit" },
		{ "sel_box_fill", "Selection Box Fill" },
		{ "sel_box_outline", "Selection Box Outline" },
		{ "overlay_background", "Info Overlay Background" },
		{ "overlay_foreground", "Info Overlay Text" },
		{ "editor_message", "Editor Message Text" },
		{ "editor_message_outline", "Editor Message Outline" },
		{ "3d_crosshair", "3D Crosshair" },
		{ "3d_highlight", "3D Highlight" },
		{ "3d_selection", "3D Selection" },
		{ "3d_overlay_background", "3D Info Overlay Background" },
		{ "3d_overlay_foreground", "3D Info Overlay Text" },
	};

	// Load default configurations
	loadDefaults(Group::MapPreview, cols_map_preview);
	loadDefaults(Group::MapImage, cols_map_image);
	loadDefaults(Group::MapEditor, cols_map_editor);

	// Load selected configurations
	if (!ccfg_map_preview.empty())
	{
		if (ccfg_map_preview.value != "custom")
			readConfiguration(Group::MapPreview, ccfg_map_preview, cols_map_preview);
		// TODO: Custom
	}
	if (!ccfg_map_image.empty())
	{
		if (ccfg_map_image.value != "custom")
			readConfiguration(Group::MapImage, ccfg_map_image, cols_map_image);
		// TODO: Custom
	}
	if (!ccfg_map_editor.empty())
	{
		if (ccfg_map_editor.value != "custom")
			readConfiguration(Group::MapEditor, ccfg_map_editor, cols_map_editor);
		// TODO: Custom
	}

	return true;
}

void colourconfig::loadDefaults(Group group)
{
	switch (group)
	{
	case Group::MapPreview: loadDefaults(group, cols_map_preview); break;
	case Group::MapImage:   loadDefaults(group, cols_map_image); break;
	case Group::MapEditor:  loadDefaults(group, cols_map_editor); break;
	}

	signals().colour_config_changed(group);
}

#define ID(id) static_cast<size_t>(id)
void colourconfig::loadDefaults(Group group, vector<Colour>& colours)
{
	switch (group)
	{
	case Group::MapPreview:
		if (colours.size() <= ID(MapPreview::Thing))
			return;

		colours[ID(MapPreview::Background)].set(0, 0, 0);
		colours[ID(MapPreview::Line1S)].set(255, 255, 255);
		colours[ID(MapPreview::Line2S)].set(170, 170, 170);
		colours[ID(MapPreview::LineSpecial)].set(130, 140, 255);
		colours[ID(MapPreview::LineMacro)].set(255, 170, 130);
		colours[ID(MapPreview::Thing)].set(211, 168, 86);
		break;

	case Group::MapImage:
		if (colours.size() <= ID(MapImage::LineMacro))
			return;

		colours[ID(MapImage::Background)].set(255, 255, 255, 0);
		colours[ID(MapImage::Line1S)].set(0, 0, 0);
		colours[ID(MapImage::Line2S)].set(144, 144, 144);
		colours[ID(MapImage::LineSpecial)].set(220, 130, 50);
		colours[ID(MapImage::LineMacro)].set(50, 130, 220);
		break;

	case Group::MapEditor:
		if (colours.size() <= ID(MapEditor::OverlayForeground3D))
			return;

		colours[ID(MapEditor::Background)].set(0, 0, 0);
		colours[ID(MapEditor::Grid)].set(50, 80, 170, 60);
		colours[ID(MapEditor::Grid64)].set(255, 255, 255, 20, true);
		colours[ID(MapEditor::LineNormal)].set(255, 255, 255);
		colours[ID(MapEditor::LineSpecial)].set(90, 130, 255);
		colours[ID(MapEditor::LineInvalid)].set(255, 0, 0);
		colours[ID(MapEditor::Vertex)].set(80, 220, 130);
		colours[ID(MapEditor::Highlight)].set(255, 255, 0, 255, true);
		colours[ID(MapEditor::Selection)].set(0, 100, 255, 180, true);
		colours[ID(MapEditor::Tagged)].set(0, 255, 100, 120, true);
		colours[ID(MapEditor::Tagging)].set(255, 0, 100, 220, true);
		colours[ID(MapEditor::ThingPath)].set(34, 255, 255);
		colours[ID(MapEditor::ThingPathDragon)].set(255, 34, 34);
		colours[ID(MapEditor::LineDraw)].set(0, 150, 50);
		colours[ID(MapEditor::Moving)].set(255, 100, 0, 255, true);
		colours[ID(MapEditor::ObjectEdit)].set(220, 140, 0, 170, true);
		colours[ID(MapEditor::SelBoxFill)].set(0, 100, 255, 100, true);
		colours[ID(MapEditor::SelBoxOutline)].set(0, 100, 255, 255, true);
		colours[ID(MapEditor::OverlayBackground)].set(0, 0, 0, 180);
		colours[ID(MapEditor::OverlayForeground)].set(255, 255, 255);
		colours[ID(MapEditor::EditorMessage)].set(255, 200, 100);
		colours[ID(MapEditor::EditorMessageOutline)].set(0, 0, 0);
		colours[ID(MapEditor::Crosshair3D)].set(180, 190, 255, 255, true);
		colours[ID(MapEditor::Highlight3D)].set(255, 255, 140, 170);
		colours[ID(MapEditor::Selection3D)].set(30, 100, 255, 180);
		colours[ID(MapEditor::OverlayBackground3D)].set(0, 0, 0, 180);
		colours[ID(MapEditor::OverlayForeground3D)].set(255, 255, 255);
		break;
	}
}
#undef ID

// -----------------------------------------------------------------------------
// Adds all available [group] colour configuration names to [names]
// -----------------------------------------------------------------------------
void colourconfig::putConfigurationNames(Group group, vector<string>& names)
{
	// Search resource pk3
	auto res = app::programResource();
	auto dir = res->dirAtPath(fmt::format("config/colours/{}", groupSubdir(group)));
	if (!dir)
		return;

	for (unsigned a = 0; a < dir->numEntries(); a++)
		names.emplace_back(dir->entryAt(a)->nameNoExt());
}

bool colourconfig::readConfiguration(Group group, string_view name, vector<Colour>& colours)
{
	return readConfiguration(name, groupSubdir(group), colours);
}

bool colourconfig::loadConfiguration(Group group, string_view name)
{
	bool ok = false;
	switch (group)
	{
	case Group::MapPreview: ok = readConfiguration(group, name, cols_map_preview); break;
	case Group::MapImage:   ok = readConfiguration(group, name, cols_map_image); break;
	case Group::MapEditor:  ok = readConfiguration(group, name, cols_map_editor); break;
	default:                break;
	}

	if (ok)
		signals().colour_config_changed(group);

	return ok;
}

colourconfig::CCfgSignals& colourconfig::signals()
{
	static CCfgSignals s;
	return s;
};




#if 0
// -----------------------------------------------------------------------------
//
// colourconfig Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::colourconfig
{
// -----------------------------------------------------------------------------
// Reads a pre-3.3.0 format colour configuration from text data [mc]
// -----------------------------------------------------------------------------
bool readOldConfiguration(const MemChunk& mc)
{
	static std::map<string, ColourId> old_id_to_new_id = {
		{ "map_view_background", ColourId::MapPreviewBackground },
		{ "map_view_line_1s", ColourId::MapPreviewLine1S },
		{ "map_view_line_2s", ColourId::MapPreviewLine2S },
		{ "map_view_line_special", ColourId::MapPreviewLineSpecial },
		{ "map_view_line_macro", ColourId::MapPreviewLineMacro },
		{ "map_view_thing", ColourId::MapPreviewThing },
		{ "map_image_background", ColourId::MapImageBackground },
		{ "map_image_line_1s", ColourId::MapImageLine1S },
		{ "map_image_line_2s", ColourId::MapImageLine2S },
		{ "map_image_line_special", ColourId::MapImageLineSpecial },
		{ "map_image_line_macro", ColourId::MapImageLineMacro },
	};

	// TODO: This
	return true;

#if 0
	// Parse text
	Parser parser;
	if (!parser.parseText(mc))
		return false;

	// Get 'colours' block
	if (auto colours = parser.parseTreeRoot()->childPTN("colours"))
	{
		// Read all colour definitions
		for (unsigned a = 0; a < colours->nChildren(); a++)
		{
			auto def = colours->childPTN(a);

			// Read properties
			for (unsigned b = 0; b < def->nChildren(); b++)
			{
				auto  prop = def->childPTN(b);
				auto& col  = cc_colours[def->name()];
				col.exists = true;

				// Colour name
				if (prop->name() == "name")
					col.name = prop->stringValue();

				// Colour group (for config ui)
				else if (prop->name() == "group")
					col.group = prop->stringValue();

				// Colour
				else if (prop->name() == "rgb")
					col.colour.set(prop->intValue(0), prop->intValue(1), prop->intValue(2));

				// Alpha
				else if (prop->name() == "alpha")
					col.colour.a = prop->intValue();

				// Additive
				else if (prop->name() == "additive")
					col.blend_additive = prop->boolValue();

				else
					log::warning(fmt::format("Unknown colour definition property \"{}\"", prop->name()));
			}
		}
	}

	// Get 'theme' block
	if (auto theme = parser.parseTreeRoot()->childPTN("theme"))
	{
		// Read all theme definitions
		for (unsigned a = 0; a < theme->nChildren(); a++)
		{
			auto prop = theme->childPTN(a);

			if (prop->name() == "line_hilight_width")
				line_hilight_width = prop->floatValue();

			else if (prop->name() == "line_selection_width")
				line_selection_width = prop->floatValue();

			else if (prop->name() == "flat_alpha")
				flat_alpha = prop->floatValue();

			else
				log::warning(fmt::format("Unknown theme property \"{}\"", prop->name()));
		}
	}

	return true;
#endif
}
} // namespace slade::colourconfig

// -----------------------------------------------------------------------------
// Returns the line hilight width multiplier
// -----------------------------------------------------------------------------
double colourconfig::lineHilightWidth()
{
	return line_hilight_width;
}

// -----------------------------------------------------------------------------
// Returns the line selection width multiplier
// -----------------------------------------------------------------------------
double colourconfig::lineSelectionWidth()
{
	return line_selection_width;
}

// -----------------------------------------------------------------------------
// Returns the flat alpha multiplier
// -----------------------------------------------------------------------------
double colourconfig::flatAlpha()
{
	return flat_alpha;
}

// -----------------------------------------------------------------------------
// Sets the line hilight width multiplier
// -----------------------------------------------------------------------------
void colourconfig::setLineHilightWidth(double mult)
{
	line_hilight_width = mult;
}

// -----------------------------------------------------------------------------
// Sets the line selection width multiplier
// -----------------------------------------------------------------------------
void colourconfig::setLineSelectionWidth(double mult)
{
	line_selection_width = mult;
}

// -----------------------------------------------------------------------------
// Sets the flat alpha multiplier
// -----------------------------------------------------------------------------
void colourconfig::setFlatAlpha(double alpha)
{
	flat_alpha = alpha;
}

// -----------------------------------------------------------------------------
// Writes the current colour configuration to [json_file]
// -----------------------------------------------------------------------------
void colourconfig::writeConfiguration(string_view json_file)
{
	json j;

	// TODO: Colours
	// for (const auto& i : cc_colours)
	// {
	// 	const auto& cc = i.second;
	// 	if (!cc.exists)
	// 		continue;
	//
	// 	j["colours"][i.first] = cc;
	// }

	// Theme
	j["theme"]["line_hilight_width"]   = line_hilight_width;
	j["theme"]["line_selection_width"] = line_selection_width;
	j["theme"]["flat_alpha"]           = flat_alpha;

	jsonutil::writeFile(j, json_file);
}

// -----------------------------------------------------------------------------
// Initialises the colour configuration
// -----------------------------------------------------------------------------
bool colourconfig::init()
{
	cc_colours.resize(static_cast<size_t>(ColourId::Count));

	// Load default configuration
	loadDefaults();

	// TODO: This
	// // Check for saved colour configuration
	// if (auto j = jsonutil::parseFile(app::path("colours.json", app::Dir::User)); !j.is_discarded())
	// 	readConfiguration(j);
	//
	// // Check for pre-3.3.0 configuration
	// else if (MemChunk mc; mc.importFile(app::path("colours.cfg", app::Dir::OldUser)))
	// 	readOldConfiguration(mc);

	return true;
}

// -----------------------------------------------------------------------------
// Sets all colours in the current configuration [group] to default.
// If [group] is empty, all colours are reset to default
// -----------------------------------------------------------------------------
void colourconfig::loadDefaults(string_view group)
{
	auto init_colour =
		[](ColourId id, string_view id_str, string_view name, u8 r, u8 g, u8 b, u8 a = 255, bool blend_additive = false)
	{ cc_colours[static_cast<size_t>(id)] = Colour(id_str, name, ColRGBA(r, g, b, a), blend_additive); };

	// Map preview
	if (group.empty() || group == "map_preview")
	{
		init_colour(ColourId::MapPreviewBackground, "map_preview.background", "Background", 0, 0, 0);
		init_colour(ColourId::MapPreviewLine1S, "map_preview.line_1s", "Line (1-Sided)", 255, 255, 255);
		init_colour(ColourId::MapPreviewLine2S, "map_preview.line_2s", "Line (2-Sided)", 170, 170, 170);
		init_colour(ColourId::MapPreviewLineSpecial, "map_preview.line_special", "Line (Special)", 130, 140, 255);
		init_colour(ColourId::MapPreviewLineMacro, "map_preview.line_macro", "Line (Macro)", 255, 170, 130);
		init_colour(ColourId::MapPreviewThing, "map_preview.thing", "Thing", 211, 168, 86);
	}

	// Map image export
	if (group.empty() || group == "map_image")
	{
		init_colour(ColourId::MapImageBackground, "map_image.background", "Background", 255, 255, 255, 0);
		init_colour(ColourId::MapImageLine1S, "map_image.line_1s", "Line (1-Sided)", 0, 0, 0);
		init_colour(ColourId::MapImageLine2S, "map_image.line_2s", "Line (2-Sided)", 144, 144, 144);
		init_colour(ColourId::MapImageLineSpecial, "map_image.line_special", "Line (Special)", 220, 130, 50);
		init_colour(ColourId::MapImageLineMacro, "map_image.line_macro", "Line (Macro)", 50, 130, 220);
	}

	// Map editor
	if (group.empty() || group == "map_editor")
	{
		init_colour(ColourId::MapBackground, "map.background", "Background", 0, 0, 0);
		init_colour(ColourId::MapGrid, "map.grid", "Grid", 50, 80, 170, 60);
		init_colour(ColourId::MapGrid64, "map.grid_64", "Grid (64)", 255, 255, 255, 20, true);
		init_colour(ColourId::MapLineNormal, "map.line_normal", "Line (Normal)", 255, 255, 255);
		init_colour(ColourId::MapLineSpecial, "map.line_special", "Line (Special)", 90, 130, 255);
		init_colour(ColourId::MapLineInvalid, "map.line_invalid", "Line (Invalid)", 255, 0, 0);
		init_colour(ColourId::MapVertex, "map.vertex", "Vertex", 80, 220, 130);
		init_colour(ColourId::MapHighlight, "map.highlight", "Highlighted Item", 255, 255, 0, 255, true);
		init_colour(ColourId::MapSelection, "map.selection", "Selected Item(s)", 0, 100, 255, 180, true);
		init_colour(ColourId::MapTagged, "map.tagged", "Tagged Item(s)", 0, 255, 100, 120, true);
		init_colour(ColourId::MapTagging, "map.tagging", "Tagging Item(s)", 255, 0, 100, 220, true);
		init_colour(ColourId::MapThingPath, "map.thing_path", "Pathed Thing Line", 34, 255, 255);
		init_colour(ColourId::MapThingPathDragon, "map.thing_path_dragon", "Pathed Thing Line (Dragon)", 255, 34, 34);
		init_colour(ColourId::MapLineDraw, "map.linedraw", "Line Drawing", 0, 150, 50);
		init_colour(ColourId::MapMoving, "map.moving", "Moving Item(s)", 255, 100, 0, 255, true);
		init_colour(ColourId::MapObjectEdit, "map.object_edit", "Object Edit", 220, 140, 0, 170, true);
		init_colour(ColourId::MapSelBoxFill, "map.selbox_fill", "Selection Box Fill", 0, 100, 255, 100, true);
		init_colour(ColourId::MapSelBoxOutline, "map.selbox_outline", "Selection Box Outline", 0, 100, 255, 255, true);
		init_colour(ColourId::MapOverlayBackground, "map.overlay_bg", "Info Overlay Background", 0, 0, 0, 180);
		init_colour(ColourId::MapOverlayForeground, "map.overlay_fg", "Info Overlay Text", 255, 255, 255);
		init_colour(ColourId::MapEditorMessage, "map.editor_message", "Editor Message", 255, 200, 100);
		init_colour(ColourId::MapEditorMessageOutline, "map.editor_message_outline", "Editor Message Outline", 0, 0, 0);
		init_colour(ColourId::Map3DCrosshair, "map.3d_crosshair", "3D Crosshair", 180, 190, 255, 255, true);
		init_colour(ColourId::Map3DHighlight, "map.3d_highlight", "3D Highlight", 255, 255, 140, 170);
		init_colour(ColourId::Map3DSelection, "map.3d_selection", "3D Selection", 30, 100, 255, 180);
		init_colour(ColourId::Map3DOverlayBackground, "map.3d_overlay_bg", "3D Info Overlay Background", 0, 0, 0, 180);
		init_colour(ColourId::Map3DOverlayForeground, "map.3d_overlay_fg", "3D Info Overlay Foreground", 255, 255, 255);

		line_hilight_width   = 3.0;
		line_selection_width = 4.0;
		flat_alpha           = 1.0;
	}
}

// -----------------------------------------------------------------------------
// Reads saved [group] colour configuration [name]
// -----------------------------------------------------------------------------
bool colourconfig::readConfiguration(string_view name, string_view group)
{
	return readConfiguration(name, group, cc_colours);
}

// -----------------------------------------------------------------------------
// Reads saved [group] colour configuration [name] into [colours]
// -----------------------------------------------------------------------------
bool colourconfig::readConfiguration(string_view name, string_view group, vector<Colour>& colours)
{
	// Search resource pk3
	auto res = app::programResource();
	auto dir = res->dirAtPath(fmt::format("config/colours/{}", group));
	if (!dir)
		return false;

	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		if (strutil::equalCI(dir->entryAt(a)->nameNoExt(), name))
		{
			if (auto j = jsonutil::parse(dir->entryAt(a)->data()); !j.is_discarded())
				readConfiguration(j, group, colours);
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Adds all available [group] colour configuration names to [names]
// -----------------------------------------------------------------------------
void colourconfig::putConfigurationNames(vector<string>& names, string_view group)
{
	// Search resource pk3
	auto res = app::programResource();
	auto dir = res->dirAtPath(fmt::format("config/colours/{}", group));
	if (!dir)
		return;

	for (unsigned a = 0; a < dir->numEntries(); a++)
		names.emplace_back(dir->entryAt(a)->nameNoExt());
}

const vector<colourconfig::Colour>& colourconfig::allColours()
{
	return cc_colours;
}

vector<const colourconfig::Colour*> colourconfig::mapPreviewColours()
{
	vector<const Colour*> ret;

	constexpr auto first = static_cast<size_t>(ColourId::MapPreview_Start);
	constexpr auto last  = static_cast<size_t>(ColourId::MapPreview_End);
	for (auto i = first; i <= last; ++i)
		ret.emplace_back(&cc_colours[i]);

	return ret;
}

vector<const colourconfig::Colour*> colourconfig::mapImageColours()
{
	vector<const Colour*> ret;

	constexpr auto first = static_cast<size_t>(ColourId::MapImage_Start);
	constexpr auto last  = static_cast<size_t>(ColourId::MapImage_End);
	for (auto i = first; i <= last; ++i)
		ret.emplace_back(&cc_colours[i]);

	return ret;
}

vector<const colourconfig::Colour*> colourconfig::mapEditorColours()
{
	vector<const Colour*> ret;

	constexpr auto first = static_cast<size_t>(ColourId::MapBackground);
	constexpr auto last  = static_cast<size_t>(ColourId::Map3DOverlayForeground);
	for (auto i = first; i <= last; ++i)
		ret.emplace_back(&cc_colours[i]);

	return ret;
}

ColRGBA colourconfig::colour(ColourId id)
{
	return cc_colours[static_cast<size_t>(id)].colour;
}

const colourconfig::Colour& colourconfig::colDef(ColourId id)
{
	return cc_colours[static_cast<size_t>(id)];
}

ColRGBA colourconfig::mapPreviewColour(ColourId id)
{
	ColRGBA fg(wxSystemSettings().GetColour(wxSYS_COLOUR_WINDOWTEXT));
	ColRGBA bg(wxutil::systemPanelBGColour());

	static double factor_line2s = app::isDarkTheme() ? 0.5 : 0.6;

	if (strutil::equalCI(ccfg_map_preview, "default") || ccfg_map_preview.empty())
	{
		// Default map preview colours (based on system theme)
		switch (id)
		{
		case ColourId::MapPreviewLine1S:      return fg;
		case ColourId::MapPreviewLine2S:      return colour::blend(fg, bg, factor_line2s);
		case ColourId::MapPreviewLineSpecial: return ColRGBA{ 130, 140, 255 };
		case ColourId::MapPreviewLineMacro:   return ColRGBA{ 255, 170, 130 };
		case ColourId::MapPreviewThing:       return ColRGBA{ 211, 168, 86 };
		case ColourId::MapPreviewBackground:  return bg;
		default:                              return colour(id);
		}
	}

	return colour(id);
}

void colourconfig::setColour(ColourId id, int red, int green, int blue, int alpha, optional<bool> blend_additive)
{
	auto& col = cc_colours[static_cast<size_t>(id)];

	if (red >= 0)
		col.colour.r = red;
	if (green >= 0)
		col.colour.g = green;
	if (blue >= 0)
		col.colour.b = blue;
	if (alpha >= 0)
		col.colour.a = alpha;
	if (blend_additive)
		col.blend_additive = *blend_additive;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

void           cc_ccfg(const vector<string>& args);
ConsoleCommand c_ccfg("ccfg", &cc_ccfg, 1, false);
void           cc_ccfg(const vector<string>& args)
{
	// Check for 'list'
	if (strutil::equalCI(args[0], "list"))
	{
		// Dump list to console
		log::console(fmt::format("{} Colours:", colourconfig::cc_colours.size()));
		for (const auto& a : colourconfig::cc_colours)
			log::console(a.id);
	}
	else
	{
		// Check for enough args to set the colour
		if (args.size() >= 4)
		{
			// Read RGB
			int red   = strutil::asInt(args[1]);
			int green = strutil::asInt(args[2]);
			int blue  = strutil::asInt(args[3]);

			// Read alpha (if specified)
			int alpha = -1;
			if (args.size() >= 5)
				alpha = strutil::asInt(args[4]);

			// Read blend (if specified)
			int blend = -1;
			if (args.size() >= 6)
				blend = strutil::asInt(args[5]);

			// Set colour
			if (auto col = colourconfig::colourFromId(args[0]); col)
			{
				col->colour.r = red;
				col->colour.g = green;
				col->colour.b = blue;
				if (alpha >= 0)
					col->colour.a = alpha;
				if (blend >= 0)
					col->blend_additive = blend != 0;
			}
		}

		// TODO: Print colour
		if (auto def = colourconfig::colourFromId(args[0]))
			log::console(
				fmt::format(
					"Colour \"{}\" = {} {} {} {} {}",
					args[0],
					def->colour.r,
					def->colour.g,
					def->colour.b,
					def->colour.a,
					def->blend_additive));
	}
}

// CONSOLE_COMMAND(load_ccfg, 1, false)
// {
// 	colourconfig::readConfiguration(args[0]);
// }
#endif
