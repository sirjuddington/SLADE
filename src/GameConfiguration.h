
#ifndef __GAME_CONFIGURATION_H__
#define __GAME_CONFIGURATION_H__

#include "ActionSpecial.h"
#include "ThingType.h"
#include "UDMFProperty.h"
#include "PropertyList.h"

struct tt_t {
	ThingType*	type;
	int			number;
	int			index;
	tt_t(ThingType* type = NULL) { this->type = type; index = 0; }

	bool operator< (const tt_t& right) const { return (index < right.index); }
	bool operator> (const tt_t& right) const { return (index > right.index); }
};

struct as_t {
	ActionSpecial*	special;
	int				number;
	int				index;
	as_t(ActionSpecial* special = NULL) { this->special = special; index = 0; }

	bool operator< (const as_t& right) const { return (index < right.index); }
	bool operator> (const as_t& right) const { return (index > right.index); }
};

struct udmfp_t {
	UDMFProperty*	property;
	int				index;
	udmfp_t(UDMFProperty* property = NULL) { this->property = property; index = 0; }

	bool operator< (const udmfp_t& right) const { return (index < right.index); }
	bool operator> (const udmfp_t& right) const { return (index > right.index); }
};

struct gc_mapinfo_t {
	string	mapname;
	string	sky1;
	string	sky2;
};

struct sectype_t {
	int		type;
	string	name;
	sectype_t() { type = -1; name = "Unknown"; }
	sectype_t(int type, string name) { this->type = type; this->name = name; }
};

WX_DECLARE_HASH_MAP(int, as_t, wxIntegerHash, wxIntegerEqual, ASpecialMap);
WX_DECLARE_HASH_MAP(int, tt_t, wxIntegerHash, wxIntegerEqual, ThingTypeMap);
WX_DECLARE_STRING_HASH_MAP(udmfp_t, UDMFPropMap);

class ParseTreeNode;
class ArchiveEntry;
class Archive;
class MapLine;
class MapThing;
class MapObject;
class GameConfiguration {
private:
	//string			name;				// Game/port name
	string				current_game;		// Current game name
	string				current_port;		// Current port name (empty if none)
	bool				map_formats[4];		// Supported map formats
	string				udmf_namespace;		// Namespace to use for UDMF
	bool				boom;				// Boom extensions enabled
	ASpecialMap			action_specials;	// Action specials
	ActionSpecial		as_unknown;			// Default action special
	ActionSpecial		as_generalized;		// Dummy for Boom generalized specials
	ThingTypeMap		thing_types;		// Thing types
	vector<ThingType*>	tt_group_defaults;	// Thing type group defaults
	ThingType			ttype_unknown;		// Default thing type
	bool				any_map_name;		// Allow any map name
	bool				mix_tex_flats;		// Allow mixed textures/flats
	bool				tx_textures;		// Allow TX_ textures
	string				sky_flat;			// Sky flat for 3d mode
	string				script_language;	// Scripting language (should be extended to allow multiple)
	vector<int>			light_levels;		// Light levels for up/down light in editor

	// Basic game configuration info
	struct gconf_t {
		string	name;
		string	title;
		string	filename;
		bool	supported_formats[4];
		bool	user;
		gconf_t() { for (int a = 0; a < 4; a++) supported_formats[a] = false; user = true; }
		bool operator>(const gconf_t& right) const { return title > right.title; }
		bool operator<(const gconf_t& right) const { return title < right.title; }
	};
	gconf_t			gconf_none;
	vector<gconf_t>	game_configs;
	size_t			lastDefaultConfig;

	// Basic port configuration info
	struct pconf_t {
		string			name;
		string			title;
		string			filename;
		bool			supported_formats[4];
		vector<string>	supported_games;
		bool			user;
		pconf_t() { for (int a = 0; a < 4; a++) supported_formats[a] = false; user = true; }
		bool operator>(const pconf_t& right) const { return title > right.title; }
		bool operator<(const pconf_t& right) const { return title < right.title; }
	};
	pconf_t			pconf_none;
	vector<pconf_t>	port_configs;
	size_t			lastDefaultPort;

	// Flags
	struct flag_t {
		int		flag;
		string	name;
		string	udmf;
		flag_t() { flag = 0; name = ""; udmf = ""; }
		flag_t(int flag, string name, string udmf = "") { this->flag = flag; this->name = name; this->udmf = udmf; }
	};
	vector<flag_t>	flags_thing;
	vector<flag_t>	flags_line;
	vector<flag_t>	triggers_line;

	// Sector types
	vector<sectype_t>	sector_types;

	// Map info
	vector<gc_mapinfo_t>	maps;

	// UDMF properties
	UDMFPropMap	udmf_vertex_props;
	UDMFPropMap	udmf_linedef_props;
	UDMFPropMap	udmf_sidedef_props;
	UDMFPropMap	udmf_sector_props;
	UDMFPropMap	udmf_thing_props;

	// Defaults
	PropertyList	defaults_line;
	PropertyList	defaults_side;
	PropertyList	defaults_sector;
	PropertyList	defaults_thing;

	// Singleton instance
	static GameConfiguration*	instance;

public:
	GameConfiguration();
	~GameConfiguration();

	// Singleton implementation
	static GameConfiguration* getInstance() {
		if (!instance)
			instance = new GameConfiguration();

		return instance;
	}

	void	setDefaults();
	string	currentGame() { return current_game; }
	string	currentPort() { return current_port; }
	bool	isBoom() { return boom; }
	bool	anyMapName() { return any_map_name; }
	bool	mixTexFlats() { return mix_tex_flats; }
	bool	txTextures() { return tx_textures; }
	string	udmfNamespace();
	string	skyFlat() { return sky_flat; }
	string	scriptLanguage() { return script_language; }
	int		lightLevelInterval();

	string			readConfigName(MemChunk& mc);
	gconf_t			readBasicGameConfig(MemChunk& mc);
	pconf_t			readBasicPortConfig(MemChunk& mc);
	void			init();
	unsigned		nGameConfigs() { return game_configs.size(); }
	unsigned		nPortConfigs() { return port_configs.size(); }
	gconf_t			gameConfig(unsigned index);
	gconf_t			gameConfig(string id);
	pconf_t			portConfig(unsigned index);
	pconf_t			portConfig(string id);
	bool			portSupportsGame(unsigned port, string game);
	bool			mapFormatSupported(int map_format, int game, int port = -1);
	unsigned		nMapNames() { return maps.size(); }
	string			mapName(unsigned index);
	gc_mapinfo_t	mapInfo(string mapname);

	// Config #include handling
	void	buildConfig(string filename, string& out);
	void	buildConfig(ArchiveEntry* entry, string& out, bool use_res = true);

	// Configuration reading
	void	readActionSpecials(ParseTreeNode* node, ActionSpecial* group_defaults = NULL);
	void	readThingTypes(ParseTreeNode* node, ThingType* group_defaults = NULL);
	void	readUDMFProperties(ParseTreeNode* node, UDMFPropMap& plist);
	void	readGameSection(ParseTreeNode* node_game, bool port_section = false);
	bool	readConfiguration(string& cfg, string source = "", bool ignore_game = false, bool clear = true);
	bool	openConfig(string game, string port = "");
	//bool	openEmbeddedConfig(ArchiveEntry* entry);
	//bool	removeEmbeddedConfig(string name);

	// Action specials
	ActionSpecial*	actionSpecial(unsigned id);
	string			actionSpecialName(int special);
	vector<as_t>	allActionSpecials();

	// Thing types
	ThingType*		thingType(unsigned type);
	vector<tt_t>	allThingTypes();

	// Thing flags
	int		nThingFlags() { return flags_thing.size(); }
	string	thingFlag(unsigned flag_index);
	bool	thingFlagSet(unsigned flag_index, MapThing* thing);
	string	thingFlagsString(int flags);
	void	setThingFlag(unsigned flag_index, MapThing* thing, bool set = true);

	// DECORATE
	bool	parseDecorateDefs(Archive* archive);

	// Line flags
	int		nLineFlags() { return flags_line.size(); }
	string	lineFlag(unsigned flag_index);
	bool	lineFlagSet(unsigned flag_index, MapLine* line);
	bool	lineBasicFlagSet(string flag, MapLine* line, int map_format);
	string	lineFlagsString(MapLine* line);
	void	setLineFlag(unsigned flag_index, MapLine* line, bool set = true);
	void	setLineBasicFlag(string flag, MapLine* line, int map_format, bool set = true);

	// Line action (SPAC) triggers
	string			spacTriggerString(MapLine* line, int map_format);
	wxArrayString	allSpacTriggers();
	void			setLineSpacTrigger(unsigned trigger_index, MapLine* line);

	// UDMF properties
	UDMFProperty*	getUDMFProperty(string name, int type);
	vector<udmfp_t>	allUDMFProperties(int type);
	void			cleanObjectUDMFProps(MapObject* object);

	// Sector types
	string				sectorTypeName(int type, int map_format);
	vector<sectype_t>	allSectorTypes() { return sector_types; }
	int					sectorTypeByName(string name);
	int					baseSectorType(int type, int map_format);
	int					sectorBoomDamage(int type, int map_format);
	bool				sectorBoomSecret(int type, int map_format);
	bool				sectorBoomFriction(int type, int map_format);
	bool				sectorBoomPushPull(int type, int map_format);
	int					boomSectorType(int base, int damage, bool secret, bool friction, bool pushpull, int map_format);

	// Defaults
	string	getDefaultString(int type, string property);
	int		getDefaultInt(int type, string property);
	double	getDefaultFloat(int type, string property);
	bool	getDefaultBool(int type, string property);
	void	applyDefaults(MapObject* object);

	// Misc
	void	setLightLevelInterval(int interval);
	int		upLightLevel(int light_level);
	int		downLightLevel(int light_level);

	// Testing
	void	dumpActionSpecials();
	void	dumpThingTypes();
	void	dumpValidMapNames();
	void	dumpUDMFProperties();
};

// Define for less cumbersome GameConfiguration::getInstance()
#define theGameConfiguration GameConfiguration::getInstance()

#endif//__GAME_CONFIGURATION_H__
