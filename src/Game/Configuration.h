#pragma once

#include "Game.h"
#include "ActionSpecial.h"
#include "ThingType.h"
#include "UDMFProperty.h"
#include "Utility/PropertyList/PropertyList.h"

class ParseTreeNode;
class ArchiveEntry;
class Archive;
class MapLine;
class MapThing;
class MapObject;

namespace Game
{
	// Feature Support
	enum class Feature
	{
		Boom,
		AnyMapName,
		MixTexFlats,
		TxTextures,
		LongNames,
	};
	enum class UDMFFeature
	{
		Slopes,				// Slope support
		FlatLighting,		// Flat lighting independent from sector lighting
		FlatPanning,		// Flat panning
		FlatRotation,		// Flat rotation
		FlatScaling,		// Flat scaling
		LineTransparency,	// Line transparency
		SectorColor,		// Sector colour
		SectorFog,			// Sector fog
		SideLighting,		// Sidedef lighting independent from sector lighting
		SideMidtexWrapping,	// Per-sidedef midtex wrapping
		SideScaling,		// Line scaling
		TextureScaling,		// Per-texture line scaling
		TextureOffsets,		// Per-texture offsets compared to per-sidedef
		ThingScaling,		// Per-thing scaling
		ThingRotation,		// Per-thing pitch and yaw rotation
	};

	struct udmfp_t
	{
		UDMFProperty*	property;
		int				index;
		udmfp_t(UDMFProperty* property = nullptr) { this->property = property; index = 0; }

		bool operator< (const udmfp_t& right) const { return (index < right.index); }
		bool operator> (const udmfp_t& right) const { return (index > right.index); }
	};

	struct gc_mapinfo_t
	{
		string	mapname;
		string	sky1;
		string	sky2;
	};

	struct sectype_t
	{
		int		type;
		string	name;
		sectype_t() { type = -1; name = "Unknown"; }
		sectype_t(int type, string name) { this->type = type; this->name = name; }
	};

	WX_DECLARE_STRING_HASH_MAP(udmfp_t, UDMFPropMap);

	class Configuration
	{
	public:
		Configuration();
		~Configuration();

		void	setDefaults();
		string	currentGame() const { return current_game_; }
		string	currentPort() const { return current_port_; }
		bool	supportsSectorFlags() const { return boom_sector_flag_start_ > 0; }
		string	udmfNamespace();
		string	skyFlat() const { return sky_flat_; }
		string	scriptLanguage() const { return script_language_; }
		int		lightLevelInterval();

		string			readConfigName(MemChunk& mc);
		unsigned		nMapNames() const { return maps_.size(); }
		string			mapName(unsigned index);
		gc_mapinfo_t	mapInfo(string mapname);

		// Feature Support
		bool	featureSupported(Feature feature) { return supported_features_[feature]; }
		bool	featureSupported(UDMFFeature feature) { return udmf_features_[feature]; }

		// Configuration reading
		void	readActionSpecials(
					ParseTreeNode* node,
					Arg::SpecialMap& shared_args,
					ActionSpecial* group_defaults = nullptr
				);
		void	readThingTypes(ParseTreeNode* node, const ThingType& group_defaults = ThingType::unknown());
		void	readUDMFProperties(ParseTreeNode* node, UDMFPropMap& plist);
		void	readGameSection(ParseTreeNode* node_game, bool port_section = false);
		bool	readConfiguration(
					string& cfg,
					string source = "",
					uint8_t format = MAP_UNKNOWN,
					bool ignore_game = false,
					bool clear = true
				);
		bool	openConfig(string game, string port = "", uint8_t format = MAP_UNKNOWN);

		// Action specials
		const ActionSpecial&				actionSpecial(unsigned id);
		string								actionSpecialName(int special);
		const std::map<int, ActionSpecial>&	allActionSpecials() const { return action_specials_; }

		// Thing types
		const ThingType&				thingType(unsigned type);
		const std::map<int, ThingType>&	allThingTypes() const { return thing_types_; }
		const ThingType&				thingTypeGroupDefaults(const string& group);

		// Thing flags
		int		nThingFlags() const { return flags_thing_.size(); }
		string	thingFlag(unsigned flag_index);
		bool	thingFlagSet(unsigned flag_index, MapThing* thing);
		bool	thingFlagSet(string udmf_name, MapThing* thing, int map_format);
		bool	thingBasicFlagSet(string flag, MapThing* line, int map_format);
		string	thingFlagsString(int flags);
		void	setThingFlag(unsigned flag_index, MapThing* thing, bool set = true);
		void	setThingFlag(string udmf_name, MapThing* thing, int map_format, bool set = true);
		void	setThingBasicFlag(string flag, MapThing* line, int map_format, bool set = true);

		// DECORATE
		bool	parseDecorateDefs(Archive* archive);
		void	clearDecorateDefs();

		// Line flags
		int		nLineFlags() const { return flags_line_.size(); }
		string	lineFlag(unsigned flag_index);
		bool	lineFlagSet(unsigned flag_index, MapLine* line);
		bool	lineFlagSet(string udmf_name, MapLine* line, int map_format);
		bool	lineBasicFlagSet(string flag, MapLine* line, int map_format);
		string	lineFlagsString(MapLine* line);
		void	setLineFlag(unsigned flag_index, MapLine* line, bool set = true);
		void	setLineFlag(string udmf_name, MapLine* line, int map_format, bool set = true);
		void	setLineBasicFlag(string flag, MapLine* line, int map_format, bool set = true);

		// Line action (SPAC) triggers
		string			spacTriggerString(MapLine* line, int map_format);
		int				spacTriggerIndexHexen(MapLine* line);
		wxArrayString	allSpacTriggers();
		void			setLineSpacTrigger(unsigned trigger_index, MapLine* line);
		static int		parseTagged(ParseTreeNode* tagged);

		// UDMF properties
		UDMFProperty*	getUDMFProperty(string name, int type);
		vector<udmfp_t>	allUDMFProperties(int type);
		void			cleanObjectUDMFProps(MapObject* object);

		// Sector types
		string				sectorTypeName(int type);
		vector<sectype_t>	allSectorTypes() const { return sector_types_; }
		int					sectorTypeByName(string name);
		int					baseSectorType(int type);
		int					sectorBoomDamage(int type);
		bool				sectorBoomSecret(int type);
		bool				sectorBoomFriction(int type);
		bool				sectorBoomPushPull(int type);
		int					boomSectorType(int base, int damage, bool secret, bool friction, bool pushpull);

		// Defaults
		string	getDefaultString(int type, string property);
		int		getDefaultInt(int type, string property);
		double	getDefaultFloat(int type, string property);
		bool	getDefaultBool(int type, string property);
		void	applyDefaults(MapObject* object, bool udmf = false);

		// Misc
		void	setLightLevelInterval(int interval);
		int		upLightLevel(int light_level);
		int		downLightLevel(int light_level);

		// Testing
		void	dumpActionSpecials();
		void	dumpThingTypes();
		void	dumpValidMapNames();
		void	dumpUDMFProperties();

	private:
		string		current_game_;				// Current game name
		string		current_port_;				// Current port name (empty if none)
		bool		map_formats_[4];			// Supported map formats
		string		udmf_namespace_;			// Namespace to use for UDMF
		int 		boom_sector_flag_start_;	// Beginning of Boom sector flags
		string		sky_flat_;					// Sky flat for 3d mode
		string		script_language_;			// Scripting language (should be extended to allow multiple)
		vector<int>	light_levels_;				// Light levels for up/down light in editor
		
		// Action specials
		std::map<int, ActionSpecial>	action_specials_;

		// Thing types
		std::map<int, ThingType>	thing_types_;
		std::map<string, ThingType>	tt_group_defaults_;

		// Flags
		struct flag_t
		{
			int		flag;
			string	name;
			string	udmf;
			flag_t() { flag = 0; name = ""; udmf = ""; }
			flag_t(int flag, string name, string udmf = "") { this->flag = flag; this->name = name; this->udmf = udmf; }
		};
		vector<flag_t>	flags_thing_;
		vector<flag_t>	flags_line_;
		vector<flag_t>	triggers_line_;

		// Sector types
		vector<sectype_t>	sector_types_;

		// Map info
		vector<gc_mapinfo_t>	maps_;

		// UDMF properties
		UDMFPropMap	udmf_vertex_props_;
		UDMFPropMap	udmf_linedef_props_;
		UDMFPropMap	udmf_sidedef_props_;
		UDMFPropMap	udmf_sector_props_;
		UDMFPropMap	udmf_thing_props_;

		// Defaults
		PropertyList	defaults_line_;
		PropertyList	defaults_line_udmf_;
		PropertyList	defaults_side_;
		PropertyList	defaults_side_udmf_;
		PropertyList	defaults_sector_;
		PropertyList	defaults_sector_udmf_;
		PropertyList	defaults_thing_;
		PropertyList	defaults_thing_udmf_;

		// Feature Support
		std::map<Feature, bool>		supported_features_;
		std::map<UDMFFeature, bool>	udmf_features_;
	};
}
