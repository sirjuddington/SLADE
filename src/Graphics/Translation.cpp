
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Translation.cpp
// Description: Translation class. Encapsulates a palette translation.
//              A translation contains one or more translation ranges,
//              where each range has an origin palette range and some kind
//              of target range. The target range can be another palette
//              range, a colour gradient or a desaturated colour gradient.
//              eg:
//              Palette range: 0...16 -> 32...48
//                  (in zdoom format: "0:16=32:48")
//              Colour gradient: 0...16 -> Red...Black
//                  (in zdoom format: "0:16=[255,0,0]:[0,0,0]")
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Translation.h"
#include "Archive/ArchiveManager.h"
#include "MainEditor/MainEditor.h"
#include "Palette/Palette.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Float, col_greyscale_r)
EXTERN_CVAR(Float, col_greyscale_g)
EXTERN_CVAR(Float, col_greyscale_b)


// -----------------------------------------------------------------------------
//
// Constants
//
// -----------------------------------------------------------------------------
namespace
{
// Colours used by the "Ice" translation, based on the Hexen palette
ColRGBA IceRange[16] = {
	ColRGBA(10, 8, 18),     ColRGBA(15, 15, 26),    ColRGBA(20, 16, 36),    ColRGBA(30, 26, 46),
	ColRGBA(40, 36, 57),    ColRGBA(50, 46, 67),    ColRGBA(59, 57, 78),    ColRGBA(69, 67, 88),
	ColRGBA(79, 77, 99),    ColRGBA(89, 87, 109),   ColRGBA(99, 97, 120),   ColRGBA(109, 107, 130),
	ColRGBA(118, 118, 141), ColRGBA(128, 128, 151), ColRGBA(138, 138, 162), ColRGBA(148, 148, 172),
};

enum SpecialBlend
{
	Ice        = 0,
	DesatFirst = 1,
	DesatLast  = 31,
	Inverse,
	Red,
	Green,
	Blue,
	Gold,
	Invalid,
};
} // namespace


// -----------------------------------------------------------------------------
//
// Translation Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parses a text definition [def] (in zdoom format, detailed here:
// http://zdoom.org/wiki/Translation)
// -----------------------------------------------------------------------------
void Translation::parse(string def)
{
	// Test for ZDoom built-in translation
	string test = def.Lower();
	string temp;
	if (test == "inverse")
	{
		built_in_name_ = "Inverse";
		return;
	}
	else if (test == "gold")
	{
		built_in_name_ = "Gold";
		return;
	}
	else if (test == "red")
	{
		built_in_name_ = "Red";
		return;
	}
	else if (test == "green")
	{
		built_in_name_ = "Green";
		return;
	}
	else if (test == "blue")
	{
		built_in_name_ = "Blue";
		return;
	}
	else if (test == "ice")
	{
		built_in_name_ = "Ice";
		return;
	}
	else if (test.StartsWith("desaturate,", &temp))
	{
		built_in_name_ = "Desaturate";
		desat_amount_  = std::max<uint8_t>(std::min<uint8_t>(StringUtils::toInt(temp), 31), 1);
		return;
	}

	// Get Hexen tables
	else if (test.StartsWith("\"$@", &temp))
	{
		temp.RemoveLast(1); // remove the closing '\"'
		auto trantbl = App::archiveManager().getResourceEntry(temp);

		if (trantbl && trantbl->size() == 256)
		{
			read(trantbl->rawData());
			return;
		}
	}
	// Test for hardcoded predefined translations
	else
		def = getPredefined(def);

	// Now we're guaranteed to have normal translation strings to parse
	Tokenizer tz;
	tz.setSpecialCharacters(",");
	tz.openString(def);
	parseRange(tz.current().text);
	while (tz.advIfNext(','))
		parseRange(tz.next().text);
}

// -----------------------------------------------------------------------------
// Parses a single translation range
// -----------------------------------------------------------------------------
void Translation::parseRange(const string& range)
{
	// Open definition string for processing w/tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters("[]:%,=#@$");
	tz.openString(range);
	Log::debug("Processing range " + range);

	// Read original range
	int o_start, o_end;
	tz.current().toInt(o_start);
	if (tz.advIfNext(':'))
		tz.next().toInt(o_end);
	else
		o_end = o_start;

	// Check for =
	if (!tz.advIfNext('='))
		return;

	// Check for reverse origin range
	const bool reverse = (o_start > o_end);

	// Type of translation depends on next token
	if (tz.advIfNext('['))
	{
		// Colour translation
		int sr, sg, sb, er, eg, eb;

		// Read start colour
		tz.next().toInt(sr);
		if (!tz.advIfNext(','))
			return;
		tz.next().toInt(sg);
		if (!tz.advIfNext(','))
			return;
		tz.next().toInt(sb);

		// Syntax check
		if (!tz.advIfNext(']'))
			return;
		if (!tz.advIfNext(':'))
			return;
		if (!tz.advIfNext('['))
			return;

		// Read end colour
		tz.next().toInt(er);
		if (!tz.advIfNext(','))
			return;
		tz.next().toInt(eg);
		if (!tz.advIfNext(','))
			return;
		tz.next().toInt(eb);

		if (!tz.advIfNext(']'))
			return;

		// Add translation
		ColRGBA col_start{ (uint8_t)sr, (uint8_t)sg, (uint8_t)sb };
		ColRGBA col_end{ (uint8_t)sr, (uint8_t)sg, (uint8_t)sb };
		if (reverse)
			translations_.emplace_back(new TransRangeColour{ { o_end, o_start }, col_end, col_start });
		else
			translations_.emplace_back(new TransRangeColour{ { o_start, o_end }, col_start, col_end });
	}
	else if (tz.advIfNext('%'))
	{
		// Desat colour translation
		float sr, sg, sb, er, eg, eb;

		if (!tz.advIfNext('['))
			return;

		// Read start colour
		tz.next().toFloat(sr);
		if (!tz.advIfNext(','))
			return;
		tz.next().toFloat(sg);
		if (!tz.advIfNext(','))
			return;
		tz.next().toFloat(sb);

		// Syntax check
		if (!tz.advIfNext(']'))
			return;
		if (!tz.advIfNext(':'))
			return;
		if (!tz.advIfNext('['))
			return;

		// Read end colour
		tz.next().toFloat(er);
		if (!tz.advIfNext(','))
			return;
		tz.next().toFloat(eg);
		if (!tz.advIfNext(','))
			return;
		tz.next().toFloat(eb);

		if (!tz.advIfNext(']'))
			return;

		// Add translation
		if (reverse)
			translations_.emplace_back(new TransRangeDesat{ { o_end, o_start }, { er, eg, eb }, { sr, sg, sb } });
		else
			translations_.emplace_back(new TransRangeDesat{ { o_start, o_end }, { sr, sg, sb }, { er, eg, eb } });
	}
	else if (tz.advIfNext('#'))
	{
		// Colourise translation
		int r, g, b;
		if (!tz.advIfNext('['))
			return;
		tz.next().toInt(r);
		if (!tz.advIfNext(','))
			return;
		tz.next().toInt(g);
		if (!tz.advIfNext(','))
			return;
		tz.next().toInt(b);
		if (!tz.advIfNext(']'))
			return;

		translations_.emplace_back(new TransRangeBlend{ { o_start, o_end }, { (uint8_t)r, (uint8_t)g, (uint8_t)b } });
	}
	else if (tz.advIfNext('@'))
	{
		// Tint translation
		int amount, r, g, b;

		tz.next().toInt(amount);
		if (!tz.advIfNext('['))
			return;
		tz.next().toInt(r);
		if (!tz.advIfNext(','))
			return;
		tz.next().toInt(g);
		if (!tz.advIfNext(','))
			return;
		tz.next().toInt(b);
		if (!tz.advIfNext(']'))
			return;

		translations_.emplace_back(
			new TransRangeTint{ { o_start, o_end }, { (uint8_t)r, (uint8_t)g, (uint8_t)b }, (uint8_t)amount });
	}
	else if (tz.advIfNext('$'))
	{
		translations_.emplace_back(new TransRangeSpecial{ { o_start, o_end }, tz.next().text });
	}
	else
	{
		// Palette range translation
		int d_start, d_end;

		// Read range values
		tz.next().toInt(d_start);
		if (!tz.advIfNext(':'))
			d_end = d_start;
		else
			tz.next().toInt(d_end);

		// Add translation
		if (reverse)
			translations_.emplace_back(new TransRangePalette{ { o_end, o_start }, { d_end, d_start } });
		else
			translations_.emplace_back(new TransRangePalette{ { o_start, o_end }, { d_start, d_end } });
	}
}

// -----------------------------------------------------------------------------
// Read an entry as a translation table. We're only looking for translations
// where the original range and the target range have the same length, so the
// index value is only ever increased by 1. This should be enough to handle
// Hexen. Asymmetric translations or reversed translations would need a lot
// more heuristics to be handled appropriately. And of course, we're not
// handling any sort of palettized translations to RGB gradients. In short,
// converting a translation string to a translation table would be lossy.
// -----------------------------------------------------------------------------
void Translation::read(const uint8_t* data)
{
	int     i = 0;
	uint8_t val, o_start, o_end, d_start, d_end;
	o_start = 0;
	d_start = val = data[0];
	while (i < 255)
	{
		++i;
		if ((data[i] != (val + 1)) || (i == 255))
		{
			o_end = i - 1;
			d_end = val;
			// Only keep actual translations
			if (o_start != d_start && o_end != d_end)
				translations_.emplace_back(new TransRangePalette{ { o_start, o_end }, { d_start, d_end } });
			o_start = i;
			d_start = data[i];
		}
		val = data[i];
	}
	Log::info(3, S_FMT("Translation table analyzed as " + asText()));
}

// -----------------------------------------------------------------------------
// Returns a string representation of the translation (in zdoom format)
// -----------------------------------------------------------------------------
string Translation::asText()
{
	string ret;

	if (built_in_name_.IsEmpty())
	{
		// Go through translation ranges
		for (auto& translation : translations_)
			ret += S_FMT("\"%s\", ", translation->asText()); // Add range to string

		// If any translations were defined, remove last ", "
		if (!ret.IsEmpty())
			ret.RemoveLast(2);
	}
	else
	{
		// ZDoom built-in translation
		ret = built_in_name_;
		if (built_in_name_ == "Desaturate")
			ret += S_FMT(", %d", desat_amount_);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Clears the translation
// -----------------------------------------------------------------------------
void Translation::clear()
{
	translations_.clear();
	built_in_name_ = "";
	desat_amount_  = 0;
}

// -----------------------------------------------------------------------------
// Copies translation information from [copy]
// -----------------------------------------------------------------------------
void Translation::copy(const Translation& copy)
{
	// Clear current definitions
	clear();

	// Copy translations
	for (auto& translation : copy.translations_)
	{
		if (translation->type() == TransRange::Type::Palette)
			translations_.emplace_back(new TransRangePalette{ dynamic_cast<TransRangePalette&>(*translation) });
		if (translation->type_ == TransRange::Type::Colour)
			translations_.emplace_back(new TransRangeColour{ dynamic_cast<TransRangeColour&>(*translation) });
		if (translation->type_ == TransRange::Type::Desat)
			translations_.emplace_back(new TransRangeDesat{ dynamic_cast<TransRangeDesat&>(*translation) });
		if (translation->type_ == TransRange::Type::Blend)
			translations_.emplace_back(new TransRangeBlend{ dynamic_cast<TransRangeBlend&>(*translation) });
		if (translation->type_ == TransRange::Type::Tint)
			translations_.emplace_back(new TransRangeTint{ dynamic_cast<TransRangeTint&>(*translation) });
		if (translation->type_ == TransRange::Type::Special)
			translations_.emplace_back(new TransRangeSpecial{ dynamic_cast<TransRangeSpecial&>(*translation) });
	}

	built_in_name_ = copy.built_in_name_;
	desat_amount_  = copy.desat_amount_;
}

// -----------------------------------------------------------------------------
// Returns the translation range at [index]
// -----------------------------------------------------------------------------
TransRange* Translation::range(unsigned index)
{
	if (index >= translations_.size())
		return nullptr;
	else
		return translations_[index].get();
}

// -----------------------------------------------------------------------------
// Apply the translation to the given color
// -----------------------------------------------------------------------------
ColRGBA Translation::translate(ColRGBA col, Palette* pal)
{
	ColRGBA colour(col);
	if (pal == nullptr)
		pal = MainEditor::currentPalette();
	uint8_t i = (col.index == -1) ? pal->nearestColour(col) : col.index;

	// Handle ZDoom's predefined texture blending:
	// blue, gold, green, red, ice, inverse, and desaturate
	if (!built_in_name_.empty())
	{
		int type = SpecialBlend::Invalid;
		if (S_CMPNOCASE(built_in_name_, "ice"))
			type = SpecialBlend::Ice;
		else if (S_CMPNOCASE(built_in_name_, "inverse"))
			type = SpecialBlend::Inverse;
		else if (S_CMPNOCASE(built_in_name_, "red"))
			type = SpecialBlend::Red;
		else if (S_CMPNOCASE(built_in_name_, "green"))
			type = SpecialBlend::Green;
		else if (S_CMPNOCASE(built_in_name_, "blue"))
			type = SpecialBlend::Blue;
		else if (S_CMPNOCASE(built_in_name_, "gold"))
			type = SpecialBlend::Gold;
		else if (S_CMPNOCASE(built_in_name_, "desaturate"))
			type = desat_amount_; // min 1, max 31 required

		return specialBlend(col, type, pal);
	}

	// Check for perfect palette matches
	bool match = col.equals(pal->colour(i));

	// Go through each translation component
	// for (unsigned a = 0; a < nRanges(); a++)
	for (auto& range : translations_)
	{
		// TransRange::UPtr& range = translations_[a];

		// Check pixel is within translation range
		if (i < range->start() || i > range->end())
			continue;

		// Only allow exact matches unless the translation applies to all colours
		if (!match && range->start() != 0 && range->end() != 255)
			continue;

		// Palette range translation
		if (range->type() == TransRange::Type::Palette)
		{
			auto tp = dynamic_cast<TransRangePalette*>(range.get());

			// Figure out how far along the range this colour is
			double range_frac = 0;
			if (tp->start() != tp->end())
				range_frac = double(i - tp->start()) / double(tp->end() - tp->start());

			// Determine destination palette index
			uint8_t di = tp->dStart() + range_frac * (tp->dEnd() - tp->dStart());

			// Apply new colour
			auto c       = pal->colour(di);
			colour.r     = c.r;
			colour.g     = c.g;
			colour.b     = c.b;
			colour.a     = c.a;
			colour.index = di;
		}

		// Colour range
		else if (range->type() == TransRange::Type::Colour)
		{
			auto tc = dynamic_cast<TransRangeColour*>(range.get());

			// Figure out how far along the range this colour is
			double range_frac = 0;
			if (tc->start() != tc->end())
				range_frac = double(i - tc->start()) / double(tc->end() - tc->start());

			// Apply new colour
			colour.r     = tc->startColour().r + range_frac * (tc->endColour().r - tc->startColour().r);
			colour.g     = tc->startColour().g + range_frac * (tc->endColour().g - tc->startColour().g);
			colour.b     = tc->startColour().b + range_frac * (tc->endColour().b - tc->startColour().b);
			colour.index = pal->nearestColour(colour);
		}

		// Desaturated colour range
		else if (range->type() == TransRange::Type::Desat)
		{
			auto td = dynamic_cast<TransRangeDesat*>(range.get());

			// Get greyscale colour
			auto  gcol = pal->colour(i);
			float grey = (gcol.r * 0.3f + gcol.g * 0.59f + gcol.b * 0.11f) / 255.0f;

			// Apply new colour
			colour.r = std::min<uint8_t>(
				255, int((td->rgbStart().r + grey * (td->rgbEnd().r - td->rgbStart().r)) * 255.0f));
			colour.g = std::min<uint8_t>(
				255, int((td->rgbStart().g + grey * (td->rgbEnd().g - td->rgbStart().g)) * 255.0f));
			colour.b = std::min<uint8_t>(
				255, int((td->rgbStart().b + grey * (td->rgbEnd().b - td->rgbStart().b)) * 255.0f));
			colour.index = pal->nearestColour(colour);
		}

		// Blended range
		else if (range->type() == TransRange::Type::Blend)
		{
			auto tc = dynamic_cast<TransRangeBlend*>(range.get());

			// Get colours
			auto blend = tc->colour();

			// Colourise
			float grey = (col.r * col_greyscale_r + col.g * col_greyscale_g + col.b * col_greyscale_b) / 255.0f;
			if (grey > 1.0)
				grey = 1.0;

			// Apply new colour
			colour.r     = blend.r * grey;
			colour.g     = blend.g * grey;
			colour.b     = blend.b * grey;
			colour.index = pal->nearestColour(colour);
		}

		// Tinted range
		else if (range->type() == TransRange::Type::Tint)
		{
			auto tt = dynamic_cast<TransRangeTint*>(range.get());

			// Get colours
			auto tint = tt->colour();

			// Colourise
			float amount  = tt->amount() * 0.01f;
			float inv_amt = 1.0f - amount;

			// Apply new colour
			colour.r     = col.r * inv_amt + tint.r * amount;
			colour.g     = col.g * inv_amt + tint.g * amount;
			colour.b     = col.b * inv_amt + tint.b * amount;
			colour.index = pal->nearestColour(colour);
		}

		// Special range
		else if (range->type() == TransRange::Type::Special)
		{
			auto    ts   = dynamic_cast<TransRangeSpecial*>(range.get());
			string  spec = ts->special();
			uint8_t type = Invalid;
			if (S_CMPNOCASE(spec, "ice"))
				type = SpecialBlend::Ice;
			else if (S_CMPNOCASE(spec, "inverse"))
				type = SpecialBlend::Inverse;
			else if (S_CMPNOCASE(spec, "red"))
				type = SpecialBlend::Red;
			else if (S_CMPNOCASE(spec, "green"))
				type = SpecialBlend::Green;
			else if (S_CMPNOCASE(spec, "blue"))
				type = SpecialBlend::Blue;
			else if (S_CMPNOCASE(spec, "gold"))
				type = SpecialBlend::Gold;
			else if (spec.Lower().StartsWith("desat"))
			{
				// This relies on SpecialBlend::1 to ::31 being occupied with desat
				long temp;
				if (spec.Right(2).ToLong(&temp) && temp < 32 && temp > 0)
					type = temp;
			}
			return specialBlend(col, type, pal);
		}
	}
	return colour;
}

// -----------------------------------------------------------------------------
// Apply one of the special colour blending modes from ZDoom:
// Desaturate, Ice, Inverse, Blue, Gold, Green, Red.
// -----------------------------------------------------------------------------
ColRGBA Translation::specialBlend(ColRGBA col, uint8_t type, Palette* pal) const
{
	// Abort just in case
	if (type == SpecialBlend::Invalid)
		return col;

	auto colour = col;

	// Get greyscale using ZDoom formula
	float grey = (col.r * 77 + col.g * 143 + col.b * 37) / 256.f;

	// Ice is a special case as it uses a colour range derived
	// from the Hexen palette instead of a linear gradient.
	if (type == Ice)
	{
		// Determine destination palette index in IceRange
		uint8_t di   = std::min<uint8_t>(((int)grey >> 4), 15);
		auto    c    = IceRange[di];
		colour.r     = c.r;
		colour.g     = c.g;
		colour.b     = c.b;
		colour.a     = c.a;
		colour.index = pal->nearestColour(colour);
	}
	// Desaturated blending goes from no effect to nearly fully desaturated
	else if (type >= DesatFirst && type <= DesatLast)
	{
		float amount = type - 1; // get value between 0 and 30

		colour.r     = std::min<uint8_t>(255, int((colour.r * (31 - amount) + grey * amount) / 31));
		colour.g     = std::min<uint8_t>(255, int((colour.g * (31 - amount) + grey * amount) / 31));
		colour.b     = std::min<uint8_t>(255, int((colour.b * (31 - amount) + grey * amount) / 31));
		colour.index = pal->nearestColour(colour);
	}
	// All others are essentially preset desaturated translations
	else
	{
		float sr, sg, sb, er, eg, eb;      // start and end ranges
		sr = sg = sb = er = eg = eb = 0.0; // let's init them to 0.

		switch (type)
		{
		case Inverse:
			// inverted grayscale: Doom invulnerability, Strife sigil
			// start white, ends black
			sr = sg = sb = 1.0;
			break;
		case Gold:
			// Heretic invulnerability
			// starts black, ends reddish yellow
			er = 1.5;
			eg = 0.75;
			break;
		case Red:
			// Skulltag doomsphere
			// starts black, ends red
			er = 1.5;
			break;
		case Green:
			// Skulltag guardsphere
			// starts black, ends greenish-white
			er = 1.25;
			eg = 1.5;
			eb = 1.0;
			break;
		case Blue:
			// Hacx invulnerability
			// starts black, ends blue
			eb = 1.5;
		default: break;
		}
		// Apply new colour
		colour.r     = std::min<uint8_t>(255, int(sr + grey * (er - sr)));
		colour.g     = std::min<uint8_t>(255, int(sg + grey * (eg - sg)));
		colour.b     = std::min<uint8_t>(255, int(sb + grey * (eb - sb)));
		colour.index = pal->nearestColour(colour);
	}
	return colour;
}

// -----------------------------------------------------------------------------
// Adds a new translation range of [type] at [pos] in the list
// -----------------------------------------------------------------------------
void Translation::addRange(TransRange::Type type, int pos)
{
	// Create range
	TransRange::UPtr       tr;
	TransRange::IndexRange range{ 0, 0 };
	switch (type)
	{
	case TransRange::Type::Colour: tr = std::make_unique<TransRangeColour>(range); break;
	case TransRange::Type::Desat: tr = std::make_unique<TransRangeDesat>(range); break;
	case TransRange::Type::Blend: tr = std::make_unique<TransRangeBlend>(range); break;
	case TransRange::Type::Tint: tr = std::make_unique<TransRangeTint>(range); break;
	case TransRange::Type::Special: tr = std::make_unique<TransRangeSpecial>(range); break;
	default: tr = std::make_unique<TransRangePalette>(range, range); break;
	}

	// Add to list
	if (pos < 0 || pos >= (int)translations_.size())
		translations_.push_back(std::move(tr));
	else
		translations_.insert(translations_.begin() + pos, std::move(tr));
}

// -----------------------------------------------------------------------------
// Translation::removeRange
//
// Removes the translation range at [pos]
// -----------------------------------------------------------------------------
void Translation::removeRange(int pos)
{
	// Check position
	if (pos < 0 || pos >= (int)translations_.size())
		return;

	// Remove it
	translations_.erase(translations_.begin() + pos);
}

// -----------------------------------------------------------------------------
// Swaps the translation range at [pos1] with the one at [pos2]
// -----------------------------------------------------------------------------
void Translation::swapRanges(int pos1, int pos2)
{
	// Check positions
	if (pos1 < 0 || pos2 < 0 || pos1 >= (int)translations_.size() || pos2 >= (int)translations_.size())
		return;

	// Swap them
	translations_[pos1].swap(translations_[pos2]);
}

// -----------------------------------------------------------------------------
// Replaces a hardcoded translation name with its transcription
// -----------------------------------------------------------------------------
string Translation::getPredefined(string def)
{
	// Some hardcoded translations from ZDoom, used in config files
	if (def == "\"doom0\"")
		def = "\"112:127=96:111\"";
	else if (def == "\"doom1\"")
		def = "\"112:127=64:79\"";
	else if (def == "\"doom2\"")
		def = "\"112:127=32:47\"";
	else if (def == "\"doom3\"")
		def = "\"112:127=88:103\"";
	else if (def == "\"doom4\"")
		def = "\"112:127=56:71\"";
	else if (def == "\"doom5\"")
		def = "\"112:127=176:191\"";
	else if (def == "\"doom6\"")
		def = "\"112:127=192:207\"";
	else if (def == "\"heretic0\"")
		def = "\"225:240=114:129\"";
	else if (def == "\"heretic1\"")
		def = "\"225:240=145:160\"";
	else if (def == "\"heretic2\"")
		def = "\"225:240=190:205\"";
	else if (def == "\"heretic3\"")
		def = "\"225:240=67:82\"";
	else if (def == "\"heretic4\"")
		def = "\"225:240=9:24\"";
	else if (def == "\"heretic5\"")
		def = "\"225:240=74:89\"";
	else if (def == "\"heretic6\"")
		def = "\"225:240=150:165\"";
	else if (def == "\"heretic7\"")
		def = "\"225:240=192:207\"";
	else if (def == "\"heretic8\"")
		def = "\"225:240=95:110\"";
	else if (def == "\"strife0\"")
		def = "\"32:63=0:31\", \"128:143=64:79\", \"241:246=224:229\", \"247:251=241:245\"";
	else if (def == "\"strife1\"")
		def = "\"32:63=0:31\", \"128:143=176:191\"";
	else if (def == "\"strife2\"")
		def = "\"32:47=208:223\", \"48:63=208:223\", \"128:143=16:31\"";
	else if (def == "\"strife3\"")
		def = "\"32:47=208:223\", \"48:63=208:223\", \"128:143=48:63\"";
	else if (def == "\"strife4\"")
		def = "\"32:63=0:31\", \"80:95=128:143\", \"128:143=80:95\", \"192:223=160:191\"";
	else if (def == "\"strife5\"")
		def = "\"32:63=0:31\", \"80:95=16:31\", \"128:143=96:111\", \"192:223=32:63\"";
	else if (def == "\"strife6\"")
		def = "\"32:63=0:31\", \"80:95=64:79\", \"128:143=144:159\", \"192=1\", \"193:223=1:31\"";
	else if (def == "\"chex0\"")
		def = "\"192:207=112:127\"";
	else if (def == "\"chex1\"")
		def = "\"192:207=96:111\"";
	else if (def == "\"chex2\"")
		def = "\"192:207=64:79\"";
	else if (def == "\"chex3\"")
		def = "\"192:207=32:47\"";
	else if (def == "\"chex4\"")
		def = "\"192:207=88:103\"";
	else if (def == "\"chex5\"")
		def = "\"192:207=56:71\"";
	else if (def == "\"chex6\"")
		def = "\"192:207=176:191\"";
	// Some more from Eternity
	else if (def == "\"tomato\"")
		def =
			"\"112:113=171:171\", \"114:114=172:172\", \"115:122=173:187\", \"123:124=188:189\", \"125:126=45:47\", "
			"\"127:127=1:1\"";
	else if (def == "\"dirt\"")
		def = "\"112:117=128:133\", \"118:120=135:137\", \"121:123=139:143\", \"124:125=237:239\", \"126:127=1:2\"";
	else if (def == "\"blue\"")
		def = "\"112:121=197:206\", \"122:127=240:245";
	else if (def == "\"gold\"")
		def = "\"112:113=160:160\", \"114:119=161:166\", \"120:123=236:239\", \"124:125=1:2\", \"126:127=7:8\"";
	else if (def == "\"sea\"")
		def = "\"112:112=91:91\", \"113:114=94:95\", \"115:122=152:159\", \"123:126=9:12\", \"127:127=8:8\"";
	else if (def == "\"black\"")
		def = "\"112:112=101:101\", \"113:121=103:111\", \"122:125=5:8\", \"126:127=0:0\"";
	else if (def == "\"purple\"")
		def = "\"112:113=4:4\", \"114:115=170:170\", \"116:125=250:254\", \"126:127=46:46\"";
	else if (def == "\"vomit\"")
		def = "\"112:119=209:216\", \"120:121=218:220\", \"122:124=69:75\", \"125:127=237:239\"";
	else if (def == "\"pink\"")
		def =
			"\"112:113=16:17\", \"114:117=19:25\", \"118:119=27:28\", \"120:124=30:38\", \"125:126=41:43\", "
			"\"127:127=46:46\"";
	else if (def == "\"cream\"")
		def =
			"\"112:112=4:4\", \"113:118=48:63\", \"119:119=65:65\", \"120:124=68:76\", \"125:126=77:79\", "
			"\"127:127=1:1\"";
	else if (def == "\"white\"")
		def = "\"112:112=4:4\", \"113:115=80:82\", \"116:117=84:86\", \"118:120=89:93\", \"121:127=96:108\"";
	// And why not this one too
	else if (def == "\"stealth\"")
		def = "\"0:255=%[0.00,0.00,0.00]:[1.31,0.84,0.84]\"";

	return def;
}
