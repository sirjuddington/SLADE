#pragma once

#include "Args.h"

class PropertyList;
class ParseTreeNode;

namespace Game
{
enum class TagType;

class ThingType
{
public:
	enum Flags
	{
		Pathed    = 1 << 0, // Things that work in paths (ZDoom's interpolation points and patrol points)
		Dragon    = 1 << 1, // Dragon makes its own paths, without using special things
		Script    = 1 << 2, // Special is actually a script number (like Hexen's Heresiarch)
		CoOpStart = 1 << 3, // Thing is a numbered player start
		DMStart   = 1 << 4, // Thing is a free-for-all player start
		TeamStart = 1 << 5, // Thing is a team-game player start
		Obsolete  = 1 << 6, // Thing is flagged as obsolete
	};

	ThingType(const string& name = "Unknown", const string& group = "", const string& class_name = "");
	~ThingType() = default;

	void copy(const ThingType& copy);

	const string&  name() const { return name_; }
	const string&  group() const { return group_; }
	ColRGBA        colour() const { return colour_; }
	int            radius() const { return radius_; }
	int            height() const { return height_; }
	float          scaleX() const { return scale_.x; }
	float          scaleY() const { return scale_.y; }
	bool           angled() const { return angled_; }
	bool           hanging() const { return hanging_; }
	bool           fullbright() const { return fullbright_; }
	bool           shrinkOnZoom() const { return shrink_; }
	bool           decoration() const { return decoration_; }
	bool           solid() const { return solid_; }
	int            zethIcon() const { return zeth_icon_; }
	int            flags() const { return flags_; }
	int            nextType() const { return next_type_; }
	int            nextArgs() const { return next_args_; }
	TagType        needsTag() const { return tagged_; }
	const string&  sprite() const { return sprite_; }
	const string&  icon() const { return icon_; }
	const string&  translation() const { return translation_; }
	const string&  palette() const { return palette_; }
	const ArgSpec& argSpec() const { return args_; }
	int            number() const { return number_; }
	bool           decorate() const { return decorate_; }
	const string&  className() const { return class_name_; }

	void setSprite(const string& sprite) { sprite_ = sprite; }

	bool defined() const { return number_ >= 0; }
	void define(int number, const string& name, const string& group);

	void   reset();
	void   parse(ParseTreeNode* node);
	string stringDesc() const;
	void   loadProps(PropertyList& props, bool decorate = true, bool zscript = false);

	static const ThingType& unknown() { return unknown_; }
	static void             initGlobal();

private:
	string  name_;
	string  group_;
	ColRGBA colour_     = { 170, 170, 180, 255, 0 };
	int     radius_     = 20;
	int     height_     = -1;
	Vec2f   scale_      = { 1., 1. };
	bool    angled_     = true;
	bool    hanging_    = false;
	bool    shrink_     = false;
	bool    fullbright_ = false;
	bool    decoration_ = false;
	int     zeth_icon_  = -1;
	string  sprite_;
	string  icon_;
	string  translation_;
	string  palette_;
	ArgSpec args_;
	bool    decorate_  = false;
	bool    solid_     = false;
	int     next_type_ = 0;
	int     next_args_ = 0;
	int     flags_     = 0;
	TagType tagged_;
	int     number_ = -1;
	string  class_name_;

	static ThingType unknown_;
};
} // namespace Game
