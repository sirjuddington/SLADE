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
			FLAG_PATHED = 1 << 0,		// Things that work in paths (ZDoom's interpolation points and patrol points)
			FLAG_DRAGON = 1 << 1,		// Dragon makes its own paths, without using special things
			FLAG_SCRIPT = 1 << 2,		// Special is actually a script number (like Hexen's Heresiarch)
			FLAG_COOPSTART = 1 << 3,	// Thing is a numbered player start
			FLAG_DMSTART = 1 << 4,		// Thing is a free-for-all player start
			FLAG_TEAMSTART = 1 << 5,	// Thing is a team-game player start
			FLAG_OBSOLETE = 1 << 6,		// Thing is flagged as obsolete
		};

		ThingType(const string& name = "Unknown", const string& group = "");
		~ThingType() {}

		void	copy(const ThingType& copy);

		const string&	name() const { return name_; }
		const string&	group() const { return group_; }
		rgba_t			colour() const { return colour_; }
		int				radius() const { return radius_; }
		int				height() const { return height_; }
		float			scaleX() const { return scale_.x; }
		float			scaleY() const { return scale_.y; }
		bool			angled() const { return angled_; }
		bool			hanging() const { return hanging_; }
		bool			fullbright() const { return fullbright_; }
		bool			shrinkOnZoom() const { return shrink_; }
		bool			decoration() const { return decoration_; }
		bool			solid() const { return solid_; }
		int				zethIcon() const { return zeth_icon_; }
		int				flags() const { return flags_; }
		int				nextType() const { return next_type_; }
		int				nextArgs() const { return next_args_; }
		TagType			needsTag() const { return tagged_; }
		const string&	sprite() const { return sprite_; }
		const string&	icon() const { return icon_; }
		const string&	translation() const { return translation_; }
		const string&	palette() const { return palette_; }
		const ArgSpec&	argSpec() const { return args_; }
		int				number() const { return number_; }
		bool 			decorate() const { return decorate_; }

		void	setSprite(string sprite) { this->sprite_ = sprite; }

		bool	defined() const { return number_ >= 0; }
		void	define(int number, const string& name, const string& group);

		void	reset();
		void	parse(ParseTreeNode* node);
		string	stringDesc() const;
		void	loadProps(PropertyList& props, bool decorate = true);

		static const ThingType&	unknown() { return unknown_; }
		static void				initGlobal();

	private:
		string		name_;
		string		group_;
		rgba_t		colour_;
		int			radius_;
		int			height_;
		fpoint2_t	scale_;
		bool		angled_;
		bool		hanging_;
		bool		shrink_;
		bool		fullbright_;
		bool		decoration_;
		int			zeth_icon_;
		string		sprite_;
		string		icon_;
		string		translation_;
		string		palette_;
		ArgSpec		args_;
		bool		decorate_;
		bool		solid_;
		int			next_type_;
		int			next_args_;
		int			flags_;
		TagType		tagged_;
		int			number_;

		static ThingType	unknown_;
	};
}
