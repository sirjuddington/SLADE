
#ifndef __THING_TYPE_H__
#define __THING_TYPE_H__

#include "Args.h"

enum ThingFlags
{
	THING_PATHED	= 1<<0,	// Things that work in paths (ZDoom's interpolation points and patrol points)
	THING_DRAGON	= 1<<1,	// Dragon makes its own paths, without using special things
	THING_SCRIPT	= 1<<2,	// Special is actually a script number (like Hexen's Heresiarch)
	THING_COOPSTART	= 1<<3, // Thing is a numbered player start
	THING_DMSTART	= 1<<4, // Thing is a free-for-all player start
	THING_TEAMSTART	= 1<<5, // Thing is a team-game player start
	THING_OBSOLETE	= 1<<6,	// Thing is flagged as obsolete
};

class ParseTreeNode;
class ThingType
{
	friend class GameConfiguration;
private:
	string	name;
	string	group;
	rgba_t	colour;
	int		radius;
	int		height;
	float	scaleX;
	float	scaleY;
	bool	angled;
	bool	hanging;
	bool	shrink;
	bool	fullbright;
	bool	decoration;
	int		zeth;
	string	sprite;
	string	icon;
	string	translation;
	string	palette;
	arg_t	args[5];
	int		arg_count;
	bool	decorate;
	bool	solid;
	int		nexttype;
	int		nextargs;
	int		flags;
	int		tagged;

public:
	ThingType(string name = "Unknown");
	~ThingType() {}

	void	copy(ThingType* copy);

	string	getName() { return name; }
	string	getGroup() { return group; }
	rgba_t	getColour() { return colour; }
	int		getRadius() { return radius; }
	int		getHeight() { return height; }
	float	getScaleX()	{ return scaleX; }
	float	getScaleY()	{ return scaleY; }
	bool	isAngled() { return angled; }
	bool	isHanging() { return hanging; }
	bool	isFullbright() { return fullbright; }
	bool	shrinkOnZoom() { return shrink; }
	bool	isDecoration() { return decoration; }
	bool	isSolid() { return solid; }
	int		getZeth() { return zeth; }
	int		getFlags() { return flags; }
	int		getNextType() { return nexttype; }
	int		getNextArgs() { return nextargs; }
	int		needsTag() { return tagged; }
	string	getSprite() { return sprite; }
	string	getIcon() { return icon; }
	string	getTranslation() { return translation; }
	string	getPalette() { return palette; }
	const argspec_t getArgspec() { return argspec_t(args, arg_count); }
	string	getArgsString(int args[5], string argstr[2]);
	void	setSprite(string sprite) { this->sprite = sprite; }

	void	reset();
	void	parse(ParseTreeNode* node);
	string	stringDesc();
};

#endif//__THING_TYPE_H__
