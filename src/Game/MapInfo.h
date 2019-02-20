#pragma once

#include "Utility/Colour.h"
#include "Utility/Tokenizer.h"

class Archive;
class ArchiveEntry;

namespace Game
{
class MapInfo
{
public:
	struct Map
	{
		std::string name;
		bool        lookup_name = false;
		std::string entry_name;
		int         level_num         = 0;
		std::string sky1              = "SKY1";
		float       sky1_scroll_speed = 0.f;
		std::string sky2;
		float       sky2_scroll_speed    = 0.f;
		bool        sky_double           = false;
		bool        sky_force_no_stretch = false;
		bool        sky_stretch          = false;
		ColRGBA     fade                 = ColRGBA::BLACK;
		ColRGBA     fade_outside         = ColRGBA::BLACK; // OutsideFog
		std::string music;
		bool        lighting_smooth      = false;
		int         lighting_wallshade_v = 0;
		int         lighting_wallshade_h = 0;
		bool        force_fake_contrast  = false;

		// GZDoom
		int fog_density         = 0;
		int fog_density_outside = 0;
		int fog_density_sky     = 0;
	};

	struct DoomEdNum
	{
		std::string actor_class;
		std::string special;
		int         args[5];
	};
	typedef std::map<int, DoomEdNum> DoomEdNumMap;

	enum class Format
	{
		Hexen,
		ZDoomOld,
		ZDoomNew,
		Eternity,
		Universal
	};

	MapInfo()  = default;
	~MapInfo() = default;

	void clear(bool maps = true, bool editor_nums = true);

	// Maps access
	const vector<Map>& maps() const { return maps_; }
	Map&               getMap(std::string_view name);
	bool               addOrUpdateMap(Map& map);

	// DoomEdNum access
	const DoomEdNumMap& doomEdNums() const { return editor_nums_; }
	DoomEdNum&          doomEdNum(int number) { return editor_nums_[number]; }
	int                 doomEdNumForClass(std::string_view actor_class);

	// MAPINFO loading
	bool readMapInfo(Archive* archive);

	// General parsing helpers
	bool checkEqualsToken(Tokenizer& tz, std::string_view parsing) const;
	bool strToCol(const std::string& str, ColRGBA& col) const;

	// ZDoom MAPINFO parsing
	bool parseZMapInfo(ArchiveEntry* entry);
	bool parseZMap(Tokenizer& tz, std::string_view type);
	bool parseDoomEdNums(Tokenizer& tz);

	// General
	Format detectMapInfoType(ArchiveEntry* entry) const;

	// Debug info
	void dumpDoomEdNums();

private:
	vector<Map>  maps_;
	Map          default_map_;
	DoomEdNumMap editor_nums_;
};
} // namespace Game
