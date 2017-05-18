
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "Configuration.h"
#include "General/Console/Console.h"
#include "GenLineSpecial.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include "Decorate.h"

using namespace Game;


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(String, game_configuration)
EXTERN_CVAR(String, port_configuration)
CVAR(Bool, debug_configuration, false, CVAR_SAVE)


// ----------------------------------------------------------------------------
//
// Configuration Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Configuration::Configuration
//
// Configuration class constructor
// ----------------------------------------------------------------------------
Configuration::Configuration()
{
	setDefaults();
}

// ----------------------------------------------------------------------------
// Configuration::~Configuration
//
// Configuration class destructor
// ----------------------------------------------------------------------------
Configuration::~Configuration()
{
}

// ----------------------------------------------------------------------------
// Configuration::setDefaults
//
// Resets all game configuration values to defaults
// ----------------------------------------------------------------------------
void Configuration::setDefaults()
{
	udmf_namespace_ = "";
	defaults_line_.clear();
	defaults_side_.clear();
	defaults_sector_.clear();
	defaults_thing_.clear();
	maps_.clear();
	sky_flat_ = "F_SKY1";
	script_language_ = "";
	light_levels_.clear();
	for (int a = 0; a < 4; a++)
		map_formats_[a] = false;
	boom_sector_flag_start_ = 0;
	supported_features_.clear();
	udmf_features_.clear();
}

// ----------------------------------------------------------------------------
// Configuration::udmfNamespace
//
// Returns the UDMF namespace for the game configuration
// ----------------------------------------------------------------------------
string Configuration::udmfNamespace()
{
	return udmf_namespace_.Lower();
}

// ----------------------------------------------------------------------------
// Configuration::lightLevelInterval
//
// Returns the light level interval for the game configuration
// ----------------------------------------------------------------------------
int Configuration::lightLevelInterval()
{
	if (light_levels_.size() == 0)
		return 1;
	else
		return light_levels_[1];
}

// ----------------------------------------------------------------------------
// Configuration::readConfigName
//
// Parses the game configuration definition in [mc] and returns the
// configuration name
// ----------------------------------------------------------------------------
string Configuration::readConfigName(MemChunk& mc)
{
	Tokenizer tz;
	tz.openMem(&mc, "gameconfig");

	// Parse text
	string token = tz.getToken();
	while (!token.IsEmpty())
	{
		// Game section
		if (S_CMPNOCASE(token, "game"))
		{
			tz.getToken();	// Skip {

			token = tz.getToken();
			while (token != "}")
			{
				// Config name
				if (S_CMPNOCASE(token, "name"))
				{
					tz.getToken();	// Skip =
					return tz.getToken();
				}

				token = tz.getToken();
			}
		}

		token = tz.getToken();
	}

	// Name not found (invalid config?)
	return "";
}

// ----------------------------------------------------------------------------
// Configuration::mapName
//
// Returns the map name at [index] for the game configuration
// ----------------------------------------------------------------------------
string Configuration::mapName(unsigned index)
{
	// Check index
	if (index > maps_.size())
		return "";

	return maps_[index].mapname;
}

// ----------------------------------------------------------------------------
// Configuration::mapInfo
//
// Returns map info for the map matching [name]
// ----------------------------------------------------------------------------
gc_mapinfo_t Configuration::mapInfo(string name)
{
	for (unsigned a = 0; a < maps_.size(); a++)
	{
		if (maps_[a].mapname == name)
			return maps_[a];
	}

	if (maps_.size() > 0)
		return maps_[0];
	else
		return gc_mapinfo_t();
}

// ----------------------------------------------------------------------------
// Configuration::readActionSpecials
//
// Reads action special definitions from a parsed tree [node], using
// [group_defaults] for default values
// ----------------------------------------------------------------------------
void Configuration::readActionSpecials(
	ParseTreeNode* node,
	Arg::SpecialMap& shared_args,
	ActionSpecial* group_defaults)
{
	// Check if we're clearing all existing specials
	if (node->getChild("clearexisting"))
		action_specials_.clear();

	// Determine current 'group'
	ParseTreeNode* group = node;
	string groupname = "";
	while (true)
	{
		if (!group || group->getName() == "action_specials")
			break;
		else
		{
			// Add current node name to group path
			groupname.Prepend(group->getName() + "/");
			group = (ParseTreeNode*)group->getParent();
		}
	}
	if (groupname.EndsWith("/"))
		groupname.RemoveLast();	// Remove last '/'

	// --- Set up group default properties ---
	ActionSpecial as_defaults;
	if (group_defaults) as_defaults = *group_defaults;
	as_defaults.parse(node, &shared_args);

	// --- Go through all child nodes ---
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);

		// Check for 'group'
		if (S_CMPNOCASE(child->getType(), "group"))
			readActionSpecials(child, shared_args, &as_defaults);

		// Predeclared argument, for action specials that share the same complex argument
		else if (S_CMPNOCASE(child->getType(), "arg"))
			shared_args[child->getName()].parse(child, &shared_args);

		// Action special
		else if (S_CMPNOCASE(child->getType(), "special"))
		{
			// Get special id as integer
			long special;
			child->getName().ToLong(&special);

			// Reset the action special (in case it's being redefined for whatever reason)
			//action_specials_[special].reset();

			// Apply group defaults
			action_specials_[special] = as_defaults;
			action_specials_[special].setGroup(groupname);

			// Parse it
			action_specials_[special].setNumber(special);
			action_specials_[special].parse(child, &shared_args);
		}
	}
}

// ----------------------------------------------------------------------------
// Configuration::readThingTypes
//
// Reads thing type definitions from a parsed tree [node], using
// [group_defaults] for default values
// ----------------------------------------------------------------------------
void Configuration::readThingTypes(ParseTreeNode* node, const ThingType& group_defaults)
{
	// Check if we're clearing all existing specials
	if (node->getChild("clearexisting"))
		thing_types_.clear();

	// --- Determine current 'group' ---
	ParseTreeNode* group = node;
	string groupname = "";
	while (true)
	{
		if (!group || group->getName() == "thing_types")
			break;
		else
		{
			// Add current node name to group path
			groupname.Prepend(group->getName() + "/");
			group = (ParseTreeNode*)group->getParent();
		}
	}
	if (groupname.EndsWith("/"))
		groupname.RemoveLast();	// Remove last '/'


	// --- Set up group default properties ---
	auto& cur_group_defaults = tt_group_defaults_[groupname];
	cur_group_defaults.define(-1, "", groupname);
	if (&group_defaults != &ThingType::unknown())
		cur_group_defaults.copy(group_defaults);
	cur_group_defaults.parse(node);

	// --- Go through all child nodes ---
	ParseTreeNode* child = nullptr;
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		child = node->getChildPTN(a);

		// Check for 'group'
		if (S_CMPNOCASE(child->getType(), "group"))
			readThingTypes(child, cur_group_defaults);

		// Thing type
		else if (S_CMPNOCASE(child->getType(), "thing"))
		{
			// Get thing type as integer
			long type;
			child->getName().ToLong(&type);

			// Reset the thing type (in case it's being redefined for whatever reason)
			thing_types_[type].reset();

			// Apply group defaults
			thing_types_[type].copy(cur_group_defaults);

			// Check for simple definition
			if (child->isLeaf())
				thing_types_[type].define(type, child->getStringValue(), groupname);
			else
			{
				// Extended definition
				thing_types_[type].define(type, "", groupname);
				thing_types_[type].parse(child);
			}
		}
	}
}

// ----------------------------------------------------------------------------
// Configuration::readUDMFProperties
//
// Reads UDMF property definitions from a parsed tree [node] into [plist]
// ----------------------------------------------------------------------------
void Configuration::readUDMFProperties(ParseTreeNode* block, UDMFPropMap& plist)
{
	// Read block properties
	for (unsigned a = 0; a < block->nChildren(); a++)
	{
		ParseTreeNode* group = (ParseTreeNode*)block->getChild(a);

		// Group definition
		if (S_CMPNOCASE(group->getType(), "group"))
		{
			string groupname = group->getName();

			// Go through the group
			for (unsigned b = 0; b < group->nChildren(); b++)
			{
				ParseTreeNode* def = (ParseTreeNode*)group->getChild(b);

				if (S_CMPNOCASE(def->getType(), "property"))
				{
					// Create property if needed
					if (!plist[def->getName()].property)
						plist[def->getName()].property = new UDMFProperty();

					// Parse group defaults
					plist[def->getName()].property->parse(group, groupname);

					// Parse definition
					plist[def->getName()].property->parse(def, groupname);

					// Set index
					plist[def->getName()].index = plist.size();
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------
// Configuration::readGameSection
//
// Reads a game or port definition from a parsed tree [node]. If [port_section]
// is true it is a port definition
// ----------------------------------------------------------------------------
#define READ_BOOL(obj, field)	else if (S_CMPNOCASE(node->getName(), #field)) obj = node->getBoolValue()
void Configuration::readGameSection(ParseTreeNode* node_game, bool port_section)
{
	for (unsigned a = 0; a < node_game->nChildren(); a++)
	{
		ParseTreeNode* node = (ParseTreeNode*)node_game->getChild(a);

		//// Game name
		//if (S_CMPNOCASE(node->getName(), "name"))
		//	this->name = node->getStringValue();

		// Allow any map name
		if (S_CMPNOCASE(node->getName(), "map_name_any"))
			supported_features_[Feature::AnyMapName] = node->getBoolValue();

		// Map formats
		else if (S_CMPNOCASE(node->getName(), "map_formats"))
		{
			// Reset supported formats
			for (unsigned f = 0; f < 4; f++)
				map_formats_[f] = false;

			// Go through values
			for (unsigned v = 0; v < node->nValues(); v++)
			{
				if (S_CMPNOCASE(node->getStringValue(v), "doom"))
				{
					map_formats_[MAP_DOOM] = true;
				}
				else if (S_CMPNOCASE(node->getStringValue(v), "hexen"))
				{
					map_formats_[MAP_HEXEN] = true;
				}
				else if (S_CMPNOCASE(node->getStringValue(v), "doom64"))
				{
					map_formats_[MAP_DOOM64] = true;
				}
				else if (S_CMPNOCASE(node->getStringValue(v), "udmf"))
				{
					map_formats_[MAP_UDMF] = true;
				}
				else
					LOG_MESSAGE(1, "Warning: Unknown/unsupported map format \"%s\"", node->getStringValue(v));
			}
		}

		// Boom extensions
		else if (S_CMPNOCASE(node->getName(), "boom"))
			supported_features_[Feature::Boom] = node->getBoolValue();
		else if (S_CMPNOCASE(node->getName(), "boom_sector_flag_start"))
			boom_sector_flag_start_ = node->getIntValue();

		// UDMF namespace
		else if (S_CMPNOCASE(node->getName(), "udmf_namespace"))
			udmf_namespace_ = node->getStringValue();

		// Mixed Textures and Flats
		else if (S_CMPNOCASE(node->getName(), "mix_tex_flats"))
			supported_features_[Feature::MixTexFlats] = node->getBoolValue();

		// TX_/'textures' namespace enabled
		else if (S_CMPNOCASE(node->getName(), "tx_textures"))
			supported_features_[Feature::TxTextures] = node->getBoolValue();

		// Sky flat
		else if (S_CMPNOCASE(node->getName(), "sky_flat"))
			sky_flat_ = node->getStringValue();

		// Scripting language
		else if (S_CMPNOCASE(node->getName(), "script_language"))
			script_language_ = node->getStringValue().Lower();

		// Light levels interval
		else if (S_CMPNOCASE(node->getName(), "light_level_interval"))
			setLightLevelInterval(node->getIntValue());

		// Long names
		else if (S_CMPNOCASE(node->getName(), "long_names"))
			supported_features_[Feature::LongNames] = node->getBoolValue();

		READ_BOOL(udmf_features_[UDMFFeature::Slopes], udmf_slopes); // UDMF slopes
		READ_BOOL(udmf_features_[UDMFFeature::FlatLighting], udmf_flat_lighting); // UDMF flat lighting
		READ_BOOL(udmf_features_[UDMFFeature::FlatPanning], udmf_flat_panning); // UDMF flat panning
		READ_BOOL(udmf_features_[UDMFFeature::FlatRotation], udmf_flat_rotation); // UDMF flat rotation
		READ_BOOL(udmf_features_[UDMFFeature::FlatScaling], udmf_flat_scaling); // UDMF flat scaling
		READ_BOOL(udmf_features_[UDMFFeature::LineTransparency], udmf_line_transparency); // UDMF line transparency
		READ_BOOL(udmf_features_[UDMFFeature::SectorColor], udmf_sector_color); // UDMF sector color
		READ_BOOL(udmf_features_[UDMFFeature::SectorFog], udmf_sector_fog); // UDMF sector fog
		READ_BOOL(udmf_features_[UDMFFeature::SideLighting], udmf_side_lighting); // UDMF per-sidedef lighting
		READ_BOOL(udmf_features_[UDMFFeature::SideMidtexWrapping], udmf_side_midtex_wrapping); // UDMF per-sidetex midtex wrapping
		READ_BOOL(udmf_features_[UDMFFeature::SideScaling], udmf_side_scaling); // UDMF per-sidedef scaling
		READ_BOOL(udmf_features_[UDMFFeature::TextureScaling], udmf_texture_scaling); // UDMF per-texture scaling
		READ_BOOL(udmf_features_[UDMFFeature::TextureOffsets], udmf_texture_offsets); // UDMF per-texture offsets
		READ_BOOL(udmf_features_[UDMFFeature::ThingScaling], udmf_thing_scaling); // UDMF per-thing scaling
		READ_BOOL(udmf_features_[UDMFFeature::ThingRotation], udmf_thing_rotation); // UDMF per-thing pitch and yaw rotation

		// Defaults section
		else if (S_CMPNOCASE(node->getName(), "defaults"))
		{
			// Go through defaults blocks
			for (unsigned b = 0; b < node->nChildren(); b++)
			{
				ParseTreeNode* block = (ParseTreeNode*)node->getChild(b);

				// Linedef defaults
				if (S_CMPNOCASE(block->getName(), "linedef"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						if (S_CMPNOCASE(def->getType(), "udmf"))
							defaults_line_udmf_[def->getName()] = def->getValue();
						else	
							defaults_line_[def->getName()] = def->getValue();
					}
				}

				// Sidedef defaults
				else if (S_CMPNOCASE(block->getName(), "sidedef"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						if (S_CMPNOCASE(def->getType(), "udmf"))
							defaults_side_udmf_[def->getName()] = def->getValue();
						else
							defaults_side_[def->getName()] = def->getValue();
					}
				}

				// Sector defaults
				else if (S_CMPNOCASE(block->getName(), "sector"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						if (S_CMPNOCASE(def->getType(), "udmf"))
							defaults_sector_udmf_[def->getName()] = def->getValue();
						else
							defaults_sector_[def->getName()] = def->getValue();
					}
				}

				// Thing defaults
				else if (S_CMPNOCASE(block->getName(), "thing"))
				{
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* def = (ParseTreeNode*)block->getChild(c);
						if (S_CMPNOCASE(def->getType(), "udmf"))
							defaults_thing_udmf_[def->getName()] = def->getValue();
						else
							defaults_thing_[def->getName()] = def->getValue();
					}
				}

				else
					LOG_MESSAGE(1, "Unknown defaults block \"%s\"", block->getName());
			}
		}

		// Maps section (game section only)
		else if (S_CMPNOCASE(node->getName(), "maps") && !port_section)
		{
			// Go through map blocks
			for (unsigned b = 0; b < node->nChildren(); b++)
			{
				ParseTreeNode* block = (ParseTreeNode*)node->getChild(b);

				// Map definition
				if (S_CMPNOCASE(block->getType(), "map"))
				{
					gc_mapinfo_t map;
					map.mapname = block->getName();

					// Go through map properties
					for (unsigned c = 0; c < block->nChildren(); c++)
					{
						ParseTreeNode* prop = (ParseTreeNode*)block->getChild(c);

						// Sky texture
						if (S_CMPNOCASE(prop->getName(), "sky"))
						{
							// Primary sky texture
							map.sky1 = prop->getStringValue();

							// Secondary sky texture
							if (prop->nValues() > 1)
								map.sky2 = prop->getStringValue(1);
						}
					}

					maps_.push_back(map);
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------
// Configuration::readConfiguration
//
// Reads a full game configuration from [cfg]
// ----------------------------------------------------------------------------
bool Configuration::readConfiguration(string& cfg, string source, uint8_t format, bool ignore_game, bool clear)
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
	case MAP_DOOM:		parser.define("MAP_DOOM");		break;
	case MAP_HEXEN:		parser.define("MAP_HEXEN");		break;
	case MAP_DOOM64:	parser.define("MAP_DOOM64");	break;
	case MAP_UDMF:		parser.define("MAP_UDMF");		break;
	default:			parser.define("MAP_UNKNOWN");	break;
	}
	parser.parseText(cfg, source);

	// Process parsed data
	ParseTreeNode* base = parser.parseTreeRoot();

	// Read game/port section(s) if needed
	ParseTreeNode* node_game = nullptr;
	ParseTreeNode* node_port = nullptr;
	if (!ignore_game)
	{
		// 'Game' section (this is required for it to be a valid game configuration, shouldn't be missing)
		for (unsigned a = 0; a < base->nChildren(); a++)
		{
			ParseTreeNode* child = (ParseTreeNode*)base->getChild(a);
			if (child->getType() == "game")
			{
				node_game = child;
				break;
			}
		}
		if (!node_game)
		{
			LOG_MESSAGE(1, "No game section found, something is pretty wrong.");
			return false;
		}
		readGameSection(node_game, false);

		// 'Port' section
		for (unsigned a = 0; a < base->nChildren(); a++)
		{
			ParseTreeNode* child = (ParseTreeNode*)base->getChild(a);
			if (child->getType() == "port")
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
		node = (ParseTreeNode*)base->getChild(a);

		// Skip read game/port section
		if (node == node_game || node == node_port)
			continue;

		// A TC configuration may override the base game
		if (S_CMPNOCASE(node->getName(), "game"))
			readGameSection(node, false);

		// Action specials section
		else if (S_CMPNOCASE(node->getName(), "action_specials"))
		{
			Arg::SpecialMap sm;
			readActionSpecials(node, sm);
		}

		// Thing types section
		else if (S_CMPNOCASE(node->getName(), "thing_types"))
			readThingTypes(node);

		// Line flags section
		else if (S_CMPNOCASE(node->getName(), "line_flags"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'flag' type
				if (!(S_CMPNOCASE(value->getType(), "flag")))
					continue;

				unsigned long flag_val;
				string flag_name, flag_udmf;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->getName();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						ParseTreeNode* prop = (ParseTreeNode*)value->getChild(v);

						if (S_CMPNOCASE(prop->getName(), "value"))
							flag_val = prop->getIntValue();
						else if (S_CMPNOCASE(prop->getName(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->getStringValue(u) + " ";
							flag_udmf.RemoveLast(1);
						}
					}
				}
				else
				{
					// Short definition
					value->getName().ToULong(&flag_val);
					flag_name = value->getStringValue();
				}

				// Check if the flag value already exists
				bool exists = false;
				for (unsigned f = 0; f < flags_line_.size(); f++)
				{
					if (flags_line_[f].flag == flag_val)
					{
						exists = true;
						flags_line_[f].name = flag_name;
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					flags_line_.push_back(flag_t(flag_val, flag_name, flag_udmf));
			}
		}

		// Line triggers section
		else if (S_CMPNOCASE(node->getName(), "line_triggers"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'trigger' type
				if (!(S_CMPNOCASE(value->getType(), "trigger")))
					continue;

				long flag_val;
				string flag_name, flag_udmf;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->getName();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						ParseTreeNode* prop = (ParseTreeNode*)value->getChild(v);

						if (S_CMPNOCASE(prop->getName(), "value"))
							flag_val = prop->getIntValue();
						else if (S_CMPNOCASE(prop->getName(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->getStringValue(u) + " ";
							flag_udmf.RemoveLast(1);
						}
					}
				}
				else
				{
					// Short definition
					value->getName().ToLong(&flag_val);
					flag_name = value->getStringValue();
				}

				// Check if the trigger value already exists
				bool exists = false;
				for (unsigned f = 0; f < triggers_line_.size(); f++)
				{
					if (triggers_line_[f].flag == flag_val)
					{
						exists = true;
						triggers_line_[f].name = flag_name;
						break;
					}
				}

				// Add trigger otherwise
				if (!exists)
					triggers_line_.push_back(flag_t(flag_val, flag_name, flag_udmf));
			}
		}

		// Thing flags section
		else if (S_CMPNOCASE(node->getName(), "thing_flags"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'flag' type
				if (!(S_CMPNOCASE(value->getType(), "flag")))
					continue;

				long flag_val;
				string flag_name, flag_udmf;

				if (value->nValues() == 0)
				{
					// Full definition
					flag_name = value->getName();

					for (unsigned v = 0; v < value->nChildren(); v++)
					{
						ParseTreeNode* prop = (ParseTreeNode*)value->getChild(v);

						if (S_CMPNOCASE(prop->getName(), "value"))
							flag_val = prop->getIntValue();
						else if (S_CMPNOCASE(prop->getName(), "udmf"))
						{
							for (unsigned u = 0; u < prop->nValues(); u++)
								flag_udmf += prop->getStringValue(u) + " ";
							flag_udmf.RemoveLast(1);
						}
					}
				}
				else
				{
					// Short definition
					value->getName().ToLong(&flag_val);
					flag_name = value->getStringValue();
				}

				// Check if the flag value already exists
				bool exists = false;
				for (unsigned f = 0; f < flags_thing_.size(); f++)
				{
					if (flags_thing_[f].flag == flag_val)
					{
						exists = true;
						flags_thing_[f].name = flag_name;
						break;
					}
				}

				// Add flag otherwise
				if (!exists)
					flags_thing_.push_back(flag_t(flag_val, flag_name, flag_udmf));
			}
		}

		// Sector types section
		else if (S_CMPNOCASE(node->getName(), "sector_types"))
		{
			for (unsigned c = 0; c < node->nChildren(); c++)
			{
				ParseTreeNode* value = (ParseTreeNode*)node->getChild(c);

				// Check for 'type'
				if (!(S_CMPNOCASE(value->getType(), "type")))
					continue;

				long type_val;
				value->getName().ToLong(&type_val);

				// Check if the sector type already exists
				bool exists = false;
				for (unsigned t = 0; t < sector_types_.size(); t++)
				{
					if (sector_types_[t].type == type_val)
					{
						exists = true;
						sector_types_[t].name = value->getStringValue();
						break;
					}
				}

				// Add type otherwise
				if (!exists)
					sector_types_.push_back(sectype_t(type_val, value->getStringValue()));
			}
		}

		// UDMF properties section
		else if (S_CMPNOCASE(node->getName(), "udmf_properties"))
		{
			// Parse vertex block properties (if any)
			ParseTreeNode* block = (ParseTreeNode*)node->getChild("vertex");
			if (block) readUDMFProperties(block, udmf_vertex_props_);

			// Parse linedef block properties (if any)
			block = (ParseTreeNode*)node->getChild("linedef");
			if (block) readUDMFProperties(block, udmf_linedef_props_);

			// Parse sidedef block properties (if any)
			block = (ParseTreeNode*)node->getChild("sidedef");
			if (block) readUDMFProperties(block, udmf_sidedef_props_);

			// Parse sector block properties (if any)
			block = (ParseTreeNode*)node->getChild("sector");
			if (block) readUDMFProperties(block, udmf_sector_props_);

			// Parse thing block properties (if any)
			block = (ParseTreeNode*)node->getChild("thing");
			if (block) readUDMFProperties(block, udmf_thing_props_);
		}

		// Unknown/unexpected section
		else
			LOG_MESSAGE(1, "Warning: Unexpected game configuration section \"%s\", skipping", node->getName());
	}

	return true;
}

// ----------------------------------------------------------------------------
// Configuration::openConfig
//
// Opens the full game configuration [game]+[port], either from the user dir or
// program resource
// ----------------------------------------------------------------------------
bool Configuration::openConfig(string game, string port, uint8_t format)
{
	string full_config;

	// Get game configuration as string
	auto& game_config = gameDef(game);
	if (game_config.name == game)
	{
		if (game_config.user)
		{
			// Config is in user dir
			string filename = App::path("games/", App::Dir::User) + game_config.filename + ".cfg";
			if (wxFileExists(filename))
				StringUtils::processIncludes(filename, full_config);
			else
			{
				LOG_MESSAGE(1, "Error: Game configuration file \"%s\" not found", filename);
				return false;
			}
		}
		else
		{
			// Config is in program resource
			string epath = S_FMT("config/games/%s.cfg", game_config.filename);
			Archive* archive = theArchiveManager->programResourceArchive();
			ArchiveEntry* entry = archive->entryAtPath(epath);
			if (entry)
				StringUtils::processIncludes(entry, full_config);
		}
	}

	// Append port configuration (if specified)
	if (!port.IsEmpty())
	{
		full_config += "\n\n";

		// Check the port supports this game
		auto& conf = portDef(port);
		if (conf.supportsGame(game))
		{
			if (conf.user)
			{
				// Config is in user dir
				string filename = App::path("games/", App::Dir::User) + conf.filename + ".cfg";
				if (wxFileExists(filename))
					StringUtils::processIncludes(filename, full_config);
				else
				{
					LOG_MESSAGE(1, "Error: Port configuration file \"%s\" not found", filename);
					return false;
				}
			}
			else
			{
				// Config is in program resource
				string epath = S_FMT("config/ports/%s.cfg", conf.filename);
				Archive* archive = theArchiveManager->programResourceArchive();
				ArchiveEntry* entry = archive->entryAtPath(epath);
				if (entry)
					StringUtils::processIncludes(entry, full_config);
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
		current_game_ = game;
		current_port_ = port;
		game_configuration = game;
		port_configuration = port;
		LOG_MESSAGE(1, "Read game configuration \"%s\" + \"%s\"", current_game_, current_port_);
	}
	else
	{
		LOG_MESSAGE(1, "Error reading game configuration, not loaded");
		ok = false;
	}

	// Read any embedded configurations in resource archives
	Archive::search_options_t opt;
	opt.match_name = "sladecfg";
	vector<ArchiveEntry*> cfg_entries = theArchiveManager->findAllResourceEntries(opt);
	for (unsigned a = 0; a < cfg_entries.size(); a++)
	{
		// Log message
		Archive* parent = cfg_entries[a]->getParent();
		if (parent)
			LOG_MESSAGE(1, "Reading SLADECFG in %s", parent->getFilename());

		// Read embedded config
		string config = wxString::FromAscii(cfg_entries[a]->getData(), cfg_entries[a]->getSize());
		if (!readConfiguration(config, cfg_entries[a]->getName(), format, true, false))
			LOG_MESSAGE(1, "Error reading embedded game configuration, not loaded");
	}

	return ok;
}

// ----------------------------------------------------------------------------
// Configuration::actionSpecial
//
// Returns the action special definition for [id]
// ----------------------------------------------------------------------------
const ActionSpecial& Configuration::actionSpecial(unsigned id)
{
	auto& as = action_specials_[id];

	// Defined Action Special
	if (as.defined())
		return as;

	// Boom Generalised Special
	if (supported_features_[Feature::Boom] && id >= 0x2f80)
	{
		if ((id & 7) >= 6)
			return ActionSpecial::generalManual();
		else
			return ActionSpecial::generalSwitched();
	}

	// Unknown
	return ActionSpecial::unknown();
}

// ----------------------------------------------------------------------------
// Configuration::actionSpecialName
//
// Returns the action special name for [special], if any
// ----------------------------------------------------------------------------
string Configuration::actionSpecialName(int special)
{
	// Check special id is valid
	if (special < 0)
		return "Unknown";
	else if (special == 0)
		return "None";

	if (action_specials_[special].defined())
		return action_specials_[special].name();
	else if (special >= 0x2F80 && supported_features_[Feature::Boom])
		return BoomGenLineSpecial::parseLineType(special);
	else
		return "Unknown";
}

// ----------------------------------------------------------------------------
// Configuration::thingType
//
// Returns the thing type definition for [type]
// ----------------------------------------------------------------------------
const ThingType& Configuration::thingType(unsigned type)
{
	auto& ttype = thing_types_[type];
	if (ttype.defined())
		return ttype;
	else
		return ThingType::unknown();
}

// ----------------------------------------------------------------------------
// Configuration::thingTypeGroupDefaults
//
// Returns the default ThingType properties for [group]
// ----------------------------------------------------------------------------
const ThingType& Configuration::thingTypeGroupDefaults(const string& group)
{
	return tt_group_defaults_[group];
}

// ----------------------------------------------------------------------------
// Configuration::thingFlag
//
// Returns the name of the thing flag at [index]
// ----------------------------------------------------------------------------
string Configuration::thingFlag(unsigned index)
{
	// Check index
	if (index >= flags_thing_.size())
		return "";

	return flags_thing_[index].name;
}

// ----------------------------------------------------------------------------
// Configuration::thingFlagSet
//
// Returns true if the flag at [index] is set for [thing]
// ----------------------------------------------------------------------------
bool Configuration::thingFlagSet(unsigned index, MapThing* thing)
{
	// Check index
	if (index >= flags_thing_.size())
		return false;

	// Check if flag is set
	unsigned long flags = thing->intProperty("flags");
	if (flags & flags_thing_[index].flag)
		return true;
	else
		return false;
}

// ----------------------------------------------------------------------------
// Configuration::thingFlagSet
//
// Returns true if the flag matching [flag] is set for [thing]
// ----------------------------------------------------------------------------
bool Configuration::thingFlagSet(string flag, MapThing* thing, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return thing->boolProperty(flag);

	// Get current flags
	unsigned long flags = thing->intProperty("flags");

	// Iterate through flags
	for (size_t i = 0; i < flags_thing_.size(); ++i)
	{
		if (flags_thing_[i].udmf == flag)
			return !!(flags & flags_thing_[i].flag);
	}
	LOG_MESSAGE(2, "Flag %s does not exist in this configuration", flag);
	return false;
}

// ----------------------------------------------------------------------------
// Configuration::thingBasicFlagSet
//
// Returns true if the basic flag matching [flag] is set for [thing]
// ----------------------------------------------------------------------------
bool Configuration::thingBasicFlagSet(string flag, MapThing* thing, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return thing->boolProperty(flag);

	// Get current flags
	unsigned long flags = thing->intProperty("flags");

	// Hexen-style flags in Hexen-format maps
	bool hexen = map_format == MAP_HEXEN;

	// Easy Skill
	if (flag == "skill2" || flag == "skill1")
		return !!(flags & 1);

	// Medium Skill
	else if (flag == "skill3")
		return !!(flags & 2);

	// Hard Skill
	else if (flag == "skill4" || flag == "skill5")
		return !!(flags & 4);

	// Game mode flags
	else if (flag == "single")
	{
		// Single Player
		if (hexen)
			return !!(flags & 256);
		// *Not* Multiplayer
		else
			return !(flags & 16);
	}
	else if (flag == "coop")
	{
		// Coop
		if (hexen)
			return !!(flags & 512);
		// *Not* Not In Coop
		else if (supported_features_[Feature::Boom])
			return !(flags & 64);
		else
			return true;
	}
	else if (flag == "dm")
	{
		// Deathmatch
		if (hexen)
			return !!(flags & 1024);
		// *Not* Not In DM
		else if (supported_features_[Feature::Boom])
			return !(flags & 32);
		else
			return true;
	}

	// Hexen class flags
	else if (hexen && flag.StartsWith("class"))
	{
		// Fighter
		if (flag == "class1")
			return !!(flags & 32);
		// Cleric
		else if (flag == "class2")
			return !!(flags & 64);
		// Mage
		else if (flag == "class3")
			return !!(flags & 128);
	}

	// Not basic
	return thingFlagSet(flag, thing, map_format);
}

// ----------------------------------------------------------------------------
// Configuration::thingFlagsString
//
// Returns a string of all thing flags set in [flags]
// ----------------------------------------------------------------------------
string Configuration::thingFlagsString(int flags)
{
	// Check against all flags
	string ret = "";
	for (unsigned a = 0; a < flags_thing_.size(); a++)
	{
		if (flags & flags_thing_[a].flag)
		{
			// Add flag name to string
			ret += flags_thing_[a].name;
			ret += ", ";
		}
	}

	// Remove ending ', ' if needed
	if (ret.Length() > 0)
		ret.RemoveLast(2);
	else
		return "None";

	return ret;
}

// ----------------------------------------------------------------------------
// Configuration::setThingFlag
//
// Sets thing flag at [index] for [thing]. If [set] is false, the flag is
// unset
// ----------------------------------------------------------------------------
void Configuration::setThingFlag(unsigned index, MapThing* thing, bool set)
{
	// Check index
	if (index >= flags_thing_.size())
		return;

	// Determine new flags value
	unsigned long flags = thing->intProperty("flags");
	if (set)
		flags |= flags_thing_[index].flag;
	else
		flags = (flags & ~flags_thing_[index].flag);

	// Update thing flags
	thing->setIntProperty("flags", flags);
}

// ----------------------------------------------------------------------------
// Configuration::setThingFlag
//
// Sets thing flag matching [flag] (UDMF name) for [thing]. If [set] is false,
// the flag is unset
// ----------------------------------------------------------------------------
void Configuration::setThingFlag(string flag, MapThing* thing, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
	{
		thing->setBoolProperty(flag, set);
		return;
	}

	// Iterate through flags
	unsigned long flag_val = 0;
	for (size_t i = 0; i < flags_thing_.size(); ++i)
	{
		if (flags_thing_[i].udmf == flag)
		{
			flag_val = flags_thing_[i].flag;
			break;
		}
	}

	if (flag_val == 0)
	{
		LOG_MESSAGE(2, "Flag %s does not exist in this configuration", flag);
		return;
	}

	// Determine new flags value
	unsigned long flags = thing->intProperty("flags");
	if (set)
		flags |= flag_val;
	else
		flags = (flags & ~flag_val);

	// Update thing flags
	thing->setIntProperty("flags", flags);
}

// ----------------------------------------------------------------------------
// Configuration::setThingBasicFlag
//
// Sets thing basic flag matching [flag] for [thing]. If [set] is false, the
// flag is unset
// ----------------------------------------------------------------------------
void Configuration::setThingBasicFlag(string flag, MapThing* thing, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
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
			set = !set;
		}
	}
	else if (flag == "coop")
	{
		// Coop
		if (hexen)
			flag_val = 512;
		// *Not* Not In Coop
		else if (supported_features_[Feature::Boom])
		{
			flag_val = 64;
			set = !set;
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
		else if (supported_features_[Feature::Boom])
		{
			flag_val = 32;
			set = !set;
		}
		// Multiplayer
		else
			flag_val = 0;
	}

	// Hexen class flags
	else if (flag.StartsWith("class"))
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
		// Determine new flags value
		unsigned long flags = thing->intProperty("flags");
		if (set)
			flags |= flag_val;
		else
			flags = (flags & ~flag_val);

		// Update thing flags
		thing->setIntProperty("flags", flags);
		return;
	}

	// Not basic
	thingFlagSet(flag, thing, map_format);
}

// ----------------------------------------------------------------------------
// Configuration::parseDecorateDefs
//
// Parses all DECORATE thing definitions in [archive]
// ----------------------------------------------------------------------------
bool Configuration::parseDecorateDefs(Archive* archive)
{
	return Game::readDecorateDefs(archive, thing_types_);
	
#if 0
	if (!archive)
		return false;

	// Get base decorate file
	Archive::search_options_t opt;
	opt.match_name = "decorate";
	//opt.match_type = EntryType::getType("text");
	opt.ignore_ext = true;
	vector<ArchiveEntry*> decorate_entries = archive->findAll(opt);
	if (decorate_entries.empty())
		return false;

	LOG_MESSAGE(2, "Parsing DECORATE entries found in archive %s", archive->getFilename());

	//ArchiveEntry* decorate_base = archive->getEntry("DECORATE", true);
	//if (!decorate_base)
	//	return false;

	// Build full definition string
	string full_defs;
	for (unsigned a = 0; a < decorate_entries.size(); a++)
		StringUtils::processIncludes(decorate_entries[a], full_defs, false);
	//StringUtils::processIncludes(decorate_base, full_defs, false);

	// Init tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters(":,{}");
	tz.enableDecorate(true);
	tz.openString(full_defs);

	// --- Parse ---
	string token = tz.getToken();
	while (!token.empty())
	{
		// Check for actor definition
		if (S_CMPNOCASE(token, "actor"))
		{
			// Get actor name
			string name = tz.getToken();

			// Check for inheritance
			string next = tz.peekToken();
			if (next == ":")
			{
				tz.skipToken(); // Skip :
				tz.skipToken(); // Skip parent actor
				next = tz.peekToken();
			}

			// Check for replaces
			if (S_CMPNOCASE(next, "replaces"))
			{
				tz.skipToken(); // Skip replaces
				tz.skipToken(); // Skip replace actor
			}

			// Skip "native" keyword if present
			if (S_CMPNOCASE(tz.peekToken(), "native"))
				tz.skipToken();

			// Check for no editor number (ie can't be placed in the map)
			if (tz.peekToken() == "{")
			{
				LOG_MESSAGE(3, "Not adding actor %s, no editor number", name);

				// Skip actor definition
				tz.skipToken();
				tz.skipSection("{", "}");
			}
			else
			{
				// Read editor number
				int type;
				tz.getInteger(&type);
				string group;
				PropertyList found_props;
				bool title_given = false;
				bool sprite_given = false;
				bool group_given = false;
				bool filters_present = false;
				bool available = false;

				// Skip "native" keyword if present
				if (S_CMPNOCASE(tz.peekToken(), "native"))
					tz.skipToken();

				// Check for actor definition open
				token = tz.getToken();
				if (token == "{")
				{
					token = tz.getToken();
					while (token != "}")
					{
						// Check for subsection
						if (token == "{")
							tz.skipSection("{", "}");

						// Title
						else if (S_CMPNOCASE(token, "//$Title"))
						{
							name = tz.getLine();
							title_given = true;
						}

						// Game filter
						else if (S_CMPNOCASE(token, "game"))
						{
							filters_present = true;
							if (gameDef(currentGame()).supportsFilter(tz.getToken()))
								available = true;
						}

						// Tag
						else if (!title_given && S_CMPNOCASE(token, "tag"))
							name = tz.getToken();

						// Category
						else if (S_CMPNOCASE(token, "//$Group") || S_CMPNOCASE(token, "//$Category"))
						{
							group = tz.getLine();
							group_given = true;
						}

						// Sprite
						else if (S_CMPNOCASE(token, "//$EditorSprite") || S_CMPNOCASE(token, "//$Sprite"))
						{
							found_props["sprite"] = tz.getToken();
							sprite_given = true;
						}

						// Radius
						else if (S_CMPNOCASE(token, "radius"))
							found_props["radius"] = tz.getInteger();

						// Height
						else if (S_CMPNOCASE(token, "height"))
							found_props["height"] = tz.getInteger();

						// Scale
						else if (S_CMPNOCASE(token, "scale"))
							found_props["scalex"] = found_props["scaley"] = tz.getFloat();
						else if (S_CMPNOCASE(token, "xscale"))
							found_props["scalex"] = tz.getFloat();
						else if (S_CMPNOCASE(token, "yscale"))
							found_props["scaley"] = tz.getFloat();

						// Angled
						else if (S_CMPNOCASE(token, "//$Angled"))
							found_props["angled"] = true;
						else if (S_CMPNOCASE(token, "//$NotAngled"))
							found_props["angled"] = false;

						// Monster
						else if (S_CMPNOCASE(token, "monster"))
						{
							found_props["solid"] = true;		// Solid
							found_props["decoration"] = false;	// Not a decoration
						}

						// Hanging
						else if (S_CMPNOCASE(token, "+spawnceiling"))
							found_props["hanging"] = true;

						// Fullbright
						else if (S_CMPNOCASE(token, "+bright"))
							found_props["bright"] = true;

						// Is Decoration
						else if (S_CMPNOCASE(token, "//$IsDecoration"))
							found_props["decoration"] = true;

						// Icon
						else if (S_CMPNOCASE(token, "//$Icon"))
							found_props["icon"] = tz.getToken();

						// DB2 Color
						else if (S_CMPNOCASE(token, "//$Color"))
							found_props["color"] = tz.getToken();

						// SLADE 3 Colour (overrides DB2 color)
						// Good thing US spelling differs from ABC (Aussie/Brit/Canuck) spelling! :p
						else if (S_CMPNOCASE(token, "//$Colour"))
							found_props["colour"] = tz.getLine();

						// Obsolete thing
						else if (S_CMPNOCASE(token, "//$Obsolete"))
							found_props["obsolete"] = true;

						// Translation
						else if (S_CMPNOCASE(token, "translation"))
						{
							string translation = "\"";
							translation += tz.getToken();
							while (tz.peekToken() == ",")
							{
								translation += tz.getToken(); // ,
								translation += tz.getToken(); // next range
							}
							translation += "\"";
							found_props["translation"] = translation;
						}

						// Solid
						else if (S_CMPNOCASE(token, "+solid"))
							found_props["solid"] = true;

						// States
						if (!sprite_given && S_CMPNOCASE(token, "states"))
						{
							tz.skipToken(); // Skip {

							int statecounter = 0;
							string spritestate;
							string laststate;
							int priority = 0;
							int lastpriority = 0;

							token = tz.getToken();
							while (token != "}")
							{
								// Idle, See, Inactive, Spawn, and finally first defined
								if (priority < SS_IDLE)
								{
									string myspritestate = token;
									token = tz.getToken();
									while (token.Cmp(":") && token.Cmp("}"))
									{
										myspritestate = token;
										token = tz.getToken();
									}
									if (S_CMPNOCASE(token, "}"))
										break;
									string sb = tz.getToken(); // Sprite base

									// Handle removed states
									if (S_CMPNOCASE(sb, "Stop"))
										continue;
									// Handle direct gotos, like ZDoom's dead monsters
									if (S_CMPNOCASE(sb, "Goto"))
									{
										tz.skipToken();
										// Skip scope and state
										if (tz.peekToken() == ":")
										{
											tz.skipToken();	// first :
											tz.skipToken(); // second :
											tz.skipToken(); // state name
										}
										continue;
									}
									string sf = tz.getToken(); // Sprite frame(s)
									int mypriority = 0;
									// If the same state is given several names, 
									// don't read the next name as a sprite name!
									// If "::" is encountered, it's a scope operator.
									if ((!sf.Cmp(":")) && tz.peekToken().Cmp(":"))
									{
										if (S_CMPNOCASE(myspritestate, "spawn"))			mypriority = SS_SPAWN;
										else if (S_CMPNOCASE(myspritestate, "inactive"))	mypriority = SS_INACTIVE;
										else if (S_CMPNOCASE(myspritestate, "see"))			mypriority = SS_SEE;
										else if (S_CMPNOCASE(myspritestate, "idle"))		mypriority = SS_IDLE;
										if (mypriority > lastpriority)
										{
											laststate = myspritestate;
											lastpriority = mypriority;
										}
										continue;
									}
									else
									{
										spritestate = myspritestate;
										if (statecounter++ == 0)						mypriority = SS_FIRSTDEFINED;
										if (S_CMPNOCASE(spritestate, "spawn"))			mypriority = SS_SPAWN;
										else if (S_CMPNOCASE(spritestate, "inactive"))	mypriority = SS_INACTIVE;
										else if (S_CMPNOCASE(spritestate, "see"))		mypriority = SS_SEE;
										else if (S_CMPNOCASE(spritestate, "idle"))		mypriority = SS_IDLE;
										if (lastpriority > mypriority)
										{
											spritestate = laststate;
											mypriority = lastpriority;
										}
									}
									if (sb.length() == 4)
									{
										string sprite = sb + sf.Left(1) + "?";
										if (mypriority > priority)
										{
											priority = mypriority;
											found_props["sprite"] = sprite;
											LOG_MESSAGE(3, "Actor %s found sprite %s from state %s", name, sprite, spritestate);
											lastpriority = -1;
										}
									}
								}
								else
								{
									tz.skipSection("{", "}");
									break;
								}
								token = tz.getToken();
							}
						}

						token = tz.getToken();
					}

					LOG_MESSAGE(3, "Parsed actor %s: %d", name, type);
				}
				else
					LOG_MESSAGE(1, "Warning: Invalid actor definition for %s", name);

				// Ignore actors filtered for other games, 
				// and actors with a negative or null type
				if (type > 0 && (available || !filters_present))
				{
					bool defined = false;

					// Create thing type object if needed
					if (!thing_types_[type].type)
					{
						thing_types_[type].type = new ThingType();
						thing_types_[type].index = thing_types_.size();
						thing_types_[type].number = type;
						thing_types_[type].type->decorate_ = true;
					}
					else
						defined = true;
					ThingType* tt = thing_types_[type].type;

					// Get group defaults (if any)
					if (!group.empty())
					{
						ThingType* group_defaults = NULL;
						for (unsigned a = 0; a < tt_group_defaults_.size(); a++)
						{
							if (S_CMPNOCASE(group, tt_group_defaults_[a]->group_))
							{
								group_defaults = tt_group_defaults_[a];
								break;
							}
						}

						if (group_defaults)
						{
							tt->copy(group_defaults);
						}
					}

					// Setup thing
					if (!defined || title_given || tt->decorate_)
						tt->name_ = name;
					if (!defined || group_given || tt->decorate_)
						tt->group_ = group.empty() ? "Decorate" : group;
					if (!defined || sprite_given || tt->sprite_.IsEmpty() || tt->decorate_)
					{
						if (found_props["sprite"].hasValue())
						{
							if (S_CMPNOCASE(found_props["sprite"].getStringValue(), "tnt1a?"))
							{
								if ((!(found_props["icon"].hasValue())) && tt->icon_.IsEmpty())
									tt->icon_ = "tnt1a0";
							}
							else
								tt->sprite_ = found_props["sprite"].getStringValue();
						}
					}
					if (found_props["radius"].hasValue()) tt->radius_ = found_props["radius"].getIntValue();
					if (found_props["height"].hasValue()) tt->height_ = found_props["height"].getIntValue();
					if (found_props["scalex"].hasValue()) tt->scale_.x = found_props["scalex"].getFloatValue();
					if (found_props["scaley"].hasValue()) tt->scale_.y = found_props["scaley"].getFloatValue();
					if (found_props["hanging"].hasValue()) tt->hanging_ = found_props["hanging"].getBoolValue();
					if (found_props["angled"].hasValue()) tt->angled_ = found_props["angled"].getBoolValue();
					if (found_props["bright"].hasValue()) tt->fullbright_ = found_props["bright"].getBoolValue();
					if (found_props["decoration"].hasValue()) tt->decoration_ = found_props["decoration"].getBoolValue();
					if (found_props["icon"].hasValue()) tt->icon_ = found_props["icon"].getStringValue();
					if (found_props["translation"].hasValue()) tt->translation_ = found_props["translation"].getStringValue();
					if (found_props["solid"].hasValue()) tt->solid_ = found_props["solid"].getBoolValue();
					if (found_props["colour"].hasValue())
					{
						wxColour wxc(found_props["colour"].getStringValue());
						if (wxc.IsOk())
						{
							tt->colour_.set(COLWX(wxc));
						}
					}
					else if (found_props["color"].hasValue())
					{
						// Translate DB2 color indices to RGB values
						int color = found_props["color"].getIntValue();
						switch (color)
						{
						case  0:	// DimGray			ARGB value of #FF696969
							tt->colour_.r = 0x69; tt->colour_.g = 0x69; tt->colour_.b = 0x69; break;
						case  1:	// RoyalBlue		ARGB value of #FF4169E1
							tt->colour_.r = 0x41; tt->colour_.g = 0x69; tt->colour_.b = 0xE1; break;
						case  2:	// ForestGreen		ARGB value of #FF228B22
							tt->colour_.r = 0x22; tt->colour_.g = 0x8B; tt->colour_.b = 0x22; break;
						case  3:	// LightSeaGreen	ARGB value of #FF20B2AA
							tt->colour_.r = 0x20; tt->colour_.g = 0xB2; tt->colour_.b = 0xAA; break;
						case  4:	// Firebrick		ARGB value of #FFB22222
							tt->colour_.r = 0xB2; tt->colour_.g = 0x22; tt->colour_.b = 0x22; break;
						case  5:	// DarkViolet		ARGB value of #FF9400D3
							tt->colour_.r = 0x94; tt->colour_.g = 0x00; tt->colour_.b = 0xD3; break;
						case  6:	// DarkGoldenrod	ARGB value of #FFB8860B
							tt->colour_.r = 0xB8; tt->colour_.g = 0x86; tt->colour_.b = 0x0B; break;
						case  7:	// Silver			ARGB value of #FFC0C0C0
							tt->colour_.r = 0xC0; tt->colour_.g = 0xC0; tt->colour_.b = 0xC0; break;
						case  8:	// Gray				ARGB value of #FF808080
							tt->colour_.r = 0x80; tt->colour_.g = 0x80; tt->colour_.b = 0x80; break;
						case  9:	// DeepSkyBlue		ARGB value of #FF00BFFF
							tt->colour_.r = 0x00; tt->colour_.g = 0xBF; tt->colour_.b = 0xFF; break;
						case 10:	// LimeGreen		ARGB value of #FF32CD32
							tt->colour_.r = 0x32; tt->colour_.g = 0xCD; tt->colour_.b = 0x32; break;
						case 11:	// PaleTurquoise	ARGB value of #FFAFEEEE
							tt->colour_.r = 0xAF; tt->colour_.g = 0xEE; tt->colour_.b = 0xEE; break;
						case 12:	// Tomato			ARGB value of #FFFF6347
							tt->colour_.r = 0xFF; tt->colour_.g = 0x63; tt->colour_.b = 0x47; break;
						case 13:	// Violet			ARGB value of #FFEE82EE
							tt->colour_.r = 0xEE; tt->colour_.g = 0x82; tt->colour_.b = 0xEE; break;
						case 14:	// Yellow			ARGB value of #FFFFFF00
							tt->colour_.r = 0xFF; tt->colour_.g = 0xFF; tt->colour_.b = 0x00; break;
						case 15:	// WhiteSmoke		ARGB value of #FFF5F5F5
							tt->colour_.r = 0xF5; tt->colour_.g = 0xF5; tt->colour_.b = 0xF5; break;
						case 16:	// LightPink		ARGB value of #FFFFB6C1
							tt->colour_.r = 0xFF; tt->colour_.g = 0xB6; tt->colour_.b = 0xC1; break;
						case 17:	// DarkOrange		ARGB value of #FFFF8C00
							tt->colour_.r = 0xFF; tt->colour_.g = 0x8C; tt->colour_.b = 0x00; break;
						case 18:	// DarkKhaki		ARGB value of #FFBDB76B
							tt->colour_.r = 0xBD; tt->colour_.g = 0xB7; tt->colour_.b = 0x6B; break;
						case 19:	// Goldenrod		ARGB value of #FFDAA520
							tt->colour_.r = 0xDA; tt->colour_.g = 0xA5; tt->colour_.b = 0x20; break;
						}
					}
					if (found_props["obsolete"].hasValue()) tt->flags_ |= THING_OBSOLETE;
				}
			}
		}
		// Old DECORATE definitions might be found
		else
		{
			string name;
			string sprite;
			string group;
			bool spritefound = false;
			char frame;
			bool framefound = false;
			int type = -1;
			PropertyList found_props;
			if (tz.peekToken() == "{")
				name = token;
			// DamageTypes aren't old DECORATE format, but we handle them here to skip over them
			else if ((S_CMPNOCASE(token, "pickup")) || (S_CMPNOCASE(token, "breakable")) 
				|| (S_CMPNOCASE(token, "projectile")) || (S_CMPNOCASE(token, "damagetype")))
			{
				group = token;
				name = tz.getToken();
			}
			tz.skipToken();	// skip '{'
			do
			{
				token = tz.getToken();
				if (S_CMPNOCASE(token, "DoomEdNum"))
				{
					tz.getInteger(&type);
				}
				else if (S_CMPNOCASE(token, "Sprite"))
				{
					sprite = tz.getToken();
					spritefound = true;
				}
				else if (S_CMPNOCASE(token, "Frames"))
				{
					token = tz.getToken();
					unsigned pos = 0;
					if (token.length() > 0)
					{
						if ((token[0] < 'a' || token[0] > 'z') && (token[0] < 'A' || token[0] > ']'))
						{
							pos = token.find(':') + 1;
							if (token.length() <= pos)
								pos = token.length() + 1;
							else if ((token.length() >= pos + 2) && token[pos + 1] == '*')
								found_props["bright"] = true;
						}
					}
					if (pos < token.length())
					{
						frame = token[pos];
						framefound = true;
					}
				}
				else if (S_CMPNOCASE(token, "Radius"))
					found_props["radius"] = tz.getInteger();
				else if (S_CMPNOCASE(token, "Height"))
					found_props["height"] = tz.getInteger();
				else if (S_CMPNOCASE(token, "Solid"))
					found_props["solid"] = true;
				else if (S_CMPNOCASE(token, "SpawnCeiling"))
					found_props["hanging"] = true;
				else if (S_CMPNOCASE(token, "Scale"))
					found_props["scale"] = tz.getFloat();
				else if (S_CMPNOCASE(token, "Translation1"))
					found_props["translation"] = S_FMT("doom%d", tz.getInteger());
			}
			while (token != "}" && !token.empty());

			// Add only if a DoomEdNum is present
			if (type > 0)
			{
				bool defined = false;
				// Create thing type object if needed
				if (!thing_types_[type].type)
				{
					thing_types_[type].type = new ThingType();
					thing_types_[type].index = thing_types_.size();
					thing_types_[type].number = type;
					thing_types_[type].type->decorate_ = true;
				}
				else
					defined = true;
				ThingType* tt = thing_types_[type].type;

				// Setup thing
				if (!defined)
				{
					tt->name_ = name;
					tt->group_ = "Decorate";
					if (group.length())
						tt->group_ += "/" + group;
					tt->angled_ = false;
					if (spritefound && framefound)
					{
						sprite = sprite + frame + '?';
						if (S_CMPNOCASE(sprite, "tnt1a?"))
							tt->icon_ = "tnt1a0";
						else
							tt->sprite_ = sprite;
					}
				}
				if (found_props["radius"].hasValue()) tt->radius_ = found_props["radius"].getIntValue();
				if (found_props["height"].hasValue()) tt->height_ = found_props["height"].getIntValue();
				if (found_props["scale"].hasValue()) tt->scale_.x = tt->scale_.y = found_props["scale"].getFloatValue();
				if (found_props["hanging"].hasValue()) tt->hanging_ = found_props["hanging"].getBoolValue();
				if (found_props["bright"].hasValue()) tt->fullbright_ = found_props["bright"].getBoolValue();
				if (found_props["translation"].hasValue()) tt->translation_ = found_props["translation"].getStringValue();
				LOG_MESSAGE(3, "Parsed %s %s: %d", group.length() ? group : "decoration", name, type);
			}
			else
				LOG_MESSAGE(3, "Not adding %s %s, no editor number", group.length() ? group : "decoration", name);
		}

		token = tz.getToken();
	}

	//wxFile tempfile(App::path("decorate_full.txt", App::Dir::Executable), wxFile::write);
	//tempfile.Write(full_defs);
	//tempfile.Close();

#endif
	return true;
}

// ----------------------------------------------------------------------------
// Configuration::clearDecorateDefs
//
// Removes any thing definitions parsed from DECORATE entries
// ----------------------------------------------------------------------------
void Configuration::clearDecorateDefs()
{
	/*for (auto def : thing_types)
		if (def.second.type && def.second.type->decorate)
		{
			delete def.second.type;
			def.second.type = nullptr;
		}*/
}

// ----------------------------------------------------------------------------
// Configuration::lineFlag
//
// Returns the name of the line flag at [index]
// ----------------------------------------------------------------------------
string Configuration::lineFlag(unsigned index)
{
	// Check index
	if (index >= flags_line_.size())
		return "";

	return flags_line_[index].name;
}

// ----------------------------------------------------------------------------
// Configuration::lineFlagSet
//
// Returns true if the flag at [index] is set for [line]
// ----------------------------------------------------------------------------
bool Configuration::lineFlagSet(unsigned index, MapLine* line)
{
	// Check index
	if (index >= flags_line_.size())
		return false;

	// Check if flag is set
	unsigned long flags = line->intProperty("flags");
	if (flags & flags_line_[index].flag)
		return true;
	else
		return false;
}

// ----------------------------------------------------------------------------
// Configuration::lineFlagSet
//
// Returns true if the flag matching [flag] (UDMF name) is set for [line]
// ----------------------------------------------------------------------------
bool Configuration::lineFlagSet(string flag, MapLine* line, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return line->boolProperty(flag);

	// Get current flags
	unsigned long flags = line->intProperty("flags");

	// Iterate through flags
	for (size_t i = 0; i < flags_line_.size(); ++i)
	{
		if (flags_line_[i].udmf == flag)
			return !!(flags & flags_line_[i].flag);
	}
	LOG_MESSAGE(2, "Flag %s does not exist in this configuration", flag);
	return false;
}

// ----------------------------------------------------------------------------
// Configuration::lineBasicFlagSet
//
// Returns true if the basic flag matching [flag] (UDMF name) is set for [line]
// 'Basic' flags are flags that are available in some way or another in all
// game configurations
// ----------------------------------------------------------------------------
bool Configuration::lineBasicFlagSet(string flag, MapLine* line, int map_format)
{
	// If UDMF, just get the bool value
	if (map_format == MAP_UDMF)
		return line->boolProperty(flag);

	// Get current flags
	unsigned long flags = line->intProperty("flags");

	// Impassable
	if (flag == "blocking")
		return !!(flags & 1);

	// Two Sided
	else if (flag == "twosided")
		return !!(flags & 4);

	// Upper unpegged
	else if (flag == "dontpegtop")
		return !!(flags & 8);

	// Lower unpegged
	else if (flag == "dontpegbottom")
		return !!(flags & 16);

	// Not basic
	return lineFlagSet(flag, line, map_format);
}

// ----------------------------------------------------------------------------
// Configuration::lineFlagsString
//
// Returns a string containing all flags set on [line]
// ----------------------------------------------------------------------------
string Configuration::lineFlagsString(MapLine* line)
{
	if (!line)
		return "None";

	// Get raw flags
	unsigned long flags = line->intProperty("flags");
	// TODO: UDMF flags

	// Check against all flags
	string ret = "";
	for (unsigned long a = 0; a < flags_line_.size(); a++)
	{
		if (flags & flags_line_[a].flag)
		{
			// Add flag name to string
			ret += flags_line_[a].name;
			ret += ", ";
		}
	}

	// Remove ending ', ' if needed
	if (ret.Length() > 0)
		ret.RemoveLast(2);
	else
		ret = "None";

	return ret;
}

// ----------------------------------------------------------------------------
// Configuration::setLineFlag
//
// Sets line flag at [index] for [line]. If [set] is false, the flag is unset
// ----------------------------------------------------------------------------
void Configuration::setLineFlag(unsigned index, MapLine* line, bool set)
{
	// Check index
	if (index >= flags_line_.size())
		return;

	// Determine new flags value
	unsigned long flags = line->intProperty("flags");
	if (set)
		flags |= flags_line_[index].flag;
	else
		flags = (flags & ~flags_line_[index].flag);

	// Update line flags
	line->setIntProperty("flags", flags);
}

// ----------------------------------------------------------------------------
// Configuration::setLineFlag
//
// Sets line flag matching [flag] (UDMF name) for [line]. If [set] is false,
// the flag is unset
// ----------------------------------------------------------------------------
void Configuration::setLineFlag(string flag, MapLine* line, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
	{
		line->setBoolProperty(flag, set);
		return;
	}

	// Iterate through flags
	unsigned long flag_val = 0;
	for (size_t i = 0; i < flags_line_.size(); ++i)
	{
		if (flags_line_[i].udmf == flag)
		{
			flag_val = flags_line_[i].flag;
			break;
		}
	}

	if (flag_val == 0)
	{
		LOG_MESSAGE(2, "Flag %s does not exist in this configuration", flag);
		return;
	}

	// Determine new flags value
	unsigned long flags = line->intProperty("flags");
	if (set)
		flags |= flag_val;
	else
		flags = (flags & ~flag_val);

	// Update line flags
	line->setIntProperty("flags", flags);
}

// ----------------------------------------------------------------------------
// Configuration::setLineBasicFlag
//
// Sets line basic flag [flag] (UDMF name) for [line]. If [set] is false, the
// flag is unset
// ----------------------------------------------------------------------------
void Configuration::setLineBasicFlag(string flag, MapLine* line, int map_format, bool set)
{
	// If UDMF, just set the bool value
	if (map_format == MAP_UDMF)
	{
		line->setBoolProperty(flag, set);
		return;
	}

	// Get current flags
	unsigned long flags = line->intProperty("flags");
	unsigned long fval = 0;

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
			line->setIntProperty("flags", flags|fval);
		else
			line->setIntProperty("flags", flags & ~fval);
	}
	// Not basic
	else setLineFlag(flag, line, map_format, set);
}

// ----------------------------------------------------------------------------
// Configuration::spacTriggerString
//
// Returns the hexen SPAC trigger for [line] as a string
// ----------------------------------------------------------------------------
string Configuration::spacTriggerString(MapLine* line, int map_format)
{
	if (!line)
		return "None";

	// Hexen format
	if (map_format == MAP_HEXEN)
	{
		// Get raw flags
		int flags = line->intProperty("flags");

		// Get SPAC trigger value from flags
		int trigger = ((flags & 0x1c00) >> 10);

		// Find matching trigger name
		for (unsigned a = 0; a < triggers_line_.size(); a++)
		{
			if (triggers_line_[a].flag == trigger)
				return triggers_line_[a].name;
		}
	}

	// UDMF format
	else if (map_format == MAP_UDMF)
	{
		// Go through all line UDMF properties
		string trigger = "";
		vector<udmfp_t> props = allUDMFProperties(MOBJ_LINE);
		sort(props.begin(), props.end());
		for (unsigned a = 0; a < props.size(); a++)
		{
			// Check for trigger property
			if (props[a].property->isTrigger())
			{
				// Check if the line has this property
				if (line->boolProperty(props[a].property->getProperty()))
				{
					// Add to trigger line
					if (!trigger.IsEmpty())
						trigger += ", ";
					trigger += props[a].property->getName();
				}
			}
		}

		// Check if there was any trigger
		if (trigger.IsEmpty())
			return "None";
		else
			return trigger;
	}

	// Unknown trigger
	return "Unknown";
}

// ----------------------------------------------------------------------------
// Configuration::spacTriggerIndexHexen
//
// Returns the hexen SPAC trigger index for [line]
// ----------------------------------------------------------------------------
int Configuration::spacTriggerIndexHexen(MapLine* line)
{
	// Get raw flags
	int flags = line->intProperty("flags");

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

// ----------------------------------------------------------------------------
// Configuration::allSpacTriggers
//
// Returns a list of all defined SPAC triggers
// ----------------------------------------------------------------------------
wxArrayString Configuration::allSpacTriggers()
{
	wxArrayString ret;

	for (unsigned a = 0; a < triggers_line_.size(); a++)
		ret.Add(triggers_line_[a].name);

	return ret;
}

// ----------------------------------------------------------------------------
// Configuration::setLineSpacTrigger
//
// Sets the SPAC trigger for [line] to the trigger at [index]
// ----------------------------------------------------------------------------
void Configuration::setLineSpacTrigger(unsigned index, MapLine* line)
{
	// Check index
	if (index >= triggers_line_.size())
		return;

	// Get trigger value
	int trigger = triggers_line_[index].flag;

	// Get raw line flags
	int flags = line->intProperty("flags");

	// Apply trigger to flags
	trigger = trigger << 10;
	flags &= ~0x1c00;
	flags |= trigger;

	// Update line flags
	line->setIntProperty("flags", flags);
}

// ----------------------------------------------------------------------------
// Configuration::getUDMFProperty
//
// Returns the UDMF property definition matching [name] for MapObject [type]
// ----------------------------------------------------------------------------
UDMFProperty* Configuration::getUDMFProperty(string name, int type)
{
	if (type == MOBJ_VERTEX)
		return udmf_vertex_props_[name].property;
	else if (type == MOBJ_LINE)
		return udmf_linedef_props_[name].property;
	else if (type == MOBJ_SIDE)
		return udmf_sidedef_props_[name].property;
	else if (type == MOBJ_SECTOR)
		return udmf_sector_props_[name].property;
	else if (type == MOBJ_THING)
		return udmf_thing_props_[name].property;
	else
		return nullptr;
}

// ----------------------------------------------------------------------------
// Configuration::allUDMFProperties
//
// Returns all defined UDMF properties for MapObject type [type]
// ----------------------------------------------------------------------------
vector<udmfp_t> Configuration::allUDMFProperties(int type)
{
	vector<udmfp_t> ret;

	// Build list depending on type
	UDMFPropMap* map = nullptr;
	if (type == MOBJ_VERTEX)
		map = &udmf_vertex_props_;
	else if (type == MOBJ_LINE)
		map = &udmf_linedef_props_;
	else if (type == MOBJ_SIDE)
		map = &udmf_sidedef_props_;
	else if (type == MOBJ_SECTOR)
		map = &udmf_sector_props_;
	else if (type == MOBJ_THING)
		map = &udmf_thing_props_;
	else
		return ret;

	UDMFPropMap::iterator i = map->begin();
	while (i != map->end())
	{
		if (i->second.property)
		{
			udmfp_t up(i->second.property);
			up.index = i->second.index;
			ret.push_back(up);
		}

		i++;
	}

	return ret;
}

// ----------------------------------------------------------------------------
// Configuration::cleanObjectUDMFProperties
//
// Removes any UDMF properties in [object] that have default values
// (so they are not written to the UDMF map unnecessarily)
// ----------------------------------------------------------------------------
void Configuration::cleanObjectUDMFProps(MapObject* object)
{
	// Get UDMF properties list for type
	UDMFPropMap* map = nullptr;
	int type = object->getObjType();
	if (type == MOBJ_VERTEX)
		map = &udmf_vertex_props_;
	else if (type == MOBJ_LINE)
		map = &udmf_linedef_props_;
	else if (type == MOBJ_SIDE)
		map = &udmf_sidedef_props_;
	else if (type == MOBJ_SECTOR)
		map = &udmf_sector_props_;
	else if (type == MOBJ_THING)
		map = &udmf_thing_props_;
	else
		return;

	// Go through properties
	UDMFPropMap::iterator i = map->begin();
	while (i != map->end())
	{
		if (!i->second.property)
		{
			i++;
			continue;
		}

		// Check if the object even has this property
		if (!object->hasProp(i->first))
		{
			i++;
			continue;
		}

		// Remove the property from the object if it is the default value
		//Property& def = i->second.property->getDefaultValue();
		if (i->second.property->getDefaultValue().getType() == PROP_BOOL)
		{
			if (i->second.property->getDefaultValue().getBoolValue() == object->boolProperty(i->first))
				object->props().removeProperty(i->first);
		}
		else if (i->second.property->getDefaultValue().getType() == PROP_INT)
		{
			if (i->second.property->getDefaultValue().getIntValue() == object->intProperty(i->first))
				object->props().removeProperty(i->first);
		}
		else if (i->second.property->getDefaultValue().getType() == PROP_FLOAT)
		{
			if (i->second.property->getDefaultValue().getFloatValue() == object->floatProperty(i->first))
				object->props().removeProperty(i->first);
		}
		else if (i->second.property->getDefaultValue().getType() == PROP_STRING)
		{
			if (i->second.property->getDefaultValue().getStringValue() == object->stringProperty(i->first))
				object->props().removeProperty(i->first);
		}

		i++;
	}
}

// ----------------------------------------------------------------------------
// Configuration::sectorTypeName
//
// Returns the name for sector type value [type], taking generalised types into
// account
// ----------------------------------------------------------------------------
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
		if (damage == 1)
			gen_flags.push_back("5% Damage");
		else if (damage == 2)
			gen_flags.push_back("10% Damage");
		else if (damage == 3)
			gen_flags.push_back("20% Damage");

		// Secret
		if (sectorBoomSecret(type))
			gen_flags.push_back("Secret");

		// Friction
		if (sectorBoomFriction(type))
			gen_flags.push_back("Friction Enabled");

		// Pushers/Pullers
		if (sectorBoomPushPull(type))
			gen_flags.push_back("Pushers/Pullers Enabled");

		// Remove flag bits from type value
		type = type & (boom_sector_flag_start_ - 1);
	}

	// Check if the type only has generalised flags
	if (type == 0 && gen_flags.size() > 0)
	{
		// Just return flags in this case
		string name = gen_flags[0];
		for (unsigned a = 1; a < gen_flags.size(); a++)
			name += S_FMT(" + %s", gen_flags[a]);

		return name;
	}

	// Go through sector types
	string name = "Unknown";
	for (unsigned a = 0; a < sector_types_.size(); a++)
	{
		if (sector_types_[a].type == type)
		{
			name = sector_types_[a].name;
			break;
		}
	}

	// Add generalised flags to type name
	for (unsigned a = 0; a < gen_flags.size(); a++)
		name += S_FMT(" + %s", gen_flags[a]);

	return name;
}

// ----------------------------------------------------------------------------
// Configuration::sectorTypeByName
//
// Returns the sector type value matching [name]
// ----------------------------------------------------------------------------
int Configuration::sectorTypeByName(string name)
{
	for (unsigned a = 0; a < sector_types_.size(); a++)
	{
		if (sector_types_[a].name == name)
			return sector_types_[a].type;
	}

	return 0;
}

// ----------------------------------------------------------------------------
// Configuration::baseSectorType
//
// Returns the 'base' sector type for value [type]
// (strips generalised flags/type)
// ----------------------------------------------------------------------------
int Configuration::baseSectorType(int type)
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

// ----------------------------------------------------------------------------
// Configuration::sectorBoomDamage
//
// Returns the generalised 'damage' flag for [type]: 0=none, 1=5%, 2=10% 3=20%
// ----------------------------------------------------------------------------
int Configuration::sectorBoomDamage(int type)
{
	if (!supportsSectorFlags())
		return 0;

	// No type
	if (type == 0)
		return 0;

	int low_bit = boom_sector_flag_start_ << 0;
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

// ----------------------------------------------------------------------------
// Configuration::sectorBoomSecret
//
// Returns true if the generalised 'secret' flag is set for [type]
// ----------------------------------------------------------------------------
bool Configuration::sectorBoomSecret(int type)
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

// ----------------------------------------------------------------------------
// Configuration::sectorBoomFriction
//
// Returns true if the generalised 'friction' flag is set for [type]
// ----------------------------------------------------------------------------
bool Configuration::sectorBoomFriction(int type)
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

// ----------------------------------------------------------------------------
// Configuration::sectorBoomPushPull
//
// Returns true if the generalised 'pusher/puller' flag is set for [type]
// ----------------------------------------------------------------------------
bool Configuration::sectorBoomPushPull(int type)
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

// ----------------------------------------------------------------------------
// Configuration::boomSectorType
//
// Returns the generalised boom sector type built from parameters
// ----------------------------------------------------------------------------
int Configuration::boomSectorType(int base, int damage, bool secret, bool friction, bool pushpull)
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

	return fulltype;
}

// ----------------------------------------------------------------------------
// Configuration::getDefaultString
//
// Returns the default string value for [property] of MapObject type [type]
// ----------------------------------------------------------------------------
string Configuration::getDefaultString(int type, string property)
{
	switch (type)
	{
	case MOBJ_LINE:
		return defaults_line_[property].getStringValue(); break;
	case MOBJ_SIDE:
		return defaults_side_[property].getStringValue(); break;
	case MOBJ_SECTOR:
		return defaults_sector_[property].getStringValue(); break;
	case MOBJ_THING:
		return defaults_thing_[property].getStringValue(); break;
	default:
		return "";
	}
}

// ----------------------------------------------------------------------------
// Configuration::getDefaultInt
//
// Returns the default int value for [property] of MapObject type [type]
// ----------------------------------------------------------------------------
int Configuration::getDefaultInt(int type, string property)
{
	switch (type)
	{
	case MOBJ_LINE:
		return defaults_line_[property].getIntValue(); break;
	case MOBJ_SIDE:
		return defaults_side_[property].getIntValue(); break;
	case MOBJ_SECTOR:
		return defaults_sector_[property].getIntValue(); break;
	case MOBJ_THING:
		return defaults_thing_[property].getIntValue(); break;
	default:
		return 0;
	}
}

// ----------------------------------------------------------------------------
// Configuration::getDefaultFloat
//
// Returns the default float value for [property] of MapObject type [type]
// ----------------------------------------------------------------------------
double Configuration::getDefaultFloat(int type, string property)
{
	switch (type)
	{
	case MOBJ_LINE:
		return defaults_line_[property].getFloatValue(); break;
	case MOBJ_SIDE:
		return defaults_side_[property].getFloatValue(); break;
	case MOBJ_SECTOR:
		return defaults_sector_[property].getFloatValue(); break;
	case MOBJ_THING:
		return defaults_thing_[property].getFloatValue(); break;
	default:
		return 0;
	}
}

// ----------------------------------------------------------------------------
// Configuration::getDefaultBool
//
// Returns the default boolean value for [property] of MapObject type [type]
// ----------------------------------------------------------------------------
bool Configuration::getDefaultBool(int type, string property)
{
	switch (type)
	{
	case MOBJ_LINE:
		return defaults_line_[property].getBoolValue(); break;
	case MOBJ_SIDE:
		return defaults_side_[property].getBoolValue(); break;
	case MOBJ_SECTOR:
		return defaults_sector_[property].getBoolValue(); break;
	case MOBJ_THING:
		return defaults_thing_[property].getBoolValue(); break;
	default:
		return false;
	}
}

// ----------------------------------------------------------------------------
// Configuration::applyDefaults
//
// Apply defined default values to [object]
// ----------------------------------------------------------------------------
void Configuration::applyDefaults(MapObject* object, bool udmf)
{
	// Get all defaults for the object type
	vector<string> prop_names;
	vector<Property> prop_vals;

	// Line defaults
	if (object->getObjType() == MOBJ_LINE)
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
	else if (object->getObjType() == MOBJ_SIDE)
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
	else if (object->getObjType() == MOBJ_SECTOR)
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
	else if (object->getObjType() == MOBJ_THING)
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
		if (prop_vals[a].getType() == PROP_BOOL)
			object->setBoolProperty(prop_names[a], prop_vals[a].getBoolValue());
		else if (prop_vals[a].getType() == PROP_INT)
			object->setIntProperty(prop_names[a], prop_vals[a].getIntValue());
		else if (prop_vals[a].getType() == PROP_FLOAT)
			object->setFloatProperty(prop_names[a], prop_vals[a].getFloatValue());
		else if (prop_vals[a].getType() == PROP_STRING)
			object->setStringProperty(prop_names[a], prop_vals[a].getStringValue());
		LOG_MESSAGE(3, "Applied default property %s = %s", prop_names[a], prop_vals[a].getStringValue());
	}
}

// ----------------------------------------------------------------------------
// Configuration::setLightLevelInterval
//
// Builds the array of valid light levels from [interval]
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// Configuration::upLightLevel
//
// Returns [light_level] incremented to the next 'valid' light level
// (defined by the game light interval)
// ----------------------------------------------------------------------------
int Configuration::upLightLevel(int light_level)
{
	// No defined levels
	if (light_levels_.size() == 0)
		return light_level;

	for (int a = 0; a < (int)light_levels_.size() - 1; a++)
	{
		if (light_level >= light_levels_[a] && light_level < light_levels_[a+1])
			return light_levels_[a+1];
	}

	return light_levels_.back();
}

// ----------------------------------------------------------------------------
// Configuration::downLightLevel
//
// Returns [light_level] decremented to the next 'valid' light level
// (defined by the game light interval)
// ----------------------------------------------------------------------------
int Configuration::downLightLevel(int light_level)
{
	// No defined levels
	if (light_levels_.size() == 0)
		return light_level;

	for (int a = 0; a < (int)light_levels_.size() - 1; a++)
	{
		if (light_level > light_levels_[a] && light_level <= light_levels_[a+1])
			return light_levels_[a];
	}

	return 0;
}

// ----------------------------------------------------------------------------
// Configuration::dumpActionSpecials
//
// Dumps all defined action specials to the log
// ----------------------------------------------------------------------------
void Configuration::dumpActionSpecials()
{
	for (auto& i : action_specials_)
		if (i.second.defined())
			LOG_MESSAGE(1, "Action special %d = %s", i.first, i.second.stringDesc());
}

// ----------------------------------------------------------------------------
// Configuration::dumpThingTypes
//
// Dumps all defined thing types to the log
// ----------------------------------------------------------------------------
void Configuration::dumpThingTypes()
{
	for (auto& i : thing_types_)
		if (i.second.defined())
			LOG_MESSAGE(1, "Thing type %d = %s", i.first, i.second.stringDesc());
}

// ----------------------------------------------------------------------------
// Configuration::dumpValidMapNames
//
// Dumps all defined map names to the log
// ----------------------------------------------------------------------------
void Configuration::dumpValidMapNames()
{
	LOG_MESSAGE(1, "Valid Map Names:");
	for (unsigned a = 0; a < maps_.size(); a++)
		Log::info(maps_[a].mapname);
}

// ----------------------------------------------------------------------------
// Configuration::dumpUDMFProperties
//
// Dumps all defined UDMF properties to the log
// ----------------------------------------------------------------------------
void Configuration::dumpUDMFProperties()
{
	// Vertex
	LOG_MESSAGE(1, "\nVertex properties:");
	UDMFPropMap::iterator i = udmf_vertex_props_.begin();
	while (i != udmf_vertex_props_.end())
	{
		Log::info(i->second.property->getStringRep());
		i++;
	}

	// Line
	LOG_MESSAGE(1, "\nLine properties:");
	i = udmf_linedef_props_.begin();
	while (i != udmf_linedef_props_.end())
	{
		Log::info(i->second.property->getStringRep());
		i++;
	}

	// Side
	LOG_MESSAGE(1, "\nSide properties:");
	i = udmf_sidedef_props_.begin();
	while (i != udmf_sidedef_props_.end())
	{
		Log::info(i->second.property->getStringRep());
		i++;
	}

	// Sector
	LOG_MESSAGE(1, "\nSector properties:");
	i = udmf_sector_props_.begin();
	while (i != udmf_sector_props_.end())
	{
		Log::info(i->second.property->getStringRep());
		i++;
	}

	// Thing
	LOG_MESSAGE(1, "\nThing properties:");
	i = udmf_thing_props_.begin();
	while (i != udmf_thing_props_.end())
	{
		Log::info(i->second.property->getStringRep());
		i++;
	}
}


// ----------------------------------------------------------------------------
//
// Console Commands
//
// ----------------------------------------------------------------------------

CONSOLE_COMMAND(testgc, 0, false)
{
	string game = "doomu";

	if (args.size() > 0)
		game = args[0];

	Game::configuration().openConfig(game);
}

CONSOLE_COMMAND(dumpactionspecials, 0, false)
{
	Game::configuration().dumpActionSpecials();
}

CONSOLE_COMMAND(dumpudmfprops, 0, false)
{
	Game::configuration().dumpUDMFProperties();
}

CONSOLE_COMMAND(dumpthingtypes, 0, false)
{
	Game::configuration().dumpThingTypes();
}
