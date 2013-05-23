
#ifndef __THING_TYPE_H__
#define __THING_TYPE_H__

#include "Args.h"

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
	bool	angled;
	bool	hanging;
	bool	shrink;
	bool	fullbright;
	bool	decoration;
	string	sprite;
	string	icon;
	string	translation;
	string	palette;
	arg_t	args[5];
	bool	decorate;

public:
	ThingType(string name = "Unknown");
	~ThingType() {}

	void	copy(ThingType* copy);

	string	getName() { return name; }
	string	getGroup() { return group; }
	rgba_t	getColour() { return colour; }
	int		getRadius() { return radius; }
	int		getHeight() { return height; }
	bool	isAngled() { return angled; }
	bool	isHanging() { return hanging; }
	bool	isFullbright() { return fullbright; }
	bool	shrinkOnZoom() { return shrink; }
	bool	isDecoration() { return decoration; }
	string	getSprite() { return sprite; }
	string	getIcon() { return icon; }
	string	getTranslation() { return translation; }
	string	getPalette() { return palette; }
	arg_t&	getArg(int index) { if (index >= 0 && index < 5) return args[index]; else return args[0]; }
	string	getArgsString(int args[5]);
	void	setSprite(string sprite) { this->sprite = sprite; }

	void	reset();
	void	parse(ParseTreeNode* node);
	string	stringDesc();
};

#endif//__THING_TYPE_H__
