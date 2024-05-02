#pragma once

#include "Utility/ColRGBA.h"

namespace slade
{
class Tokenizer;

namespace game
{
	class MapInfo
	{
	public:
		struct Map
		{
			string  name;
			bool    lookup_name = false;
			string  entry_name;
			int     level_num         = 0;
			string  sky1              = "SKY1";
			float   sky1_scroll_speed = 0.f;
			string  sky2;
			float   sky2_scroll_speed    = 0.f;
			bool    sky_double           = false;
			bool    sky_force_no_stretch = false;
			bool    sky_stretch          = false;
			ColRGBA fade                 = ColRGBA::BLACK;
			ColRGBA fade_outside         = ColRGBA::BLACK; // OutsideFog
			string  music;
			bool    lighting_smooth      = false;
			int     lighting_wallshade_v = 0;
			int     lighting_wallshade_h = 0;
			bool    force_fake_contrast  = false;

			// GZDoom
			int fog_density         = 0;
			int fog_density_outside = 0;
			int fog_density_sky     = 0;
		};

		struct DoomEdNum
		{
			string actor_class;
			string special;
			int    args[5];
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
		Map&               getMap(string_view name);
		bool               addOrUpdateMap(const Map& map);

		// DoomEdNum access
		const DoomEdNumMap& doomEdNums() const { return editor_nums_; }
		DoomEdNum&          doomEdNum(int number) { return editor_nums_[number]; }
		int                 doomEdNumForClass(string_view actor_class) const;

		// MAPINFO loading
		bool readMapInfo(const Archive& archive);

		// General parsing helpers
		bool checkEqualsToken(Tokenizer& tz, string_view parsing) const;
		bool strToCol(const string& str, ColRGBA& col) const;

		// ZDoom MAPINFO parsing
		bool parseZMapInfo(const ArchiveEntry* entry);
		bool parseZMap(Tokenizer& tz, string_view type);
		bool parseDoomEdNums(Tokenizer& tz);

		// General
		Format detectMapInfoType(const ArchiveEntry* entry) const;

		// Debug info
		void dumpDoomEdNums() const;

	private:
		vector<Map>  maps_;
		Map          default_map_;
		DoomEdNumMap editor_nums_;
	};
} // namespace game
} // namespace slade
