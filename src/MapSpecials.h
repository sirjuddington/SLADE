
#ifndef __MAP_SPECIALS_H__
#define __MAP_SPECIALS_H__

class SLADEMap;
class ArchiveEntry;

namespace MapSpecials
{
	struct sector_colour_t
	{
		int		tag;
		rgba_t	colour;
	};

	void	processMapSpecials(SLADEMap* map);
	void	applySectorColours(SLADEMap* map);

	// ZDoom
	void	processZDoomMapSpecials(SLADEMap* map);
	void	processACSScripts(ArchiveEntry* entry);
}

#endif//__MAP_SPECIALS_H__
