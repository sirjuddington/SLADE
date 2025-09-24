#pragma once

#include "Args.h"
#include "General/JsonFwd.h"
#include "General/MapFormat.h"
#include "Utility/PropertyList.h"

// Forward declarations
namespace slade
{
namespace map
{
	enum class ObjectType : u8;
}
namespace zscript
{
	class Definitions;
}
namespace game
{
	class ActionSpecial;
	class MapInfo;
	class ThingType;
	class UDMFProperty;
	struct SpecialPreset;
} // namespace game
} // namespace slade

namespace slade::game
{
// Feature Support
enum class Feature
{
	Boom,
	AnyMapName,
	MixTexFlats,
	TxTextures,
	LongNames,
	MBF21,
};
enum class UDMFFeature
{
	Slopes,             // Slope support
	FlatLighting,       // Flat lighting independent from sector lighting
	FlatPanning,        // Flat panning
	FlatRotation,       // Flat rotation
	FlatScaling,        // Flat scaling
	LineTransparency,   // Line transparency
	SectorColor,        // Sector colour
	SectorFog,          // Sector fog
	SideLighting,       // Sidedef lighting independent from sector lighting
	SideMidtexWrapping, // Per-sidedef midtex wrapping
	SideScaling,        // Line scaling
	TextureScaling,     // Per-texture line scaling
	TextureOffsets,     // Per-texture offsets compared to per-sidedef
	ThingScaling,       // Per-thing scaling
	ThingRotation,      // Per-thing pitch and yaw rotation
};

typedef std::map<string, UDMFProperty> UDMFPropMap;

class Configuration
{
public:
	struct Flag
	{
		int    flag;
		string name;
		string udmf;
		bool   activation;
	};

	struct MapConf
	{
		string mapname;
		string sky1;
		string sky2;
	};

	struct ConfigDesc
	{
		string game;
		string port;
		string map_format;
	};

	Configuration();
	~Configuration();

	void          setDefaults();
	const string& currentGame() const { return current_game_; }
	const string& currentPort() const { return current_port_; }
	bool          supportsSectorFlags() const { return boom_sector_flag_start_ > 0; }
	string        udmfNamespace() const;
	const string& skyFlat() const { return sky_flat_; }
	const string& scriptLanguage() const { return script_language_; }
	int           lightLevelInterval() const;
	int           playerEyeHeight() const { return player_eye_height_; }

	unsigned      nMapNames() const { return maps_.size(); }
	const string& mapName(unsigned index);
	MapConf       mapInfo(string_view mapname);

	// General Accessors
	const std::map<int, ActionSpecial>& allActionSpecials() const { return action_specials_; }
	const std::map<int, ThingType>&     allThingTypes() const { return thing_types_; }
	const std::map<int, string>&        allSectorTypes() const { return sector_types_; }

	// Feature Support
	bool featureSupported(Feature feature) const { return supported_features_[static_cast<int>(feature)]; }
	bool featureSupported(UDMFFeature feature) const { return udmf_features_[static_cast<int>(feature)]; }

	// Configuration reading
	void readActionSpecials(
		ParseTreeNode*       node,
		Arg::SpecialMap&     shared_args,
		const ActionSpecial* group_defaults = nullptr);
	void readThingTypes(const ParseTreeNode* node, const ThingType* group_defaults = nullptr);
	void readUDMFProperties(const ParseTreeNode* block, UDMFPropMap& plist) const;
	void readGameSection(const ParseTreeNode* node_game, bool port_section = false);
	bool readConfiguration(
		string_view cfg,
		string_view source      = "",
		MapFormat   format      = MapFormat::Unknown,
		bool        ignore_game = false,
		bool        clear       = true);
	bool openConfig(const string& game, const string& port = "", MapFormat format = MapFormat::Unknown);

	// JSON Configuration reading
	bool readGameConfiguration(const Json& j, ConfigDesc desc, ArchiveEntry* entry = nullptr);
	void readConfigurationSection(const Json& j, ConfigDesc cfg, ArchiveEntry* entry = nullptr);
	void readActionSpecials(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry = nullptr);
	void readThingTypes(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry = nullptr);
	void readFlags(const Json& j, vector<Flag>& flags, const ConfigDesc& config, const ArchiveEntry* entry = nullptr);
	void readSectorTypes(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry = nullptr);
	void readUDMFProperties(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry = nullptr);
	void readSpecialPresets(const Json& j, const ConfigDesc& config, const ArchiveEntry* entry = nullptr);

	// Action specials
	const ActionSpecial& actionSpecial(unsigned id);
	string               actionSpecialName(int special);

	// Thing types
	const ThingType& thingType(unsigned type);
	const ThingType& thingTypeGroupDefaults(const string& group);

	// Thing flags
	int    nThingFlags() const { return flags_thing_.size(); }
	string thingFlag(unsigned flag_index);
	bool   thingFlagSet(unsigned flag_index, const MapThing* thing) const;
	bool   thingFlagSet(string_view udmf_name, MapThing* thing, MapFormat map_format) const;
	bool   thingBasicFlagSet(string_view flag, MapThing* thing, MapFormat map_format) const;
	string thingFlagsString(int flags) const;
	void   setThingFlag(unsigned flag_index, MapThing* thing, bool set = true) const;
	void   setThingFlag(string_view udmf_name, MapThing* thing, MapFormat map_format, bool set = true) const;
	void   setThingBasicFlag(string_view flag, MapThing* thing, MapFormat map_format, bool set = true) const;

	// DECORATE
	bool parseDecorateDefs(Archive* archive);
	void clearDecorateDefs() const;

	// ZScript
	void importZScriptDefs(zscript::Definitions& defs);

	// MapInfo
	bool parseMapInfo(const Archive& archive) const;
	void clearMapInfo() const;
	void linkDoomEdNums();

	// Line flags
	unsigned    nLineFlags() const { return flags_line_.size(); }
	const Flag& lineFlag(unsigned flag_index);
	bool        lineFlagSet(unsigned flag_index, const MapLine* line) const;
	bool        lineFlagSet(string_view udmf_name, MapLine* line, MapFormat map_format) const;
	bool        lineBasicFlagSet(string_view flag, MapLine* line, MapFormat map_format) const;
	string      lineFlagsString(const MapLine* line) const;
	void        setLineFlag(unsigned flag_index, MapLine* line, bool set = true) const;
	void        setLineFlag(string_view udmf_name, MapLine* line, MapFormat map_format, bool set = true) const;
	void        setLineBasicFlag(string_view flag, MapLine* line, MapFormat map_format, bool set = true) const;

	// Line action (SPAC) triggers
	string         spacTriggerString(MapLine* line, MapFormat map_format);
	int            spacTriggerIndexHexen(const MapLine* line) const;
	vector<string> allSpacTriggers() const;
	void           setLineSpacTrigger(unsigned trigger_index, MapLine* line) const;
	const string&  spacTriggerUDMFName(unsigned trigger_index);

	// UDMF properties
	UDMFProperty*                                  getUDMFProperty(const string& name, map::ObjectType type);
	UDMFPropMap&                                   allUDMFProperties(map::ObjectType type);
	vector<std::pair<const string, UDMFProperty>*> sortedUDMFProperties(map::ObjectType type);
	void                                           cleanObjectUDMFProps(MapObject* object);

	// Sector types
	string sectorTypeName(int type);
	int    sectorTypeByName(string_view name) const;
	int    baseSectorType(int type) const;
	int    sectorBoomDamage(int type) const;
	bool   sectorBoomSecret(int type) const;
	bool   sectorBoomFriction(int type) const;
	bool   sectorBoomPushPull(int type) const;
	bool   sectorMBF21AltDamageMode(int type) const;
	bool   sectorMBF21KillGroundedMonsters(int type) const;
	int    boomSectorType(
		   int  base,
		   int  damage,
		   bool secret,
		   bool friction,
		   bool pushpull,
		   bool alt_damage,
		   bool kill_grounded) const;

	// Defaults
	string defaultString(map::ObjectType type, const string& property) const;
	int    defaultInt(map::ObjectType type, const string& property) const;
	double defaultFloat(map::ObjectType type, const string& property) const;
	bool   defaultBool(map::ObjectType type, const string& property) const;
	void   applyDefaults(MapObject* object, bool udmf = false) const;

	// Special Presets
	const vector<SpecialPreset>& specialPresets() const { return special_presets_; }

	// Misc
	void setLightLevelInterval(int interval);
	int  upLightLevel(int light_level) const;
	int  downLightLevel(int light_level) const;

	// Testing
	void dumpActionSpecials() const;
	void dumpThingTypes() const;
	void dumpValidMapNames() const;
	void dumpUDMFProperties();

private:
	string                    current_game_;           // Current game name
	string                    current_port_;           // Current port name (empty if none)
	std::map<MapFormat, bool> map_formats_;            // Supported map formats
	string                    udmf_namespace_;         // Namespace to use for UDMF
	int                       boom_sector_flag_start_; // Beginning of Boom sector flags
	string                    sky_flat_;               // Sky flat for 3d mode
	string                    script_language_;        // Scripting language (should be extended to allow multiple)
	vector<int>               light_levels_;           // Light levels for up/down light in editor
	int                       player_eye_height_;      // Player eye height for 3d mode camera

	// Action specials
	std::map<int, ActionSpecial> action_specials_;

	// Thing types
	std::map<int, ThingType>    thing_types_;
	std::map<string, ThingType> tt_group_defaults_;
	vector<ThingType>           parsed_types_;
	// std::map<string, ThingType> parsed_types_;		// ThingTypes parsed from definitions
	// (DECORATE, ZScript etc.)

	// Flags
	vector<Flag> flags_thing_;
	vector<Flag> flags_line_;
	vector<Flag> triggers_line_;

	// Sector types
	std::map<int, string> sector_types_;

	// Map info
	vector<MapConf>     maps_;
	unique_ptr<MapInfo> map_info_;

	// UDMF properties
	UDMFPropMap udmf_vertex_props_;
	UDMFPropMap udmf_linedef_props_;
	UDMFPropMap udmf_sidedef_props_;
	UDMFPropMap udmf_sector_props_;
	UDMFPropMap udmf_thing_props_;

	// Defaults
	PropertyList defaults_line_;
	PropertyList defaults_line_udmf_;
	PropertyList defaults_side_;
	PropertyList defaults_side_udmf_;
	PropertyList defaults_sector_;
	PropertyList defaults_sector_udmf_;
	PropertyList defaults_thing_;
	PropertyList defaults_thing_udmf_;

	// Feature Support
	std::array<bool, 6>  supported_features_;
	std::array<bool, 15> udmf_features_;

	// Special Presets
	vector<SpecialPreset> special_presets_;
};

// Full Game Configuration
Configuration& configuration();

} // namespace slade::game
