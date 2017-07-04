#pragma once

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
			string	name;
			bool	lookup_name;
			string	entry_name;
			int		level_num;
			string	sky1;
			float	sky1_scroll_speed;
			string	sky2;
			float	sky2_scroll_speed;
			bool	sky_double;
			bool	sky_force_no_stretch;
			bool	sky_stretch;
			rgba_t	fade;
			rgba_t	fade_outside;	// OutsideFog
			string	music;
			bool	lighting_smooth;
			int		lighting_wallshade_v;
			int		lighting_wallshade_h;
			bool	force_fake_contrast;

			// GZDoom
			int		fog_density;
			int		fog_density_outside;
			int		fog_density_sky;

			Map() :
				level_num{ 0 },
				sky1{ "SKY1" },
				sky1_scroll_speed{ 0 },
				sky2_scroll_speed{ 0 },
				sky_double{ false },
				sky_force_no_stretch{ false },
				sky_stretch{ false },
				fade{ COL_BLACK },
				fade_outside{ COL_BLACK },
				lighting_smooth{ false },
				lighting_wallshade_v{ 0 },
				lighting_wallshade_h{ 0 },
				force_fake_contrast{ false } {}
		};

		struct DoomEdNum
		{
			string	actor_class;
			string	special;
			int		args[5];
		};
		typedef std::map<int, MapInfo::DoomEdNum> DoomEdNumMap;

		enum class Format
		{
			Hexen,
			ZDoomOld,
			ZDoomNew,
			Eternity,
			Universal
		};

		MapInfo();
		~MapInfo();

		void	clear(bool maps = true, bool editor_nums = true);

		// Maps access
		const vector<Map>&	maps() const { return maps_; }
		Map&				getMap(const string& name);
		bool				addOrUpdateMap(Map& map);

		// DoomEdNum access
		const DoomEdNumMap&	doomEdNums() const { return editor_nums_; }
		DoomEdNum&			doomEdNum(int number) { return editor_nums_[number]; }
		int					doomEdNumForClass(const string& actor_class);

		// MAPINFO loading
		bool	readMapInfo(Archive* archive);

		// General parsing helpers
		bool	checkEqualsToken(Tokenizer& tz, const string& parsing) const;
		bool	strToCol(const string& str, rgba_t& col);

		// ZDoom MAPINFO parsing
		bool	parseZMapInfo(ArchiveEntry* entry);
		bool	parseZMap(Tokenizer& tz, const string& type);
		bool	parseDoomEdNums(Tokenizer& tz);

		// Debug info
		void	dumpDoomEdNums();

	private:
		vector<Map>		maps_;
		Map				default_map_;
		DoomEdNumMap	editor_nums_;
	};
}
