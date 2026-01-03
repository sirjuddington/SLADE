
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Utility/JsonUtils.h"
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

// Cache parsed JSON entries to avoid re-parsing anything imported multiple
// times
struct ParsedJsonEntry
{
	const ArchiveEntry* entry;
	Json                parsed_json;

	ParsedJsonEntry(const ArchiveEntry& entry) : entry(&entry) { parsed_json = jsonutil::parse(entry.data()); }
};
vector<ParsedJsonEntry> parsed_json_entries;
} // namespace slade::game


// -----------------------------------------------------------------------------
//
// Game Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::game
{
inline int featureId(Feature feature)
{
	return static_cast<int>(feature);
}
inline int udmfFeatureId(UDMFFeature feature)
{
	return static_cast<int>(feature);
}

// -----------------------------------------------------------------------------
// Checks if a filter item [j_filter] allows the value [value]
// -----------------------------------------------------------------------------
inline bool checkFilterItem(const Json& j_filter, string_view value)
{
	bool pass = true;

	if (j_filter.is_string())
		pass = strutil::equalCI(j_filter.get<string>(), value);
	else if (j_filter.is_array())
	{
		pass = false;
		for (auto& format : j_filter.get<vector<string>>())
			if (strutil::equalCI(format, value))
			{
				pass = true;
				break;
			}
	}

	return pass;
}

// -----------------------------------------------------------------------------
// Checks if the filter object in [j] allows the game+mapformat combo in [cfg].
// If no filter exists in [j], always pass
// -----------------------------------------------------------------------------
bool checkFilter(const Json& j, const Configuration::ConfigDesc& cfg)
{
	if (j.contains("filter"))
	{
		const auto& j_filter = j.at("filter");

		// Map format filter
		if (j_filter.contains("map_format"))
			if (!checkFilterItem(j_filter.at("map_format"), cfg.map_format))
				return false; // Map format not listed in filter, fail

		// Not map format filter
		if (j_filter.contains("not_map_format"))
			if (checkFilterItem(j_filter.at("not_map_format"), cfg.map_format))
				return false; // Map format *is* listed in filter, fail

		// Game filter
		if (j_filter.contains("game"))
			if (!checkFilterItem(j_filter.at("game"), cfg.game))
				return false; // Game not listed in filter, fail

		// Not game filter
		if (j_filter.contains("not_game"))
			if (checkFilterItem(j_filter.at("not_game"), cfg.game))
				return false; // Game *is* listed in filter, fail
	}

	return true;
}

// -----------------------------------------------------------------------------
// Handles an 'import' JSON object [j], returning a parsed JSON object from the
// import target entry (relative to the given base [entry]).
// If the import objects contains a filter, it will be checked against [cfg] and
// a discarded value will be returned if it doesn't match
// -----------------------------------------------------------------------------
Json getImport(const Json& j, const Configuration::ConfigDesc& cfg, const ArchiveEntry& entry)
{
	string file, key;

	// Check for simple or filtered import
	if (j.is_string())
		file = j.get<string>();
	else if (j.is_object())
	{
		// Check filter
		if (!checkFilter(j, cfg))
			return nlohmann::detail::value_t::discarded;

		jsonutil::getIf(j, "file", file); // Json file to import
		jsonutil::getIf(j, "key", key);   // Key within Json object to import
	}

	// Get target entry to import
	auto import_entry_path = entry.path() + file;
	if (auto import_entry = entry.parent()->entryAtPath(import_entry_path))
	{
		// Check if the entry was previously parsed
		Json* imported = nullptr;
		for (auto& parsed : parsed_json_entries)
			if (parsed.entry == import_entry)
			{
				imported = &parsed.parsed_json;
				break;
			}

		// Parse entry as JSON if it wasn't previously
		if (!imported)
		{
			parsed_json_entries.emplace_back(*import_entry);
			imported = &parsed_json_entries.back().parsed_json;
		}

		// Check parsing was successful
		if (imported->is_discarded())
		{
			log::error("Failed to parse imported entry \"{}\"", import_entry_path);
			return nlohmann::detail::value_t::discarded;
		}

		// If key specified, return the object at said key instead of the base object
		if (!key.empty())
		{
			if (imported->contains(key))
				return imported->at(key);

			log::error("Key {} does not exist in imported JSON entry \"{}\"", key, import_entry_path);
			return nlohmann::detail::value_t::discarded;
		}

		return *imported;
	}

	// Target entry not found
	log::error("Entry \"{}\" not found to import", import_entry_path);
	return nlohmann::detail::value_t::discarded;
}
} // namespace slade::game

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
// Opens the full game configuration [game]+[port], either from the user dir or
// program resource
// -----------------------------------------------------------------------------
bool Configuration::openConfig(const string& game, const string& port, MapFormat format)
{
	auto& game_config = gameDef(game);
	if (game_config.name != game)
		return false;

	// Clear current configuration
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

	// Load base game configuration
	bool game_ok = false;
	if (game_config.user)
	{
		// TODO: Config is in user dir
	}
	else
	{
		// Config is in program resource
		auto epath   = fmt::format("config/games/{}.json", game_config.filename);
		auto archive = app::archiveManager().programResourceArchive();
		auto entry   = archive->entryAtPath(epath);
		if (entry)
		{
			if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
				game_ok = readGameConfiguration(j, ConfigDesc{ game, port, mapFormatId(format) }, entry);
		}
	}

	if (!game_ok)
		return false;

	// Append port configuration (if specified)
	bool port_ok = true;
	if (!port.empty())
	{
		port_ok = false;

		// Check the port supports this game
		auto& conf = portDef(port);
		if (conf.supportsGame(game))
		{
			if (conf.user)
			{
				// TODO: Config is in user dir
			}
			else
			{
				// Config is in program resource
				auto epath   = fmt::format("config/ports/{}.json", conf.filename);
				auto archive = app::archiveManager().programResourceArchive();
				auto entry   = archive->entryAtPath(epath);
				if (entry)
				{
					if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
						port_ok = readGameConfiguration(j, ConfigDesc{ game, port, mapFormatId(format) }, entry);
				}
			}
		}
	}

	if (!port_ok)
		return false;

	// Update current game/port selection
	current_game_      = game;
	current_port_      = port;
	game_configuration = game;
	port_configuration = port;

#if 0
	// TODO: Read any embedded configurations in resource archives
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
#endif

	return true;
}

// -----------------------------------------------------------------------------
// Reads a game configuration from parsed JSON [j], using [cfg] to filter
// sub-sections
// -----------------------------------------------------------------------------
bool Configuration::readGameConfiguration(const Json& j, ConfigDesc cfg, ArchiveEntry* entry)
{
	parsed_json_entries.clear();

	// Map formats
	if (j.contains("map_formats"))
	{
		map_formats_.clear();
		for (const auto& mf : j.at("map_formats").get<vector<string>>())
			map_formats_[mapFormatFromId(mf)] = true;
	}

	// Configuration section(s)
	if (j.contains("configuration"))
	{
		const auto& j_configuration = j.at("configuration");

		// Single configuration section
		if (j_configuration.is_object() && checkFilter(j_configuration, cfg))
			readConfigurationSection(j_configuration, cfg, entry);

		// Multiple configuration sections
		else if (j_configuration.is_array())
			for (const auto& j_cfg_section : j_configuration)
				if (checkFilter(j_cfg_section, cfg))
					readConfigurationSection(j_cfg_section, cfg, entry);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Reads a game configuration section from JSON object [j].
// If the section contains a filter, it will be ignored if the filter does not
// match [cfg]
// -----------------------------------------------------------------------------
void Configuration::readConfigurationSection(const Json& j, ConfigDesc cfg, ArchiveEntry* entry)
{
	// General properties
	jsonutil::getIf(j, "boom_sector_flag_start", boom_sector_flag_start_);
	jsonutil::getIf(j, "udmf_namespace", udmf_namespace_);
	jsonutil::getIf(j, "sky_flat", sky_flat_);
	jsonutil::getIf(j, "script_language", script_language_);
	jsonutil::getIf(j, "player_eye_height", player_eye_height_);

	// Features
	jsonutil::getIf(j, "boom", supported_features_[featureId(Feature::Boom)]);
	jsonutil::getIf(j, "mbf21", supported_features_[featureId(Feature::MBF21)]);
	jsonutil::getIf(j, "map_name_any", supported_features_[featureId(Feature::AnyMapName)]);
	jsonutil::getIf(j, "mix_tex_flats", supported_features_[featureId(Feature::MixTexFlats)]);
	jsonutil::getIf(j, "tx_textures", supported_features_[featureId(Feature::TxTextures)]);
	jsonutil::getIf(j, "long_names", supported_features_[featureId(Feature::LongNames)]);

	// UDMF features
	jsonutil::getIf(j, "udmf_slopes", udmf_features_[udmfFeatureId(UDMFFeature::Slopes)]);
	jsonutil::getIf(j, "udmf_flat_lighting", udmf_features_[udmfFeatureId(UDMFFeature::FlatLighting)]);
	jsonutil::getIf(j, "udmf_flat_panning", udmf_features_[udmfFeatureId(UDMFFeature::FlatPanning)]);
	jsonutil::getIf(j, "udmf_flat_rotation", udmf_features_[udmfFeatureId(UDMFFeature::FlatRotation)]);
	jsonutil::getIf(j, "udmf_flat_scaling", udmf_features_[udmfFeatureId(UDMFFeature::FlatScaling)]);
	jsonutil::getIf(j, "udmf_line_transparency", udmf_features_[udmfFeatureId(UDMFFeature::LineTransparency)]);
	jsonutil::getIf(j, "udmf_sector_color", udmf_features_[udmfFeatureId(UDMFFeature::SectorColor)]);
	jsonutil::getIf(j, "udmf_sector_fog", udmf_features_[udmfFeatureId(UDMFFeature::SectorFog)]);
	jsonutil::getIf(j, "udmf_side_lighting", udmf_features_[udmfFeatureId(UDMFFeature::SideLighting)]);
	jsonutil::getIf(j, "udmf_side_midtex_wrapping", udmf_features_[udmfFeatureId(UDMFFeature::SideMidtexWrapping)]);
	jsonutil::getIf(j, "udmf_side_scaling", udmf_features_[udmfFeatureId(UDMFFeature::SideScaling)]);
	jsonutil::getIf(j, "udmf_texture_scaling", udmf_features_[udmfFeatureId(UDMFFeature::TextureScaling)]);
	jsonutil::getIf(j, "udmf_texture_offsets", udmf_features_[udmfFeatureId(UDMFFeature::TextureOffsets)]);
	jsonutil::getIf(j, "udmf_thing_scaling", udmf_features_[udmfFeatureId(UDMFFeature::ThingScaling)]);
	jsonutil::getIf(j, "udmf_thing_rotation", udmf_features_[udmfFeatureId(UDMFFeature::ThingRotation)]);

	// Light levels interval
	if (j.contains("light_level_interval"))
		setLightLevelInterval(j.at("light_level_interval"));

	// Defaults section
	if (j.contains("defaults"))
	{
		auto& defaults = j.at("defaults");

		// Linedef defaults
		if (defaults.contains("linedef"))
		{
			for (const auto& [prop, value] : defaults.at("linedef").items())
			{
				if (prop == "udmf")
					for (const auto& [u_prop, u_value] : value.items())
						defaults_line_udmf_[u_prop] = jsonutil::toProp(u_value);
				else
					defaults_line_[prop] = jsonutil::toProp(value);
			}
		}

		// Sidedef defaults
		if (defaults.contains("sidedef"))
		{
			for (const auto& [prop, value] : defaults.at("sidedef").items())
			{
				if (prop == "udmf")
					for (const auto& [u_prop, u_value] : value.items())
						defaults_side_udmf_[u_prop] = jsonutil::toProp(u_value);
				else
					defaults_side_[prop] = jsonutil::toProp(value);
			}
		}

		// Sector defaults
		if (defaults.contains("sector"))
		{
			for (const auto& [prop, value] : defaults.at("sector").items())
			{
				if (prop == "udmf")
					for (const auto& [u_prop, u_value] : value.items())
						defaults_sector_udmf_[u_prop] = jsonutil::toProp(u_value);
				else
					defaults_sector_[prop] = jsonutil::toProp(value);
			}
		}

		// Thing defaults
		if (defaults.contains("thing"))
		{
			for (const auto& [prop, value] : defaults.at("thing").items())
			{
				if (prop == "udmf")
					for (const auto& [u_prop, u_value] : value.items())
						defaults_thing_udmf_[u_prop] = jsonutil::toProp(u_value);
				else
					defaults_thing_[prop] = jsonutil::toProp(value);
			}
		}
	}

	// Maps
	if (j.contains("maps"))
	{
		for (const auto& j_map : j.at("maps"))
		{
			MapConf map;
			map.mapname = j_map.at("mapname");
			if (j_map.contains("sky"))
			{
				// Sky texture
				auto& j_sky = j_map.at("sky");
				if (j_sky.is_string())
					map.sky1 = j_sky;
				else if (j_sky.is_array() && j_sky.size() > 1)
				{
					map.sky1 = j_sky.at(0);
					map.sky2 = j_sky.at(1);
				}
				else
					log::warning("Invalid map sky definition for {}", map.mapname);
			}
			maps_.push_back(map);
		}
	}

	// Action specials
	if (j.contains("action_specials"))
		readActionSpecials(j.at("action_specials"), cfg, entry);

	// Special presets
	if (j.contains("special_presets"))
		readSpecialPresets(j.at("special_presets"), cfg, entry);

	// Thing types
	if (j.contains("thing_types"))
		readThingTypes(j.at("thing_types"), cfg, entry);

	// Thing flags
	if (j.contains("thing_flags"))
		readFlags(j.at("thing_flags"), flags_thing_, cfg, entry);

	// Line flags
	if (j.contains("line_flags"))
		readFlags(j.at("line_flags"), flags_line_, cfg, entry);

	// Line triggers
	if (j.contains("line_triggers"))
		readFlags(j.at("line_triggers"), triggers_line_, cfg, entry);

	// Sector types
	if (j.contains("sector_types"))
		readSectorTypes(j.at("sector_types"), cfg, entry);

	// UDMF properties
	if (j.contains("udmf_properties"))
		readUDMFProperties(j.at("udmf_properties"), cfg, entry);
}

// -----------------------------------------------------------------------------
// Reads an action_specials section from JSON object [j]
// -----------------------------------------------------------------------------
void Configuration::readActionSpecials(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry)
{
	// Check if we are clearing existing action specials
	if (j.value("clear_existing", false))
		action_specials_.clear();

	// Handle imports
	if (entry && j.contains("import"))
	{
		for (auto& i : j["import"])
			if (auto j_import = getImport(i, config, *entry); !j_import.is_discarded())
				readActionSpecials(j_import, config, entry);
	}

	// Read any shared args
	Arg::SpecialMap shared_args;
	if (j.contains("arg_presets"))
	{
		for (auto& [name, j_arg] : j.at("arg_presets").items())
			shared_args[name].fromJson(j_arg, nullptr);
	}

	// Check if any groups are present
	if (!j.contains("groups"))
		return;

	// Go through all groups
	for (auto& j_group : j.at("groups"))
	{
		auto group = j_group.value("name", "");
		if (group.empty())
			continue;

		if (!j_group.contains("specials") || !j_group.at("specials").is_array())
		{
			log::warning("Action specials group \"{}\" does not contain specials key", group);
			continue;
		}

		// Setup group defaults
		ActionSpecial defaults;
		if (j_group.contains("defaults"))
			defaults.fromJson(j_group.at("defaults"), shared_args);

		// Go through all specials in group
		for (auto& j_special : j_group.at("specials"))
		{
			if (!j_special.contains("id"))
				continue; // Must have id

			int   special_id     = j_special.at("id");
			auto& action_special = action_specials_[special_id];

			// Apply defaults
			action_special = defaults;
			action_special.setGroup(group);

			// Parse it
			action_special.setNumber(special_id);
			action_special.fromJson(j_special, shared_args);
		}
	}
}

// -----------------------------------------------------------------------------
// Reads a thing_types section from JSON object [j]
// -----------------------------------------------------------------------------
void Configuration::readThingTypes(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry)
{
	// Handle imports
	if (entry && j.contains("import"))
	{
		for (auto& i : j["import"])
			if (auto j_import = getImport(i, config, *entry); !j_import.is_discarded())
				readThingTypes(j_import, config, entry);
	}

	// Check if any groups are present
	if (!j.contains("groups"))
		return;

	// Go through all groups
	for (auto& j_group : j.at("groups"))
	{
		auto group = j_group.value("name", "");
		if (group.empty())
			continue;

		if (!j_group.contains("things") || !j_group.at("things").is_array())
		{
			log::warning("Thing types group \"{}\" does not contain things key", group);
			continue;
		}

		// Setup group defaults
		auto& defaults = tt_group_defaults_[group];
		defaults.define(-1, "", group);
		if (j_group.contains("defaults"))
			defaults.fromJson(j_group.at("defaults"));

		// Go through all things in group
		for (auto& j_thing : j_group.at("things"))
		{
			if (!j_thing.contains("type"))
				continue; // Must have type no.

			int   type       = j_thing.at("type");
			auto& thing_type = thing_types_[type];

			// Reset the thing type (in case it's being redefined for whatever reason)
			thing_type.reset();

			// Apply group defaults
			thing_type.copy(defaults);

			// Parse thing type
			thing_type.define(type, "", group);
			thing_type.fromJson(j_thing);
		}
	}
}

// -----------------------------------------------------------------------------
// Reads flags from JSON object [j] and appends them to [flags]
// -----------------------------------------------------------------------------
void Configuration::readFlags(const Json& j, vector<Flag>& flags, const ConfigDesc& config, const ArchiveEntry* entry)
{
	// Handle imports
	if (entry && j.contains("import"))
	{
		for (auto& i : j["import"])
			if (auto j_import = getImport(i, config, *entry); !j_import.is_discarded())
				readFlags(j_import, flags, config, entry);
	}

	// Go through flags (if any)
	if (j.contains("flags") && j.at("flags").is_array())
	{
		for (const auto& j_flag : j.at("flags"))
		{
			// Parse flag details
			Flag flag;
			flag.flag       = j_flag.value("value", 0);
			flag.name       = j_flag.value("name", "");
			flag.activation = j_flag.value("activation", false);
			if (j_flag.contains("udmf"))
			{
				if (j_flag.at("udmf").is_string())
					flag.udmf = j_flag.at("udmf").get<string>();
				else if (j_flag.at("udmf").is_array())
				{
					for (const auto& udmf_flag : j_flag.at("udmf"))
						flag.udmf += udmf_flag.get<string>() + " ";
					flag.udmf.pop_back();
				}
			}

			// Check if the flag value already exists
			bool exists = false;
			for (auto& f : flags)
				if (f.flag == flag.flag)
				{
					f      = flag;
					exists = true;
					break;
				}

			// Add flag otherwise
			if (!exists)
				flags.push_back(flag);
		}
	}
}

// -----------------------------------------------------------------------------
// Reads a sector_types section from JSON object [j]
// -----------------------------------------------------------------------------
void Configuration::readSectorTypes(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry)
{
	// Handle imports
	if (entry && j.contains("import"))
	{
		for (auto& i : j["import"])
			if (auto j_import = getImport(i, config, *entry); !j_import.is_discarded())
				readSectorTypes(j_import, config, entry);
	}

	// Check if any types are present
	if (!j.contains("types") || !j.at("types").is_array())
		return;

	for (const auto& j_type : j.at("types"))
		if (j_type.contains("type"))
			sector_types_[j_type.at("type").get<int>()] = j_type.value("name", "Unknown");
}

// -----------------------------------------------------------------------------
// Reads a udmf_properties section from JSON object [j]
// -----------------------------------------------------------------------------
void Configuration::readUDMFProperties(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry)
{
	// Handle imports
	if (entry && j.contains("import"))
	{
		for (auto& i : j["import"])
			if (auto j_import = getImport(i, config, *entry); !j_import.is_discarded())
				readUDMFProperties(j_import, config, entry);
	}

	// Check if any blocks are present
	if (!j.contains("blocks") || !j.at("blocks").is_array())
		return;

	// Go through blocks
	for (auto& j_block : j.at("blocks"))
	{
		if (!j_block.contains("block") || !j_block.contains("groups"))
			continue;

		auto         block    = j_block.value("block", "");
		UDMFPropMap* prop_map = nullptr;
		if (block == "vertex")
			prop_map = &udmf_vertex_props_;
		else if (block == "linedef")
			prop_map = &udmf_linedef_props_;
		else if (block == "sidedef")
			prop_map = &udmf_sidedef_props_;
		else if (block == "sector")
			prop_map = &udmf_sector_props_;
		else if (block == "thing")
			prop_map = &udmf_thing_props_;
		else
			continue;

		// Go through groups
		for (auto& j_group : j_block.at("groups"))
		{
			auto group = j_group.value("name", "");
			if (group.empty() || !j_group.contains("properties"))
				continue;

			auto has_default = j_group.contains("defaults");
			for (auto& j_prop : j_group.at("properties"))
			{
				auto udmf = j_prop.value("udmf", "");
				if (udmf.empty())
					continue;

				// Apply defaults if set
				if (has_default)
					(*prop_map)[udmf].fromJson(j_group.at("defaults"), group);

				// Read property
				(*prop_map)[udmf].fromJson(j_prop, group);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Reads a special_presets section from JSON object [j]
// -----------------------------------------------------------------------------
void Configuration::readSpecialPresets(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry)
{
	// Handle imports
	if (entry && j.contains("import"))
	{
		for (auto& i : j["import"])
			if (auto j_import = getImport(i, config, *entry); !j_import.is_discarded())
				readSpecialPresets(j_import, config, entry);
	}

	// Check if any presets are present
	if (!j.contains("presets") || !j.at("presets").is_array())
		return;

	// Go through presets
	for (const auto& j_preset : j.at("presets"))
	{
		special_presets_.emplace_back();
		special_presets_.back().fromJson(j_preset);
	}
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
