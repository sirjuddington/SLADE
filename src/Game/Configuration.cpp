
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GameConfiguration.cpp
// Description: GameConfiguration class, handles all game configuration
//              related stuff - action specials, thing types, supported
//              formats, etc.
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
#include "Configuration.h"
#include "ActionSpecial.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Decorate.h"
#include "Game.h"
#include "GenLineSpecial.h"
#include "General/Console.h"
#include "MapInfo.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SpecialPreset.h"
#include "ThingType.h"
#include "UDMFProperty.h"
#include "Utility/Parser.h"
#include "Utility/PropertyUtils.h"
#include "Utility/StringUtils.h"
#include "ZScript.h"

using namespace slade;
using namespace game;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, game_configuration)
EXTERN_CVAR(String, port_configuration)
CVAR(Bool, debug_configuration, false, CVar::Flag::Save)
namespace slade::game
{
Configuration config_current;
}


// -----------------------------------------------------------------------------
//
// Game Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Returns the currently loaded game configuration
// -----------------------------------------------------------------------------
Configuration& game::configuration()
{
	return config_current;
}


// -----------------------------------------------------------------------------
//
// Configuration Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Configuration class constructor
// -----------------------------------------------------------------------------
Configuration::Configuration() : map_info_{ new MapInfo }
{
	setDefaults();
}

// -----------------------------------------------------------------------------
// Configuration class destructor
// -----------------------------------------------------------------------------
Configuration::~Configuration() = default;

// -----------------------------------------------------------------------------
// Resets all game configuration values to defaults
// -----------------------------------------------------------------------------
void Configuration::setDefaults()
{
	udmf_namespace_ = "";
	defaults_line_.clear();
	defaults_side_.clear();
	defaults_sector_.clear();
	defaults_thing_.clear();
	maps_.clear();
	sky_flat_        = "F_SKY1";
	script_language_ = "";
	light_levels_.clear();
	map_formats_.clear();
	boom_sector_flag_start_ = 0;
	supported_features_.fill(false);
	udmf_features_.fill(false);
	special_presets_.clear();
	player_eye_height_ = 41;
}

// -----------------------------------------------------------------------------
// Returns the UDMF namespace for the game configuration
// -----------------------------------------------------------------------------
string Configuration::udmfNamespace() const
{
	return strutil::lower(udmf_namespace_);
}

// -----------------------------------------------------------------------------
// Returns the light level interval for the game configuration
// -----------------------------------------------------------------------------
int Configuration::lightLevelInterval() const
{
	if (light_levels_.empty())
		return 1;
	else
		return light_levels_[1];
}

// -----------------------------------------------------------------------------
// Returns the map name at [index] for the game configuration
// -----------------------------------------------------------------------------
const string& Configuration::mapName(unsigned index)
{
	// Check index
	if (index > maps_.size())
		return strutil::EMPTY;

	return maps_[index].mapname;
}

// -----------------------------------------------------------------------------
// Returns map info for the map matching [name]
// -----------------------------------------------------------------------------
Configuration::MapConf Configuration::mapInfo(string_view mapname)
{
	for (auto& map : maps_)
	{
		if (strutil::equalCI(map.mapname, mapname))
			return map;
	}

	if (!maps_.empty())
		return maps_[0];
	else
		return {};
}

// -----------------------------------------------------------------------------
// Reads action special definitions from a parsed tree [node], using
// [group_defaults] for default values
// -----------------------------------------------------------------------------
void Configuration::readActionSpecials(
	ParseTreeNode*       node,
	Arg::SpecialMap&     shared_args,
	const ActionSpecial* group_defaults)
{
	// Check if we're clearing all existing specials
	if (node->child("clearexisting"))
		action_specials_.clear();

	// Determine current 'group'
	auto   group = node;
	string groupname;
	while (true)
	{
		if (!group || group->name() == "action_specials")
			break;
		else
		{
			// Add current node name to group path
			groupname = fmt::format("{}/{}", group->name(), groupname);
			group     = dynamic_cast<ParseTreeNode*>(group->parent());
		}
	}
	strutil::removeSuffixIP(groupname, '/');

	// --- Set up group default properties ---
	ActionSpecial as_defaults;
	if (group_defaults)
		as_defaults = *group_defaults;
	as_defaults.parse(node, &shared_args);

	// --- Go through all child nodes ---
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->childPTN(a);

		// Check for 'group'
		if (strutil::equalCI(child->type(), "group"))
			readActionSpecials(child, shared_args, &as_defaults);

		// Predeclared argument, for action specials that share the same complex argument
		else if (strutil::equalCI(child->type(), "arg"))
			shared_args[child->name()].parse(child, &shared_args);

		// Action special
		else if (strutil::equalCI(child->type(), "special"))
		{
			// Get special id as integer
			auto special = strutil::asInt(child->name());

			// Reset the action special (in case it's being redefined for whatever reason)
			// action_specials_[special].reset();

			// Apply group defaults
			action_specials_[special] = as_defaults;
			action_specials_[special].setGroup(groupname);

			// Parse it
			action_specials_[special].setNumber(special);
			action_specials_[special].parse(child, &shared_args);
		}
	}
}

// -----------------------------------------------------------------------------
// Reads thing type definitions from a parsed tree [node], using
// [group_defaults] for default values
// -----------------------------------------------------------------------------
void Configuration::readThingTypes(const ParseTreeNode* node, const ThingType* group_defaults)
{
	// Check if we're clearing all existing specials
	if (node->child("clearexisting"))
		thing_types_.clear();

	// --- Determine current 'group' ---
	auto   group = node;
	string groupname;
	while (true)
	{
		if (!group || group->name() == "thing_types")
			break;
		else
		{
			// Add current node name to group path
			groupname = fmt::format("{}/{}", group->name(), groupname);
			group     = dynamic_cast<ParseTreeNode*>(group->parent());
		}
	}
	strutil::removeSuffixIP(groupname, '/');


	// --- Set up group default properties ---
	auto& cur_group_defaults = tt_group_defaults_[groupname];
	cur_group_defaults.define(-1, "", groupname);
	if (group_defaults)
		cur_group_defaults.copy(*group_defaults);
	cur_group_defaults.parse(node);

	// --- Go through all child nodes ---
	ParseTreeNode* child;
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		child = node->childPTN(a);

		// Check for 'group'
		if (strutil::equalCI(child->type(), "group"))
			readThingTypes(child, &cur_group_defaults);

		// Thing type
		else if (strutil::equalCI(child->type(), "thing"))
		{
			// Get thing type as integer
			auto type = strutil::asInt(child->name());

			// Reset the thing type (in case it's being redefined for whatever reason)
			thing_types_[type].reset();

			// Apply group defaults
			thing_types_[type].copy(cur_group_defaults);

			// Check for simple definition
			if (child->isLeaf())
				thing_types_[type].define(type, child->stringValue(), groupname);
			else
			{
				// Extended definition
				thing_types_[type].define(type, "", groupname);
				thing_types_[type].parse(child);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Reads UDMF property definitions from a parsed tree [node] into [plist]
// -----------------------------------------------------------------------------
void Configuration::readUDMFProperties(const ParseTreeNode* block, UDMFPropMap& plist) const
{
	// Read block properties
	for (unsigned a = 0; a < block->nChildren(); a++)
	{
		auto group = block->childPTN(a);

		// Group definition
		if (strutil::equalCI(group->type(), "group"))
		{
			auto groupname = group->name();

			// Go through the group
			for (unsigned b = 0; b < group->nChildren(); b++)
			{
				auto def = group->childPTN(b);

				if (strutil::equalCI(def->type(), "property"))
				{
					// Parse group defaults
					plist[def->name()].parse(group, groupname);

					// Parse definition
					plist[def->name()].parse(def, groupname);
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Reads a game or port definition from a parsed tree [node]. If [port_section]
// is true it is a port definition
// -----------------------------------------------------------------------------
#define SET_UDMF_FEATURE(feature, field) \
	else if (strutil::equalCI(node->name(), #field))(udmf_features_[static_cast<unsigned>(feature)]) = node->boolValue()
#define SET_FEATURE(feature, value) supported_features_[static_cast<unsigned>(feature)] = value
void Configuration::readGameSection(const ParseTreeNode* node_game, bool port_section)
{
	for (unsigned a = 0; a < node_game->nChildren(); a++)
	{
		auto node = node_game->childPTN(a);

		// Allow any map name
		if (strutil::equalCI(node->name(), "map_name_any"))
			SET_FEATURE(Feature::AnyMapName, node->boolValue());

		// Map formats
		else if (strutil::equalCI(node->name(), "map_formats"))
		{
			// Reset supported formats
			map_formats_.clear();

			// Go through values
			for (unsigned v = 0; v < node->nValues(); v++)
			{
				if (strutil::equalCI(node->stringValue(v), "doom"))
				{
					map_formats_[MapFormat::Doom] = true;
				}
				else if (strutil::equalCI(node->stringValue(v), "hexen"))
				{
					map_formats_[MapFormat::Hexen] = true;
				}
				else if (strutil::equalCI(node->stringValue(v), "doom64"))
				{
					map_formats_[MapFormat::Doom64] = true;
				}
				else if (strutil::equalCI(node->stringValue(v), "doom32x"))
				{
					map_formats_[MapFormat::Doom32X] = true;
				}
				else if (strutil::equalCI(node->stringValue(v), "udmf"))
				{
					map_formats_[MapFormat::UDMF] = true;
				}
				else
					log::warning("Unknown/unsupported map format \"{}\"", node->stringValue(v));
			}
		}

		// Boom extensions
		else if (strutil::equalCI(node->name(), "boom"))
			SET_FEATURE(Feature::Boom, node->boolValue());
		else if (strutil::equalCI(node->name(), "boom_sector_flag_start"))
			boom_sector_flag_start_ = node->intValue();

		// MBF21 extensions
		else if (strutil::equalCI(node->name(), "mbf21"))
			SET_FEATURE(Feature::MBF21, node->boolValue());

		// UDMF namespace
		else if (strutil::equalCI(node->name(), "udmf_namespace"))
			udmf_namespace_ = node->stringValue();

		// Mixed Textures and Flats
		else if (strutil::equalCI(node->name(), "mix_tex_flats"))
			SET_FEATURE(Feature::MixTexFlats, node->boolValue());

		// TX_/'textures' namespace enabled
		else if (strutil::equalCI(node->name(), "tx_textures"))
			SET_FEATURE(Feature::TxTextures, node->boolValue());

		// Sky flat
		else if (strutil::equalCI(node->name(), "sky_flat"))
			sky_flat_ = node->stringValue();

		// Scripting language
		else if (strutil::equalCI(node->name(), "script_language"))
			script_language_ = strutil::lower(node->stringValue());

		// Light levels interval
		else if (strutil::equalCI(node->name(), "light_level_interval"))
			setLightLevelInterval(node->intValue());

		// Long names
		else if (strutil::equalCI(node->name(), "long_names"))
			SET_FEATURE(Feature::LongNames, node->boolValue());

		// 3d mode camera eye height
		else if (node->nameIsCI("player_eye_height"))
			player_eye_height_ = node->intValue();

		// UDMF features
		SET_UDMF_FEATURE(UDMFFeature::Slopes, udmf_slopes);                      // UDMF slopes
		SET_UDMF_FEATURE(UDMFFeature::FlatLighting, udmf_flat_lighting);         // UDMF flat lighting
		SET_UDMF_FEATURE(UDMFFeature::FlatPanning, udmf_flat_panning);           // UDMF flat panning
		SET_UDMF_FEATURE(UDMFFeature::FlatRotation, udmf_flat_rotation);         // UDMF flat rotation
		SET_UDMF_FEATURE(UDMFFeature::FlatScaling, udmf_flat_scaling);           // UDMF flat scaling
		SET_UDMF_FEATURE(UDMFFeature::LineTransparency, udmf_line_transparency); // UDMF line transparency
		SET_UDMF_FEATURE(UDMFFeature::SectorColor, udmf_sector_color);           // UDMF sector color
		SET_UDMF_FEATURE(UDMFFeature::SectorFog, udmf_sector_fog);               // UDMF sector fog
		SET_UDMF_FEATURE(UDMFFeature::SideLighting, udmf_side_lighting);         // UDMF per-sidedef lighting
		SET_UDMF_FEATURE(
			UDMFFeature::SideMidtexWrapping, udmf_side_midtex_wrapping);     // UDMF per-sidetex midtex wrapping
		SET_UDMF_FEATURE(UDMFFeature::SideScaling, udmf_side_scaling);       // UDMF per-sidedef scaling
		SET_UDMF_FEATURE(UDMFFeature::TextureScaling, udmf_texture_scaling); // UDMF per-texture scaling
		SET_UDMF_FEATURE(UDMFFeature::TextureOffsets, udmf_texture_offsets); // UDMF per-texture offsets
		SET_UDMF_FEATURE(UDMFFeature::ThingScaling, udmf_thing_scaling);     // UDMF per-thing scaling
		SET_UDMF_FEATURE(UDMFFeature::ThingRotation, udmf_thing_rotation);   // UDMF per-thing pitch and yaw rotation

		// Defaults section
		else if (strutil::equalCI(node->name(), "defaults"))
		{
			// Go through defaults blocks
			for (unsigned b = 0; b < node->nChildren(); b++)
			{
				auto block = node->childPTN(b);

				// Linedef defaults
				if (strutil::equalCI(block->name(), "linedef"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						auto def = block->childPTN(c);
						if (strutil::equalCI(def->type(), "udmf"))
							defaults_line_udmf_[def->name()] = def->value();
						else
							defaults_line_[def->name()] = def->value();
					}
				}

				// Sidedef defaults
				else if (strutil::equalCI(block->name(), "sidedef"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						auto def = block->childPTN(c);
						if (strutil::equalCI(def->type(), "udmf"))
							defaults_side_udmf_[def->name()] = def->value();
						else
							defaults_side_[def->name()] = def->value();
					}
				}

				// Sector defaults
				else if (strutil::equalCI(block->name(), "sector"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						auto def = block->childPTN(c);
						if (strutil::equalCI(def->type(), "udmf"))
							defaults_sector_udmf_[def->name()] = def->value();
						else
							defaults_sector_[def->name()] = def->value();
					}
				}

				// Thing defaults
				else if (strutil::equalCI(block->name(), "thing"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						auto def = block->childPTN(c);
						if (strutil::equalCI(def->type(), "udmf"))
							defaults_thing_udmf_[def->name()] = def->value();
						else
							defaults_thing_[def->name()] = def->value();
					}
				}

				else
					log::warning("Unknown defaults block \"{}\"", block->name());
			}
		}

		// Maps section (game section only)
		else if (strutil::equalCI(node->name(), "maps") && !port_section)
		{
			// Go through map blocks
			for (unsigned b = 0; b < node->nChildren(); b++)
			{
				auto block = node->childPTN(b);

				// Map definition
				if (strutil::equalCI(block->type(), "map"))
				{
					MapConf map;
					map.mapname = block->name();

					// Go through map properties
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						auto prop = block->childPTN(c);

						// Sky texture
						if (strutil::equalCI(prop->name(), "sky"))
						{
							// Primary sky texture
							map.sky1 = prop->stringValue();

							// Secondary sky texture
							if (prop->nValues() > 1)
								map.sky2 = prop->stringValue(1);
						}
					}

					maps_.push_back(map);
				}
			}
		}
	}
}
#undef SET_UDMF_FEATURE
#undef SET_FEATURE

// -----------------------------------------------------------------------------
// Reads a full game configuration from [cfg]
// -----------------------------------------------------------------------------
bool Configuration::readConfiguration(
	string_view cfg,
	string_view source,
	MapFormat   format,
	bool        ignore_game,
	bool        clear)
{
	// Clear current configuration
	if (clear)
	{
		setDefaults();
		action_specials_.clear();
		thing_types_.clear();
		flags_thing_.clear();
		flags_line_.clear();
		sector_types_.clear();
		udmf_vertex_props_.clear();
		udmf_linedef_props_.clear();
		udmf_sidedef_props_.clear();
		udmf_sector_props_.clear();
		udmf_thing_props_.clear();
		tt_group_defaults_.clear();
	}

	// Parse the full configuration
	Parser parser;
	switch (format)
	{
	case MapFormat::Doom:    parser.define("MAP_DOOM"); break;
	case MapFormat::Hexen:   parser.define("MAP_HEXEN"); break;
	case MapFormat::Doom64:  parser.define("MAP_DOOM64"); break;
	case MapFormat::Doom32X: parser.define("MAP_DOOM32X"); break;
	case MapFormat::UDMF:    parser.define("MAP_UDMF"); break;
	default:                 parser.define("MAP_UNKNOWN"); break;
	}
	parser.parseText(cfg, source);

	// Process parsed data
	auto base = parser.parseTreeRoot();

	// Read game/port section(s) if needed
	ParseTreeNode* node_game = nullptr;
	ParseTreeNode* node_port = nullptr;
	if (!ignore_game)
	{
		// 'Game' section (this is required for it to be a valid game configuration, shouldn't be missing)
		for (unsigned a = 0; a < base->nChildren(); a++)
		{
			auto child = base->childPTN(a);
			if (child->type() == "game")
			{
				node_game = child;
				break;
			}
		}
		if (!node_game)
		{
			log::error("No game section found, something is pretty wrong.");
			return false;
		}
		readGameSection(node_game, false);

		// 'Port' section
		for (unsigned a = 0; a < base->nChildren(); a++)
		{
			auto child = base->childPTN(a);
			if (child->type() == "port")
			{
				node_port = child;
				break;
			}
		}
		if (node_port)
			readGameSection(node_port, true);
	}

	// Go through all other config sections
	ParseTreeNode* node = nullptr;
	for (unsigned a = 0; a < base->nChildren(); a++)
	{
		node = base->childPTN(a);

		// Skip read game/port section
		if (node == node_game || node == node_port)
			continue;

		// A TC configuration may override the base game
		if (strutil::equalCI(node->name(), "game"))
			readGameSection(node, false);

		// Action specials section
		else if (strutil::equalCI(node->name(), "action_specials"))
		{
			Arg::SpecialMap sm;
			readActionSpecials(node, sm);
		}

		// Thing types section
		else if (strutil::equalCI(node->name(), "thing_types"))
			readThingTypes(node);

		// Line flags section
		else if (strutil::equalCI(node->name(), "line_flags"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				auto value = node->childPTN(c);

				// Check for 'flag' type
				if (!(strutil::equalCI(value->type(), "flag")))
					continue;

				unsigned long flag_val = 0;
				string        flag_name, flag_udmf;
				bool          activation = false;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->name();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						auto prop = value->childPTN(v);

						if (strutil::equalCI(prop->name(), "value"))
							flag_val = prop->intValue();
						else if (strutil::equalCI(prop->name(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->stringValue(u) + " ";
							flag_udmf.pop_back();
						}
						else if (strutil::equalCI(prop->name(), "activation"))
							activation = prop->boolValue();
					}
				}
				else
				{
					// Short definition
					flag_val  = strutil::asUInt(value->name());
					flag_name = value->stringValue();
				}

				// Check if the flag value already exists
				bool exists = false;
				for (auto& f : flags_line_)
				{
					if (static_cast<unsigned>(f.flag) == flag_val)
					{
						exists = true;
						f.name = flag_name;
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					flags_line_.push_back({ static_cast<int>(flag_val), flag_name, flag_udmf, activation });
			}
		}

		// Line triggers section
		else if (strutil::equalCI(node->name(), "line_triggers"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				auto value = node->childPTN(c);

				// Check for 'trigger' type
				if (!(strutil::equalCI(value->type(), "trigger")))
					continue;

				long   flag_val = 0;
				string flag_name, flag_udmf;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->name();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						auto prop = value->childPTN(v);

						if (strutil::equalCI(prop->name(), "value"))
							flag_val = prop->intValue();
						else if (strutil::equalCI(prop->name(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->stringValue(u) + " ";
							flag_udmf.pop_back();
						}
					}
				}
				else
				{
					// Short definition
					flag_val  = strutil::asInt(value->name());
					flag_name = value->stringValue();
				}

				// Check if the trigger value already exists
				bool exists = false;
				for (auto& f : triggers_line_)
				{
					if (f.flag == flag_val)
					{
						exists = true;
						f.name = flag_name;
						break;
					}
				}

				// Add trigger otherwise
				if (!exists)
					triggers_line_.push_back({ static_cast<int>(flag_val), flag_name, flag_udmf, false });
			}
		}

		// Thing flags section
		else if (strutil::equalCI(node->name(), "thing_flags"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				auto value = node->childPTN(c);

				// Check for 'flag' type
				if (!(strutil::equalCI(value->type(), "flag")))
					continue;

				long   flag_val = 0;
				string flag_name, flag_udmf;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->name();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						auto prop = value->childPTN(v);

						if (strutil::equalCI(prop->name(), "value"))
							flag_val = prop->intValue();
						else if (strutil::equalCI(prop->name(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->stringValue(u) + " ";
							flag_udmf.pop_back();
						}
					}
				}
				else
				{
					// Short definition
					flag_val  = strutil::asInt(value->name());
					flag_name = value->stringValue();
				}

				// Check if the flag value already exists
				bool exists = false;
				for (auto& f : flags_thing_)
				{
					if (f.flag == flag_val)
					{
						exists = true;
						f.name = flag_name;
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					flags_thing_.push_back({ static_cast<int>(flag_val), flag_name, flag_udmf, false });
			}
		}

		// Sector types section
		else if (strutil::equalCI(node->name(), "sector_types"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				auto value = node->childPTN(c);

				// Check for 'type'
				if (!(strutil::equalCI(value->type(), "type")))
					continue;

				// Parse type value
				int type_val = strutil::asInt(value->name());

				// Set type name
				sector_types_[type_val] = value->stringValue();
			}
		}

		// UDMF properties section
		else if (strutil::equalCI(node->name(), "udmf_properties"))
		{
			// Parse vertex block properties (if any)
			auto block = node->childPTN("vertex");
			if (block)
				readUDMFProperties(block, udmf_vertex_props_);

			// Parse linedef block properties (if any)
			block = node->childPTN("linedef");
			if (block)
				readUDMFProperties(block, udmf_linedef_props_);

			// Parse sidedef block properties (if any)
			block = node->childPTN("sidedef");
			if (block)
				readUDMFProperties(block, udmf_sidedef_props_);

			// Parse sector block properties (if any)
			block = node->childPTN("sector");
			if (block)
				readUDMFProperties(block, udmf_sector_props_);

			// Parse thing block properties (if any)
			block = node->childPTN("thing");
			if (block)
				readUDMFProperties(block, udmf_thing_props_);
		}

		// Special Presets section
		else if (strutil::equalCI(node->name(), "special_presets"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				auto preset = node->childPTN(c);
				if (strutil::equalCI(preset->type(), "preset"))
				{
					special_presets_.push_back({});
					special_presets_.back().parse(preset);
				}
			}
		}

		// Unknown/unexpected section
		else
			log::warning("Unexpected game configuration section \"{}\", skipping", node->name());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Opens the full game configuration [game]+[port], either from the user dir or
// program resource
// -----------------------------------------------------------------------------
bool Configuration::openConfig(const string& game, const string& port, MapFormat format)
{
	string full_config;

	// Get game configuration as string
	auto& game_config = gameDef(game);
	if (game_config.name == game)
	{
		if (game_config.user)
		{
			// Config is in user dir
			auto filename = app::path("games/", app::Dir::User) + game_config.filename + ".cfg";
			if (wxFileExists(filename))
				strutil::processIncludes(filename, full_config);
			else
			{
				log::error("Error: Game configuration file \"{}\" not found", filename);
				return false;
			}
		}
		else
		{
			// Config is in program resource
			auto epath   = fmt::format("config/games/{}.cfg", game_config.filename);
			auto archive = app::archiveManager().programResourceArchive();
			auto entry   = archive->entryAtPath(epath);
			if (entry)
				strutil::processIncludes(entry, full_config);
		}
	}

	// Append port configuration (if specified)
	if (!port.empty())
	{
		full_config += "\n\n";

		// Check the port supports this game
		auto& conf = portDef(port);
		if (conf.supportsGame(game))
		{
			if (conf.user)
			{
				// Config is in user dir
				auto filename = app::path("ports/", app::Dir::User) + conf.filename + ".cfg";
				if (wxFileExists(filename))
					strutil::processIncludes(filename, full_config);
				else
				{
					log::error("Error: Port configuration file \"{}\" not found", filename);
					return false;
				}
			}
			else
			{
				// Config is in program resource
				auto epath   = fmt::format("config/ports/{}.cfg", conf.filename);
				auto archive = app::archiveManager().programResourceArchive();
				auto entry   = archive->entryAtPath(epath);
				if (entry)
					strutil::processIncludes(entry, full_config);
			}
		}
	}

	if (debug_configuration)
	{
		wxFile test("full.cfg", wxFile::write);
		test.Write(full_config);
		test.Close();
	}

	// Read fully built configuration
	bool ok = true;
	if (readConfiguration(full_config, "full.cfg", format))
	{
		current_game_      = game;
		current_port_      = port;
		game_configuration = game;
		port_configuration = port;
		log::info(2, R"(Read game configuration "{}" + "{}")", current_game_, current_port_);
	}
	else
	{
		log::error("Error reading game configuration, not loaded");
		ok = false;
	}

	// Read any embedded configurations in resource archives
	ArchiveSearchOptions opt;
	opt.match_name   = "sladecfg";
	auto cfg_entries = app::archiveManager().findAllResourceEntries(opt);
	for (auto& cfg_entry : cfg_entries)
	{
		// Log message
		auto parent = cfg_entry->parent();
		if (parent)
			log::info("Reading SLADECFG in {}", parent->filename());

		// Read embedded config
		string config{ reinterpret_cast<const char*>(cfg_entry->rawData()), cfg_entry->size() };
		if (!readConfiguration(config, cfg_entry->name(), format, true, false))
			log::error("Error reading embedded game configuration, not loaded");
	}

	return ok;
}

// -----------------------------------------------------------------------------
// Returns the action special definition for [id]
// -----------------------------------------------------------------------------
const ActionSpecial& Configuration::actionSpecial(unsigned id)
{
	auto& as = action_specials_[id];

	// Defined Action Special
	if (as.defined())
		return as;

	// Boom Generalised Special
	if (featureSupported(Feature::Boom) && id >= 0x2f80)
	{
		if ((id & 7) >= 6)
			return ActionSpecial::generalManual();
		else
			return ActionSpecial::generalSwitched();
	}

	// Unknown
	return ActionSpecial::unknown();
}

// -----------------------------------------------------------------------------
// Returns the action special name for [special], if any
// -----------------------------------------------------------------------------
string Configuration::actionSpecialName(int special)
{
	// Check special id is valid
	if (special < 0)
		return "Unknown";
	else if (special == 0)
		return "None";

	if (action_specials_[special].defined())
		return action_specials_[special].name();
	else if (special >= 0x2F80 && featureSupported(Feature::Boom))
		return genlinespecial::parseLineType(special);
	else
		return "Unknown";
}

// -----------------------------------------------------------------------------
// Returns the thing type definition for [type]
// -----------------------------------------------------------------------------
const ThingType& Configuration::thingType(unsigned type)
{
	auto& ttype = thing_types_[type];
	if (ttype.defined())
		return ttype;
	else
		return ThingType::unknown();
}

// -----------------------------------------------------------------------------
// Returns the default ThingType properties for [group]
// -----------------------------------------------------------------------------
const ThingType& Configuration::thingTypeGroupDefaults(const string& group)
{
	return tt_group_defaults_[group];
}

// -----------------------------------------------------------------------------
// Returns the name of the thing flag at [index]
// -----------------------------------------------------------------------------
string Configuration::thingFlag(unsigned flag_index)
{
	// Check index
	if (flag_index >= flags_thing_.size())
		return "";

	return flags_thing_[flag_index].name;
}

// -----------------------------------------------------------------------------
// Returns true if the flag at [index] is set for [thing]
// -----------------------------------------------------------------------------
bool Configuration::thingFlagSet(unsigned flag_index, const MapThing* thing) const
{
	// Check index
	if (flag_index >= flags_thing_.size())
		return false;

	// Check if flag is set
	return thing->flagSet(flags_thing_[flag_index].flag);
}

// -----------------------------------------------------------------------------
// Returns true if the flag matching [flag] is set for [thing]
// -----------------------------------------------------------------------------
bool Configuration::thingFlagSet(string_view udmf_name, MapThing* thing, MapFormat map_format) const
{
	// If UDMF, just get the bool value
	if (map_format == MapFormat::UDMF)
		return thing->boolProperty(udmf_name);

	// Iterate through flags
	for (auto& i : flags_thing_)
	{
		if (i.udmf == udmf_name)
			return thing->flagSet(i.flag);
	}
	log::warning(2, "Flag {} does not exist in this configuration", udmf_name);
	return false;
}

// -----------------------------------------------------------------------------
// Returns true if the basic flag matching [flag] is set for [thing]
// -----------------------------------------------------------------------------
bool Configuration::thingBasicFlagSet(string_view flag, MapThing* thing, MapFormat map_format) const
{
	// If UDMF, just get the bool value
	if (map_format == MapFormat::UDMF)
		return thing->boolProperty(flag);

	// Hexen-style flags in Hexen-format maps
	bool hexen = map_format == MapFormat::Hexen;

	// Easy Skill
	if (flag == "skill2" || flag == "skill1")
		return thing->flagSet(1);

	// Medium Skill
	else if (flag == "skill3")
		return thing->flagSet(2);

	// Hard Skill
	else if (flag == "skill4" || flag == "skill5")
		return thing->flagSet(4);

	// Game mode flags
	else if (flag == "single")
	{
		// Single Player
		if (hexen)
			return thing->flagSet(256);
		// *Not* Multiplayer
		else
			return !thing->flagSet(16);
	}
	else if (flag == "coop")
	{
		// Coop
		if (hexen)
			return thing->flagSet(512);
		// *Not* Not In Coop
		else if (featureSupported(Feature::Boom))
			return !thing->flagSet(64);
		else
			return true;
	}
	else if (flag == "dm")
	{
		// Deathmatch
		if (hexen)
			return thing->flagSet(1024);
		// *Not* Not In DM
		else if (featureSupported(Feature::Boom))
			return !thing->flagSet(32);
		else
			return true;
	}

	// Hexen class flags
	else if (hexen && strutil::startsWith(flag, "class"))
	{
		// Fighter
		if (flag == "class1")
			return thing->flagSet(32);
		// Cleric
		else if (flag == "class2")
			return thing->flagSet(64);
		// Mage
		else if (flag == "class3")
			return thing->flagSet(128);
	}

	// Not basic
	return thingFlagSet(flag, thing, map_format);
}

// -----------------------------------------------------------------------------
// Returns a string of all thing flags set in [flags]
// -----------------------------------------------------------------------------
string Configuration::thingFlagsString(int flags) const
{
	// Check against all flags
	string ret;
	for (auto& a : flags_thing_)
	{
		if (flags & a.flag)
		{
			// Add flag name to string
			ret += a.name;
			ret += ", ";
		}
	}

	// Remove ending ', ' if needed
	if (!ret.empty())
		strutil::removeLastIP(ret, 2);
	else
		return "None";

	return ret;
}

// -----------------------------------------------------------------------------
// Sets thing flag at [index] for [thing]. If [set] is false, the flag is unset
// -----------------------------------------------------------------------------
void Configuration::setThingFlag(unsigned flag_index, MapThing* thing, bool set) const
{
	// Check index
	if (flag_index >= flags_thing_.size())
		return;

	if (set)
		thing->setFlag(flags_thing_[flag_index].flag);
	else
		thing->clearFlag(flags_thing_[flag_index].flag);
}

// -----------------------------------------------------------------------------
// Sets thing flag matching [flag] (UDMF name) for [thing].
// If [set] is false, the flag is unset
// -----------------------------------------------------------------------------
void Configuration::setThingFlag(string_view udmf_name, MapThing* thing, MapFormat map_format, bool set) const
{
	// If UDMF, just set the bool value
	if (map_format == MapFormat::UDMF)
	{
		thing->setBoolProperty(udmf_name, set);
		return;
	}

	// Iterate through flags
	unsigned long flag_val = 0;
	for (auto& i : flags_thing_)
	{
		if (i.udmf == udmf_name)
		{
			flag_val = i.flag;
			break;
		}
	}

	if (flag_val == 0)
	{
		log::warning(2, "Flag {} does not exist in this configuration", udmf_name);
		return;
	}

	// Update thing flags
	if (set)
		thing->setFlag(flag_val);
	else
		thing->clearFlag(flag_val);
}

// -----------------------------------------------------------------------------
// Sets thing basic flag matching [flag] for [thing].
// If [set] is false, the flag is unset
// -----------------------------------------------------------------------------
void Configuration::setThingBasicFlag(string_view flag, MapThing* thing, MapFormat map_format, bool set) const
{
	// If UDMF, just set the bool value
	if (map_format == MapFormat::UDMF)
	{
		thing->setBoolProperty(flag, set);
		return;
	}

	// Seek flag value
	unsigned long flag_val = 0;

	// ZDoom uses Hexen-style flags
	bool hexen = (currentGame() == "hexen") || (currentPort() == "zdoom");

	// Easy Skill
	if (flag == "skill2" || flag == "skill1")
		flag_val = 1;

	// Medium Skill
	else if (flag == "skill3")
		flag_val = 2;

	// Hard Skill
	else if (flag == "skill4" || flag == "skill5")
		flag_val = 4;

	// Game mode flags
	else if (flag == "single")
	{
		// Single Player
		if (hexen)
			flag_val = 256;
		// *Not* Multiplayer
		else
		{
			flag_val = 16;
			set      = !set;
		}
	}
	else if (flag == "coop")
	{
		// Coop
		if (hexen)
			flag_val = 512;
		// *Not* Not In Coop
		else if (featureSupported(Feature::Boom))
		{
			flag_val = 64;
			set      = !set;
		}
		// Multiplayer
		else
			flag_val = 0;
	}
	else if (flag == "dm")
	{
		// Deathmatch
		if (hexen)
			flag_val = 1024;
		// *Not* Not In DM
		else if (featureSupported(Feature::Boom))
		{
			flag_val = 32;
			set      = !set;
		}
		// Multiplayer
		else
			flag_val = 0;
	}

	// Hexen class flags
	else if (strutil::startsWith(flag, "class"))
	{
		if (hexen)
		{
			// Fighter
			if (flag == "class1")
				flag_val = 32;
			// Cleric
			else if (flag == "class2")
				flag_val = 64;
			// Mage
			else if (flag == "class3")
				flag_val = 128;
		}
		else
			flag_val = 0;
	}

	if (flag_val)
	{
		// Update thing flags
		if (set)
			thing->setFlag(flag_val);
		else
			thing->clearFlag(flag_val);

		return;
	}

	// Not basic
	thingFlagSet(flag, thing, map_format);
}

// -----------------------------------------------------------------------------
// Parses all DECORATE thing definitions in [archive]
// -----------------------------------------------------------------------------
bool Configuration::parseDecorateDefs(Archive* archive)
{
	return readDecorateDefs(archive, thing_types_, parsed_types_);
}

// -----------------------------------------------------------------------------
// Removes any thing definitions parsed from DECORATE entries
// -----------------------------------------------------------------------------
void Configuration::clearDecorateDefs() const
{
	for (auto def : thing_types_)
		if (def.second.decorate() && def.second.defined())
			def.second.define(-1, "", "");
}

// -----------------------------------------------------------------------------
// Configuration::importZScriptDefs
//
// Imports parsed classes from ZScript [defs] as thing types
// -----------------------------------------------------------------------------
void Configuration::importZScriptDefs(zscript::Definitions& defs)
{
	defs.exportThingTypes(thing_types_, parsed_types_);
}

// -----------------------------------------------------------------------------
// Parses all *MAPINFO definitions in [archive]
// -----------------------------------------------------------------------------
bool Configuration::parseMapInfo(const Archive& archive) const
{
	return map_info_->readMapInfo(archive);
}

// -----------------------------------------------------------------------------
// Clear all parsed *MAPINFO definitions
// -----------------------------------------------------------------------------
void Configuration::clearMapInfo() const
{
	map_info_->clear();
}

// -----------------------------------------------------------------------------
// Attempts to find editor numbers in *MAPINFO for parsed DECORATE/ZScript types
// that were not given one along with their definition
// -----------------------------------------------------------------------------
void Configuration::linkDoomEdNums()
{
	for (auto& parsed : parsed_types_)
	{
		// Find MAPINFO editor number for parsed actor class
		int ednum = map_info_->doomEdNumForClass(parsed.className());

		if (ednum >= 0)
		{
			// Editor number found, copy the definition to thing types map
			thing_types_[ednum].define(ednum, parsed.name(), parsed.group());
			thing_types_[ednum].copy(parsed);
			log::info(2, "Linked parsed class {} to DoomEdNum {}", parsed.className(), ednum);
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the name of the line flag at [index]
// -----------------------------------------------------------------------------
const Configuration::Flag& Configuration::lineFlag(unsigned flag_index)
{
	// Check index
	static Flag invalid{ 0, "", "", false };
	if (flag_index >= flags_line_.size())
		return invalid;

	return flags_line_[flag_index];
}

// -----------------------------------------------------------------------------
// Returns true if the flag at [index] is set for [line]
// -----------------------------------------------------------------------------
bool Configuration::lineFlagSet(unsigned flag_index, const MapLine* line) const
{
	// Check index
	if (flag_index >= flags_line_.size())
		return false;

	// Check if flag is set
	return line->flagSet(flags_line_[flag_index].flag);
}

// -----------------------------------------------------------------------------
// Returns true if the flag matching [flag] (UDMF name) is set for [line]
// -----------------------------------------------------------------------------
bool Configuration::lineFlagSet(string_view udmf_name, MapLine* line, MapFormat map_format) const
{
	// If UDMF, just get the bool value
	if (map_format == MapFormat::UDMF)
		return line->boolProperty(udmf_name);

	// Get current flags
	unsigned long flags = line->flags();

	// Iterate through flags
	for (auto& i : flags_line_)
	{
		if (i.udmf == udmf_name)
			return !!(flags & i.flag);
	}
	log::warning(2, "Flag {} does not exist in this configuration", udmf_name);
	return false;
}

// -----------------------------------------------------------------------------
// Returns true if the basic flag matching [flag] (UDMF name) is set for [line]
// 'Basic' flags are flags that are available in some way or another in all
// game configurations
// -----------------------------------------------------------------------------
bool Configuration::lineBasicFlagSet(string_view flag, MapLine* line, MapFormat map_format) const
{
	// If UDMF, just get the bool value
	if (map_format == MapFormat::UDMF)
		return line->boolProperty(flag);

	// Impassable
	if (flag == "blocking")
		return line->flagSet(1);

	// Two Sided
	if (flag == "twosided")
		return line->flagSet(4);

	// Upper unpegged
	if (flag == "dontpegtop")
		return line->flagSet(8);

	// Lower unpegged
	if (flag == "dontpegbottom")
		return line->flagSet(16);

	// Not basic
	return lineFlagSet(flag, line, map_format);
}

// -----------------------------------------------------------------------------
// Returns a string containing all flags set on [line]
// -----------------------------------------------------------------------------
string Configuration::lineFlagsString(const MapLine* line) const
{
	if (!line)
		return "None";

	// TODO: UDMF flags

	// Check against all flags
	string ret;
	for (auto& flag : flags_line_)
	{
		if (line->flagSet(flag.flag))
		{
			// Add flag name to string
			ret += flag.name;
			ret += ", ";
		}
	}

	// Remove ending ', ' if needed
	if (!ret.empty())
		strutil::removeLastIP(ret, 2);
	else
		ret = "None";

	return ret;
}

// -----------------------------------------------------------------------------
// Sets line flag at [index] for [line]. If [set] is false, the flag is unset
// -----------------------------------------------------------------------------
void Configuration::setLineFlag(unsigned flag_index, MapLine* line, bool set) const
{
	// Check index
	if (flag_index >= flags_line_.size())
		return;

	if (set)
		line->setFlag(flags_line_[flag_index].flag);
	else
		line->clearFlag(flags_line_[flag_index].flag);
}

// -----------------------------------------------------------------------------
// Sets line flag matching [flag] (UDMF name) for [line].
// If [set] is false, the flag is unset
// -----------------------------------------------------------------------------
void Configuration::setLineFlag(string_view udmf_name, MapLine* line, MapFormat map_format, bool set) const
{
	// If UDMF, just set the bool value
	if (map_format == MapFormat::UDMF)
	{
		line->setBoolProperty(udmf_name, set);
		return;
	}

	// Iterate through flags
	int flag_val = 0;
	for (auto& i : flags_line_)
	{
		if (i.udmf == udmf_name)
		{
			flag_val = i.flag;
			break;
		}
	}

	if (flag_val == 0)
	{
		log::warning(2, "Flag {} does not exist in this configuration", udmf_name);
		return;
	}

	// Determine new flags value
	if (set)
		line->setFlag(flag_val);
	else
		line->clearFlag(flag_val);
}

// -----------------------------------------------------------------------------
// Sets line basic flag [flag] (UDMF name) for [line].
// If [set] is false, the flag is unset
// -----------------------------------------------------------------------------
void Configuration::setLineBasicFlag(string_view flag, MapLine* line, MapFormat map_format, bool set) const
{
	// If UDMF, just set the bool value
	if (map_format == MapFormat::UDMF)
	{
		line->setBoolProperty(flag, set);
		return;
	}

	int fval = 0;

	// Impassable
	if (flag == "blocking")
		fval = 1;

	// Two Sided
	else if (flag == "twosided")
		fval = 4;

	// Upper unpegged
	else if (flag == "dontpegtop")
		fval = 8;

	// Lower unpegged
	else if (flag == "dontpegbottom")
		fval = 16;

	// Set/unset flag
	if (fval)
	{
		if (set)
			line->setFlag(fval);
		else
			line->clearFlag(fval);
	}
	// Not basic
	else
		setLineFlag(flag, line, map_format, set);
}

// -----------------------------------------------------------------------------
// Returns the hexen SPAC trigger for [line] as a string
// -----------------------------------------------------------------------------
string Configuration::spacTriggerString(MapLine* line, MapFormat map_format)
{
	if (!line)
		return "None";

	// Hexen format
	if (map_format == MapFormat::Hexen)
	{
		// Get raw flags
		int flags = line->flags();

		// Get SPAC trigger value from flags
		int trigger = ((flags & 0x1c00) >> 10);

		// Find matching trigger name
		for (auto& a : triggers_line_)
		{
			if (a.flag == trigger)
				return a.name;
		}
	}

	// UDMF format
	else if (map_format == MapFormat::UDMF)
	{
		// Go through all line UDMF properties
		string trigger;
		auto&  props = allUDMFProperties(MapObject::Type::Line);
		for (auto& prop : props)
		{
			// Check for trigger property
			if (prop.second.isTrigger())
			{
				// Check if the line has this property
				if (line->boolProperty(prop.second.propName()))
				{
					// Add to trigger line
					if (!trigger.empty())
						trigger += ", ";
					trigger += prop.second.name();
				}
			}
		}

		// Check if there was any trigger
		if (trigger.empty())
			return "None";
		else
			return trigger;
	}

	// Unknown trigger
	return "Unknown";
}

// -----------------------------------------------------------------------------
// Returns the hexen SPAC trigger index for [line]
// -----------------------------------------------------------------------------
int Configuration::spacTriggerIndexHexen(const MapLine* line) const
{
	// Get raw flags
	int flags = line->flags();

	// Get SPAC trigger value from flags
	int trigger = ((flags & 0x1c00) >> 10);

	// Find matching trigger name
	for (unsigned a = 0; a < triggers_line_.size(); a++)
	{
		if (triggers_line_[a].flag == trigger)
			return a;
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Returns a list of all defined SPAC triggers
// -----------------------------------------------------------------------------
vector<string> Configuration::allSpacTriggers() const
{
	vector<string> ret;

	for (auto& a : triggers_line_)
		ret.push_back(a.name);

	return ret;
}

// -----------------------------------------------------------------------------
// Sets the SPAC trigger for [line] to the trigger at [index]
// -----------------------------------------------------------------------------
void Configuration::setLineSpacTrigger(unsigned trigger_index, MapLine* line) const
{
	// Check index
	if (trigger_index >= triggers_line_.size())
		return;

	// Get trigger value
	int trigger = triggers_line_[trigger_index].flag;

	// Get raw line flags
	int flags = line->flags();

	// Apply trigger to flags
	trigger = trigger << 10;
	flags &= ~0x1c00;
	flags |= trigger;

	// Update line flags
	line->setFlags(flags);
}

// -----------------------------------------------------------------------------
// Returns the UDMF name for the SPAC trigger at [index]
// -----------------------------------------------------------------------------
const string& Configuration::spacTriggerUDMFName(unsigned trigger_index)
{
	// Check index
	if (trigger_index >= triggers_line_.size())
		return strutil::EMPTY;

	return triggers_line_[trigger_index].udmf;
}

// -----------------------------------------------------------------------------
// Returns the UDMF property definition matching [name] for MapObject [type]
// -----------------------------------------------------------------------------
UDMFProperty* Configuration::getUDMFProperty(const string& name, MapObject::Type type)
{
	using Type = MapObject::Type;

	UDMFPropMap* udmf_props;

	switch (type)
	{
	case Type::Vertex: udmf_props = &udmf_vertex_props_; break;
	case Type::Line:   udmf_props = &udmf_linedef_props_; break;
	case Type::Side:   udmf_props = &udmf_sidedef_props_; break;
	case Type::Sector: udmf_props = &udmf_sector_props_; break;
	case Type::Thing:  udmf_props = &udmf_thing_props_; break;
	default:           return nullptr;
	}

	if (udmf_props->count(name) == 1)
		return &(*udmf_props)[name];
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Returns all defined UDMF properties for MapObject type [type]
// -----------------------------------------------------------------------------
UDMFPropMap& Configuration::allUDMFProperties(MapObject::Type type)
{
	static UDMFPropMap map_invalid_type;

	switch (type)
	{
	case MapObject::Type::Vertex: return udmf_vertex_props_;
	case MapObject::Type::Line:   return udmf_linedef_props_;
	case MapObject::Type::Side:   return udmf_sidedef_props_;
	case MapObject::Type::Sector: return udmf_sector_props_;
	case MapObject::Type::Thing:  return udmf_thing_props_;
	default:                      return map_invalid_type;
	}
}

// -----------------------------------------------------------------------------
// Returns all defined UDMF properties for MapObject type [type], in the order
// they were defined in the configuration
// -----------------------------------------------------------------------------
vector<std::pair<const string, UDMFProperty>*> Configuration::sortedUDMFProperties(MapObject::Type type)
{
	auto&                                          all_props = allUDMFProperties(type);
	vector<std::pair<const string, UDMFProperty>*> sorted_props;
	sorted_props.reserve(all_props.size());
	for (auto& prop : all_props)
	{
		sorted_props.push_back(&prop);
	}
	std::sort(
		sorted_props.begin(),
		sorted_props.end(),
		[](const auto& a, const auto& b) { return a->second.order() < b->second.order(); });
	return sorted_props;
}

// -----------------------------------------------------------------------------
// Removes any UDMF properties in [object] that have default values
// (so they are not written to the UDMF map unnecessarily)
// -----------------------------------------------------------------------------
void Configuration::cleanObjectUDMFProps(MapObject* object)
{
	using namespace property;

	// Get UDMF properties list for type
	UDMFPropMap* map;
	switch (object->objType())
	{
	case MapObject::Type::Vertex: map = &udmf_vertex_props_; break;
	case MapObject::Type::Line:   map = &udmf_linedef_props_; break;
	case MapObject::Type::Side:   map = &udmf_sidedef_props_; break;
	case MapObject::Type::Sector: map = &udmf_sector_props_; break;
	case MapObject::Type::Thing:  map = &udmf_thing_props_; break;
	default:                      return;
	}

	// Go through properties
	for (const auto& i : *map)
	{
		const auto& name      = i.first;
		const auto& udmf_prop = i.second;

		// Check if the object even has this property
		if (!object->hasProp(name))
			continue;

		// Remove the property from the object if it is the default value
		const auto& default_val = udmf_prop.defaultValue();
		switch (valueType(default_val))
		{
		case ValueType::Bool:
			if (udmf_prop.isDefault<bool>(object->boolProperty(name)))
				object->props().remove(name);
			break;
		case ValueType::Int:
			if (udmf_prop.isDefault<int>(object->intProperty(name)))
				object->props().remove(name);
			break;
		case ValueType::Float:
			if (udmf_prop.isDefault<double>(object->floatProperty(name)))
				object->props().remove(name);
			break;
		case ValueType::String:
			if (udmf_prop.isDefault<string>(object->stringProperty(name)))
				object->props().remove(name);
			break;
		default: break;
		}
	}
}

// -----------------------------------------------------------------------------
// Returns the name for sector type value [type], taking generalised types into
// account
// -----------------------------------------------------------------------------
string Configuration::sectorTypeName(int type)
{
	// Check for zero type
	if (type == 0)
		return "Normal";

	// Deal with generalised flags
	vector<string> gen_flags;
	if (supportsSectorFlags() && type >= boom_sector_flag_start_)
	{
		// Damage flags
		int damage = sectorBoomDamage(type);
		if (sectorMBF21AltDamageMode(type))
		{
			// MBF21 alternate damage mode flags
			if (damage == 0)
				gen_flags.emplace_back("Instantly Kill Player w/o Radsuit or Invuln");
			else if (damage == 1)
				gen_flags.emplace_back("Instantly Kill Player");
			else if (damage == 2)
				gen_flags.emplace_back("Kill All Players, Exit Map (Normal Exit)");
			else if (damage == 3)
				gen_flags.emplace_back("Kill All Players, Exit Map (Secret Exit)");
		}
		else
		{
			// Standard Boom damage flags
			if (damage == 1)
				gen_flags.emplace_back("5% Damage");
			else if (damage == 2)
				gen_flags.emplace_back("10% Damage");
			else if (damage == 3)
				gen_flags.emplace_back("20% Damage");
		}

		// Secret
		if (sectorBoomSecret(type))
			gen_flags.emplace_back("Secret");

		// Friction
		if (sectorBoomFriction(type))
			gen_flags.emplace_back("Friction Enabled");

		// Pushers/Pullers
		if (sectorBoomPushPull(type))
			gen_flags.emplace_back("Pushers/Pullers Enabled");

		// Kill Grounded Monsters
		if (sectorMBF21KillGroundedMonsters(type))
			gen_flags.emplace_back("Kill Grounded Monsters");

		// Remove flag bits from type value
		type = type & (boom_sector_flag_start_ - 1);
	}

	// Check if the type only has generalised flags
	if (type == 0 && !gen_flags.empty())
	{
		// Just return flags in this case
		auto name = gen_flags[0];
		for (unsigned a = 1; a < gen_flags.size(); a++)
			name += fmt::format(" + {}", gen_flags[a]);

		return name;
	}

	// Get base type name
	auto name = sector_types_[type];
	if (name.empty())
		name = "Unknown";

	// Add generalised flags to type name
	for (const auto& gen_flag : gen_flags)
		name += fmt::format(" + {}", gen_flag);

	return name;
}

// -----------------------------------------------------------------------------
// Returns the sector type value matching [name]
// -----------------------------------------------------------------------------
int Configuration::sectorTypeByName(string_view name) const
{
	for (auto& i : sector_types_)
		if (i.second == name)
			return i.first;

	return 0;
}

// -----------------------------------------------------------------------------
// Returns the 'base' sector type for value [type]
// (strips generalised flags/type)
// -----------------------------------------------------------------------------
int Configuration::baseSectorType(int type) const
{
	// No type
	if (type == 0)
		return 0;

	// Strip boom flags depending on map format
	if (supportsSectorFlags())
		return type & (boom_sector_flag_start_ - 1);

	// No flags
	return type;
}

// -----------------------------------------------------------------------------
// Returns the generalised 'damage' flag for [type]: 0=none, 1=5%, 2=10% 3=20%
// -----------------------------------------------------------------------------
int Configuration::sectorBoomDamage(int type) const
{
	if (!supportsSectorFlags())
		return 0;

	// No type
	if (type == 0)
		return 0;

	int low_bit  = boom_sector_flag_start_ << 0;
	int high_bit = boom_sector_flag_start_ << 1;

	if ((type & (low_bit | high_bit)) == (low_bit | high_bit))
		return 3;
	else if (type & low_bit)
		return 1;
	else if (type & high_bit)
		return 2;

	// No damage
	return 0;
}

// -----------------------------------------------------------------------------
// Returns true if the generalised 'secret' flag is set for [type]
// -----------------------------------------------------------------------------
bool Configuration::sectorBoomSecret(int type) const
{
	if (!supportsSectorFlags())
		return false;

	// No type
	if (type == 0)
		return false;

	if (type & (boom_sector_flag_start_ << 2))
		return true;

	// Not secret
	return false;
}

// -----------------------------------------------------------------------------
// Returns true if the generalised 'friction' flag is set for [type]
// -----------------------------------------------------------------------------
bool Configuration::sectorBoomFriction(int type) const
{
	if (!supportsSectorFlags())
		return false;

	// No type
	if (type == 0)
		return false;

	if (type & (boom_sector_flag_start_ << 3))
		return true;

	// Friction disabled
	return false;
}

// -----------------------------------------------------------------------------
// Returns true if the generalised 'pusher/puller' flag is set for [type]
// -----------------------------------------------------------------------------
bool Configuration::sectorBoomPushPull(int type) const
{
	if (!supportsSectorFlags())
		return false;

	// No type
	if (type == 0)
		return false;

	if (type & (boom_sector_flag_start_ << 4))
		return true;

	// Pusher/Puller disabled
	return false;
}

// -----------------------------------------------------------------------------
// Returns true if the MBF21 generalised 'alternate damage mode' flag is set for
// [type]
// -----------------------------------------------------------------------------
bool Configuration::sectorMBF21AltDamageMode(int type) const
{
	if (!(supportsSectorFlags() && featureSupported(Feature::MBF21)))
		return false;

	// No type
	if (type == 0)
		return false;

	if (type & (boom_sector_flag_start_ << 7))
		return true;

	// Alternate damage mode disabled
	return false;
}

// -----------------------------------------------------------------------------
// Returns true if the MBF21 generalised 'kill grounded monsters' flag is set
// for [type]
// -----------------------------------------------------------------------------
bool Configuration::sectorMBF21KillGroundedMonsters(int type) const
{
	if (!(supportsSectorFlags() && featureSupported(Feature::MBF21)))
		return false;

	// No type
	if (type == 0)
		return false;

	if (type & (boom_sector_flag_start_ << 8))
		return true;

	// Kill grounded monsters disabled
	return false;
}

// -----------------------------------------------------------------------------
// Returns the generalised boom sector type built from parameters
// -----------------------------------------------------------------------------
int Configuration::boomSectorType(
	int  base,
	int  damage,
	bool secret,
	bool friction,
	bool pushpull,
	bool alt_damage,
	bool kill_grounded) const
{
	int fulltype = base;

	// Damage
	fulltype += damage * boom_sector_flag_start_;

	// Secret
	if (secret)
		fulltype += boom_sector_flag_start_ << 2;

	// Friction
	if (friction)
		fulltype += boom_sector_flag_start_ << 3;

	// Pusher/Puller
	if (pushpull)
		fulltype += boom_sector_flag_start_ << 4;

	if (featureSupported(Feature::MBF21))
	{
		// MBF21 Alternate Damage Mode
		if (alt_damage)
			fulltype += boom_sector_flag_start_ << 7;

		// MBF21 Kill Grounded Monsters
		if (kill_grounded)
			fulltype += boom_sector_flag_start_ << 8;
	}

	return fulltype;
}

// -----------------------------------------------------------------------------
// Returns the default string value for [property] of MapObject type [type]
// -----------------------------------------------------------------------------
string Configuration::defaultString(MapObject::Type type, const string& property) const
{
	switch (type)
	{
	case MapObject::Type::Line:   return defaults_line_.getOr<string>(property, {});
	case MapObject::Type::Side:   return defaults_side_.getOr<string>(property, {});
	case MapObject::Type::Sector: return defaults_sector_.getOr<string>(property, {});
	case MapObject::Type::Thing:  return defaults_thing_.getOr<string>(property, {});
	default:                      return {};
	}
}

// -----------------------------------------------------------------------------
// Returns the default int value for [property] of MapObject type [type]
// -----------------------------------------------------------------------------
int Configuration::defaultInt(MapObject::Type type, const string& property) const
{
	switch (type)
	{
	case MapObject::Type::Line:   return defaults_line_.getOr<int>(property, 0);
	case MapObject::Type::Side:   return defaults_side_.getOr<int>(property, 0);
	case MapObject::Type::Sector: return defaults_sector_.getOr<int>(property, 0);
	case MapObject::Type::Thing:  return defaults_thing_.getOr<int>(property, 0);
	default:                      return 0;
	}
}

// -----------------------------------------------------------------------------
// Returns the default float value for [property] of MapObject type [type]
// -----------------------------------------------------------------------------
double Configuration::defaultFloat(MapObject::Type type, const string& property) const
{
	switch (type)
	{
	case MapObject::Type::Line:   return defaults_line_.getOr(property, 0.);
	case MapObject::Type::Side:   return defaults_side_.getOr(property, 0.);
	case MapObject::Type::Sector: return defaults_sector_.getOr(property, 0.);
	case MapObject::Type::Thing:  return defaults_thing_.getOr(property, 0.);
	default:                      return 0.;
	}
}

// -----------------------------------------------------------------------------
// Returns the default boolean value for [property] of MapObject type [type]
// -----------------------------------------------------------------------------
bool Configuration::defaultBool(MapObject::Type type, const string& property) const
{
	switch (type)
	{
	case MapObject::Type::Line:   return defaults_line_.getOr(property, false);
	case MapObject::Type::Side:   return defaults_side_.getOr(property, false);
	case MapObject::Type::Sector: return defaults_sector_.getOr(property, false);
	case MapObject::Type::Thing:  return defaults_thing_.getOr(property, false);
	default:                      return false;
	}
}

// -----------------------------------------------------------------------------
// Apply defined default values to [object]
// -----------------------------------------------------------------------------
void Configuration::applyDefaults(MapObject* object, bool udmf) const
{
	// Get all defaults for the object type
	vector<string>   prop_names;
	vector<Property> prop_vals;

	// Line defaults
	if (object->objType() == MapObject::Type::Line)
	{
		defaults_line_.allProperties(prop_vals);
		defaults_line_.allPropertyNames(prop_names);
		if (udmf)
		{
			defaults_line_udmf_.allProperties(prop_vals);
			defaults_line_udmf_.allPropertyNames(prop_names);
		}
	}

	// Side defaults
	else if (object->objType() == MapObject::Type::Side)
	{
		defaults_side_.allProperties(prop_vals);
		defaults_side_.allPropertyNames(prop_names);
		if (udmf)
		{
			defaults_side_udmf_.allProperties(prop_vals);
			defaults_side_udmf_.allPropertyNames(prop_names);
		}
	}

	// Sector defaults
	else if (object->objType() == MapObject::Type::Sector)
	{
		defaults_sector_.allProperties(prop_vals);
		defaults_sector_.allPropertyNames(prop_names);
		if (udmf)
		{
			defaults_sector_udmf_.allProperties(prop_vals);
			defaults_sector_udmf_.allPropertyNames(prop_names);
		}
	}

	// Thing defaults
	else if (object->objType() == MapObject::Type::Thing)
	{
		defaults_thing_.allProperties(prop_vals);
		defaults_thing_.allPropertyNames(prop_names);
		if (udmf)
		{
			defaults_thing_udmf_.allProperties(prop_vals);
			defaults_thing_udmf_.allPropertyNames(prop_names);
		}
	}

	// Apply defaults to object
	for (unsigned a = 0; a < prop_names.size(); a++)
	{
		switch (property::valueType(prop_vals[a]))
		{
		case property::ValueType::Bool: object->setBoolProperty(prop_names[a], std::get<bool>(prop_vals[a])); break;
		case property::ValueType::Int:  object->setIntProperty(prop_names[a], std::get<int>(prop_vals[a])); break;
		case property::ValueType::UInt:
			object->setIntProperty(prop_names[a], std::get<unsigned int>(prop_vals[a]));
			break;
		case property::ValueType::Float: object->setFloatProperty(prop_names[a], std::get<double>(prop_vals[a])); break;
		case property::ValueType::String:
			object->setStringProperty(prop_names[a], std::get<string>(prop_vals[a]));
			break;
		default: break;
		}

		// log::info(3, "Applied default property {} = {}", prop_names[a], prop_vals[a].stringValue());
	}
}

// -----------------------------------------------------------------------------
// Builds the array of valid light levels from [interval]
// -----------------------------------------------------------------------------
void Configuration::setLightLevelInterval(int interval)
{
	// Clear current
	light_levels_.clear();

	// Fill light levels array
	int light = 0;
	while (light < 255)
	{
		light_levels_.push_back(light);
		light += interval;
	}
	light_levels_.push_back(255);
}

// -----------------------------------------------------------------------------
// Returns [light_level] incremented to the next 'valid' light level
// (defined by the game light interval)
// -----------------------------------------------------------------------------
int Configuration::upLightLevel(int light_level) const
{
	// No defined levels
	if (light_levels_.empty())
		return light_level;

	for (int a = 0; a < static_cast<int>(light_levels_.size()) - 1; a++)
	{
		if (light_level >= light_levels_[a] && light_level < light_levels_[a + 1])
			return light_levels_[a + 1];
	}

	return light_levels_.back();
}

// -----------------------------------------------------------------------------
// Returns [light_level] decremented to the next 'valid' light level
// (defined by the game light interval)
// -----------------------------------------------------------------------------
int Configuration::downLightLevel(int light_level) const
{
	// No defined levels
	if (light_levels_.empty())
		return light_level;

	for (int a = 0; a < static_cast<int>(light_levels_.size()) - 1; a++)
	{
		if (light_level > light_levels_[a] && light_level <= light_levels_[a + 1])
			return light_levels_[a];
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Dumps all defined action specials to the log
// -----------------------------------------------------------------------------
void Configuration::dumpActionSpecials() const
{
	for (auto& i : action_specials_)
		log::info("Action special {} = {}", i.first, i.second.stringDesc());
}

// -----------------------------------------------------------------------------
// Dumps all defined thing types to the log
// -----------------------------------------------------------------------------
void Configuration::dumpThingTypes() const
{
	for (auto& i : thing_types_)
		if (i.second.defined())
			log::info("Thing type {} = {}", i.first, i.second.stringDesc());
}

// -----------------------------------------------------------------------------
// Dumps all defined map names to the log
// -----------------------------------------------------------------------------
void Configuration::dumpValidMapNames() const
{
	log::info("Valid Map Names:");
	for (auto& map : maps_)
		log::info(map.mapname);
}

// -----------------------------------------------------------------------------
// Dumps all defined UDMF properties to the log
// -----------------------------------------------------------------------------
void Configuration::dumpUDMFProperties()
{
	// Vertex
	log::info("\nVertex properties:");
	for (auto& i : udmf_vertex_props_)
		log::info(i.second.getStringRep());

	// Line
	log::info("\nLine properties:");
	for (auto& i : udmf_linedef_props_)
		log::info(i.second.getStringRep());

	// Side
	log::info("\nSide properties:");
	for (auto& i : udmf_sidedef_props_)
		log::info(i.second.getStringRep());

	// Sector
	log::info("\nSector properties:");
	for (auto& i : udmf_sector_props_)
		log::info(i.second.getStringRep());

	// Thing
	log::info("\nThing properties:");
	for (auto& i : udmf_thing_props_)
		log::info(i.second.getStringRep());
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

CONSOLE_COMMAND(testgc, 0, false)
{
	string game = "doomu";

	if (!args.empty())
		game = args[0];

	game::configuration().openConfig(game);
}

CONSOLE_COMMAND(dumpactionspecials, 0, false)
{
	game::configuration().dumpActionSpecials();
}

CONSOLE_COMMAND(dumpudmfprops, 0, false)
{
	game::configuration().dumpUDMFProperties();
}

CONSOLE_COMMAND(dumpthingtypes, 0, false)
{
	game::configuration().dumpThingTypes();
}

CONSOLE_COMMAND(dumpspecialpresets, 0, false)
{
	for (auto& preset : game::configuration().specialPresets())
		log::console(fmt::format("{}/{}", preset.group, preset.name));
}
