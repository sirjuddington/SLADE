
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Translation.cpp
 * Description: Translation class. Encapsulates a palette translation.
 *              A translation contains one or more translation ranges,
 *              where each range has an origin palette range and some kind
 *              of target range. The target range can be another palette
 *              range, a colour gradient or a desaturated colour gradient.
 *              eg:
 *              Palette range: 0...16 -> 32...48		(in zdoom format: "0:16=32:48")
 *              Colour gradient: 0...16 -> Red...Black	(in zdoom format: "0:16=[255,0,0]:[0,0,0]")
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "Translation.h"
#include "Utility/Tokenizer.h"
#include "Palette/Palette.h"
#include "MainEditor/MainWindow.h"
#include "Archive/ArchiveManager.h"

 /*******************************************************************
 * VARIABLES
 *******************************************************************/
EXTERN_CVAR(Float, col_greyscale_r)
EXTERN_CVAR(Float, col_greyscale_g)
EXTERN_CVAR(Float, col_greyscale_b)

/*******************************************************************
* CONSTANTS
*******************************************************************/

// Colours used by the "Ice" translation, based on the Hexen palette
rgba_t IceRange[16] = 
{
	rgba_t( 10,   8,  18), rgba_t( 15,  15,  26),
	rgba_t( 20,  16,  36), rgba_t( 30,  26,  46),
	rgba_t( 40,  36,  57), rgba_t( 50,  46,  67),
	rgba_t( 59,  57,  78), rgba_t( 69,  67,  88),
	rgba_t( 79,  77,  99), rgba_t( 89,  87, 109),
	rgba_t( 99,  97, 120), rgba_t(109, 107, 130),
	rgba_t(118, 118, 141), rgba_t(128, 128, 151),
	rgba_t(138, 138, 162), rgba_t(148, 148, 172),
};

enum SpecialBlend
{
	BLEND_ICE = 0,
	BLEND_DESAT_FIRST = 1,
	BLEND_DESAT_LAST = 31,
	BLEND_INVERSE,
	BLEND_RED,
	BLEND_GREEN,
	BLEND_BLUE,
	BLEND_GOLD,
	BLEND_INVALID,
};

/*******************************************************************
 * TRANSLATION CLASS FUNCTIONS
 *******************************************************************/

/* Translation::Translation
 * Translation class constructor
 *******************************************************************/
Translation::Translation() : built_in_name(""), desat_amount(0)
{
}

/* Translation::~Translation
 * Translation class destructor
 *******************************************************************/
Translation::~Translation()
{
	for (unsigned a = 0; a < translations.size(); a++)
		delete translations[a];
}

/* Translation::parse
 * Parses a text definition [def] (in zdoom format, detailed here:
 * http://zdoom.org/wiki/Translation)
 *******************************************************************/
void Translation::parse(string def)
{
	//wxLogMessage("Parse translation \"%s\"", def);
	
	// Test for ZDoom built-in translation
	string test = def.Lower();
	string temp;
	if (test == "inverse")
	{
		built_in_name = "Inverse";
		return;
	}
	else if (test == "gold")
	{
		built_in_name = "Gold";
		return;
	}
	else if (test == "red")
	{
		built_in_name = "Red";
		return;
	}
	else if (test == "green")
	{
		built_in_name = "Green";
		return;
	}
	else if (test == "blue")
	{
		built_in_name = "Blue";
		return;
	}
	else if (test == "ice")
	{
		built_in_name = "Ice";
		return;
	}
	else if (test.StartsWith("desaturate,", &temp))
	{
		built_in_name = "Desaturate";
		desat_amount = MAX(MIN(atoi(CHR(temp)), 31), 1);
		return;
	}

	// Some hardcoded translations from ZDoom, used in config files
	else if (test == "\"doom0\"")		def = "\"112:127=96:111\"";
	else if (test == "\"doom1\"")		def = "\"112:127=64:79\"";
	else if (test == "\"doom2\"")		def = "\"112:127=32:47\"";
	else if (test == "\"doom3\"")		def = "\"112:127=88:103\"";
	else if (test == "\"doom4\"")		def = "\"112:127=56:71\"";
	else if (test == "\"doom5\"")		def = "\"112:127=176:191\"";
	else if (test == "\"doom6\"")		def = "\"112:127=192:207\"";
	else if (test == "\"heretic0\"")	def = "\"225:240=114:129\"";
	else if (test == "\"heretic1\"")	def = "\"225:240=145:160\"";
	else if (test == "\"heretic2\"")	def = "\"225:240=190:205\"";
	else if (test == "\"heretic3\"")	def = "\"225:240=67:82\"";
	else if (test == "\"heretic4\"")	def = "\"225:240=9:24\"";
	else if (test == "\"heretic5\"")	def = "\"225:240=74:89\"";
	else if (test == "\"heretic6\"")	def = "\"225:240=150:165\"";
	else if (test == "\"heretic7\"")	def = "\"225:240=192:207\"";
	else if (test == "\"heretic8\"")	def = "\"225:240=95:110\"";
	else if (test == "\"strife0\"")		def = "\"32:63=0:31\", \"128:143=64:79\", \"241:246=224:229\", \"247:251=241:245\"";
	else if (test == "\"strife1\"")		def = "\"32:63=0:31\", \"128:143=176:191\"";
	else if (test == "\"strife2\"")		def = "\"32:47=208:223\", \"48:63=208:223\", \"128:143=16:31\"";
	else if (test == "\"strife3\"")		def = "\"32:47=208:223\", \"48:63=208:223\", \"128:143=48:63\"";
	else if (test == "\"strife4\"")		def = "\"32:63=0:31\", \"80:95=128:143\", \"128:143=80:95\", \"192:223=160:191\"";
	else if (test == "\"strife5\"")		def = "\"32:63=0:31\", \"80:95=16:31\", \"128:143=96:111\", \"192:223=32:63\"";
	else if (test == "\"strife6\"")		def = "\"32:63=0:31\", \"80:95=64:79\", \"128:143=144:159\", \"192=1\", \"193:223=1:31\"";
	else if (test == "\"chex0\"")		def = "\"192:207=112:127\"";
	else if (test == "\"chex1\"")		def = "\"192:207=96:111\"";
	else if (test == "\"chex2\"")		def = "\"192:207=64:79\"";
	else if (test == "\"chex3\"")		def = "\"192:207=32:47\"";
	else if (test == "\"chex4\"")		def = "\"192:207=88:103\"";
	else if (test == "\"chex5\"")		def = "\"192:207=56:71\"";
	else if (test == "\"chex6\"")		def = "\"192:207=176:191\"";
	// Some more from Eternity
	else if (test == "\"tomato\"")		def = "\"112:113=171:171\", \"114:114=172:172\", \"115:122=173:187\", \"123:124=188:189\", \"125:126=45:47\", \"127:127=1:1\"";
	else if (test == "\"dirt\"")		def = "\"112:117=128:133\", \"118:120=135:137\", \"121:123=139:143\", \"124:125=237:239\", \"126:127=1:2\"";
	else if (test == "\"blue\"")		def = "\"112:121=197:206\", \"122:127=240:245";
	else if (test == "\"gold\"")		def = "\"112:113=160:160\", \"114:119=161:166\", \"120:123=236:239\", \"124:125=1:2\", \"126:127=7:8\"";
	else if (test == "\"sea\"")			def = "\"112:112=91:91\", \"113:114=94:95\", \"115:122=152:159\", \"123:126=9:12\", \"127:127=8:8\"";
	else if (test == "\"black\"")		def = "\"112:112=101:101\", \"113:121=103:111\", \"122:125=5:8\", \"126:127=0:0\"";
	else if (test == "\"purple\"")		def = "\"112:113=4:4\", \"114:115=170:170\", \"116:125=250:254\", \"126:127=46:46\"";
	else if (test == "\"vomit\"")		def = "\"112:119=209:216\", \"120:121=218:220\", \"122:124=69:75\", \"125:127=237:239\"";
	else if (test == "\"pink\"")		def = "\"112:113=16:17\", \"114:117=19:25\", \"118:119=27:28\", \"120:124=30:38\", \"125:126=41:43\", \"127:127=46:46\"";
	else if (test == "\"cream\"")		def = "\"112:112=4:4\", \"113:118=48:63\", \"119:119=65:65\", \"120:124=68:76\", \"125:126=77:79\", \"127:127=1:1\"";
	else if (test == "\"white\"")		def = "\"112:112=4:4\", \"113:115=80:82\", \"116:117=84:86\", \"118:120=89:93\", \"121:127=96:108\"";

	// Get Hexen tables
	else if (test.StartsWith("\"$@", &temp))
	{
		temp.RemoveLast(1); // remove the closing '\"'
		ArchiveEntry *trantbl = theArchiveManager->getResourceEntry(temp);

		if (trantbl && trantbl->getSize() == 256)
		{
			read(trantbl->getData());
			return;
		}
	}
	Tokenizer tz;
	tz.openString(def);
	string token = tz.getToken();
	while (!token.IsEmpty())
	{
		parseRange(token);
		tz.getToken();
		token = tz.getToken();
	}

}

/* Translation::parseRange
 * Parses a single translation range
 *******************************************************************/
void Translation::parseRange(string range)
{
	// Open definition string for processing w/tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters("[]:%,=#@$");
	tz.openString(range);

	// Read original range
	uint8_t o_start, o_end;
	o_start = tz.getInteger();
	if (tz.peekToken() == "=") o_end = o_start;
	else if (!tz.checkToken(":")) return;
	else o_end = tz.getInteger();
	if (!tz.checkToken("=")) return;

	// Check for reverse origin range
	bool reverse = (o_start > o_end);

	// Type of translation depends on next token
	if (tz.peekToken() == "[")
	{
		// Colour translation
		rgba_t start, end;

		tz.getToken();	// Skip [

						// Read start colour
		start.r = tz.getInteger();
		if (!tz.checkToken(",")) return;
		start.g = tz.getInteger();
		if (!tz.checkToken(",")) return;
		start.b = tz.getInteger();

		if (!tz.checkToken("]")) return;
		if (!tz.checkToken(":")) return;
		if (!tz.checkToken("[")) return;

		// Read end colour
		end.r = tz.getInteger();
		if (!tz.checkToken(",")) return;
		end.g = tz.getInteger();
		if (!tz.checkToken(",")) return;
		end.b = tz.getInteger();

		if (!tz.checkToken("]")) return;

		// Add translation
		TransRangeColour* tr = new TransRangeColour();
		if (reverse)
		{
			tr->o_start = o_end;
			tr->o_end = o_start;
			tr->d_start.set(end);
			tr->d_end.set(start);
		}
		else
		{
			tr->o_start = o_start;
			tr->o_end = o_end;
			tr->d_start.set(start);
			tr->d_end.set(end);
		}
		translations.push_back(tr);
	}
	else if (tz.peekToken() == "%")
	{
		// Desat colour translation
		float sr, sg, sb, er, eg, eb;

		tz.getToken();	// Skip %
		if (!tz.checkToken("[")) return;

		// Read start colour
		sr = tz.getFloat();
		if (!tz.checkToken(",")) return;
		sg = tz.getFloat();
		if (!tz.checkToken(",")) return;
		sb = tz.getFloat();

		if (!tz.checkToken("]")) return;
		if (!tz.checkToken(":")) return;
		if (!tz.checkToken("[")) return;

		// Read end colour
		er = tz.getFloat();
		if (!tz.checkToken(",")) return;
		eg = tz.getFloat();
		if (!tz.checkToken(",")) return;
		eb = tz.getFloat();

		if (!tz.checkToken("]")) return;

		// Add translation
		TransRangeDesat* tr = new TransRangeDesat();
		if (reverse)
		{
			tr->o_start = o_end;
			tr->o_end = o_start;
			tr->d_sr = er;
			tr->d_sg = eg;
			tr->d_sb = eb;
			tr->d_er = sr;
			tr->d_eg = sg;
			tr->d_eb = sb;
		}
		else
		{
			tr->o_start = o_start;
			tr->o_end = o_end;
			tr->d_sr = sr;
			tr->d_sg = sg;
			tr->d_sb = sb;
			tr->d_er = er;
			tr->d_eg = eg;
			tr->d_eb = eb;
		}
		translations.push_back(tr);
	}
	else if (tz.peekToken() == "#")
	{
		// Colourise translation
		rgba_t col;
		tz.skipToken(); // skip #
		if (!tz.checkToken("[")) return;
		col.r = tz.getInteger();
		if (!tz.checkToken(",")) return;
		col.g = tz.getInteger();
		if (!tz.checkToken(",")) return;
		col.b = tz.getInteger();
		if (!tz.checkToken("]")) return;
		TransRangeBlend* tr = new TransRangeBlend();
		tr->o_start = o_start;
		tr->o_end = o_end;
		tr->setColour(col);
		translations.push_back(tr);
	}
	else if (tz.peekToken() == '@')
	{
		// Tint translation
		rgba_t col;
		uint8_t amount;
		tz.skipToken(); // skip @
		amount = tz.getInteger();
		if (!tz.checkToken("[")) return;
		col.r = tz.getInteger();
		if (!tz.checkToken(",")) return;
		col.g = tz.getInteger();
		if (!tz.checkToken(",")) return;
		col.b = tz.getInteger();
		if (!tz.checkToken("]")) return;
		TransRangeTint* tr = new TransRangeTint();
		tr->o_start = o_start;
		tr->o_end = o_end;
		tr->setColour(col);
		tr->setAmount(amount);
		translations.push_back(tr);
	}
	else if (tz.peekToken() == '$')
	{
		tz.skipToken();
		string special = tz.getToken();
		TransRangeSpecial* tr = new TransRangeSpecial();
		tr->o_start = o_start;
		tr->o_end = o_end;
		tr->special = special;
		translations.push_back(tr);
	}
	else
	{
		// Palette range translation
		uint8_t d_start, d_end;

		// Read range values
		d_start = tz.getInteger();
		if (!tz.checkToken(":")) d_end = d_start;
		else d_end = tz.getInteger();

		// Add translation
		TransRangePalette* tr = new TransRangePalette();
		if (reverse)
		{
			tr->o_start = o_end;
			tr->o_end = o_start;
			tr->d_start = d_end;
			tr->d_end = d_start;
		}
		else
		{
			tr->o_start = o_start;
			tr->o_end = o_end;
			tr->d_start = d_start;
			tr->d_end = d_end;
		}
		translations.push_back(tr);
	}
}

/* Translation::read
 * Read an entry as a translation table. We're only looking for 
 * translations where the original range and the target range have 
 * the same length, so the index value is only ever increased by 1. 
 * This should be enough to handle Hexen. Asymmetric translations
 * or reversed translations would need a lot more heuristics to be
 * handled appropriately. And of course, we're not handling any sort
 * of palettized translations to RGB gradients. In short, converting
 * a translation string to a translation table would be lossy.
 *******************************************************************/
void Translation::read(const uint8_t * data)
{
	int i = 0;
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
			{
				TransRangePalette* tr = new TransRangePalette();
				tr->o_start = o_start; tr->o_end = o_end;
				tr->d_start = d_start; tr->d_end = d_end;
				translations.push_back(tr);
			}
			o_start = i;
			d_start = data[i];
		}
		val = data[i];
	}
	LOG_MESSAGE(3, "Translation table analyzed as " + asText());
}

/* Translation::asText
 * Returns a string representation of the translation
 * (in zdoom format)
 *******************************************************************/
string Translation::asText()
{
	string ret;

	if (built_in_name.IsEmpty())
	{
		// Go through translation ranges
		for (unsigned a = 0; a < translations.size(); a++)
			ret += S_FMT("\"%s\", ", translations[a]->asText());	// Add range to string

		// If any translations were defined, remove last ", "
		if (!ret.IsEmpty())
			ret.RemoveLast(2);
	}
	else
	{
		// ZDoom built-in translation
		ret = built_in_name;
		if (built_in_name == "Desaturate")
			ret += S_FMT(", %d", desat_amount);
	}

	return ret;
}

/* Translation::clear
 * Clears the translation
 *******************************************************************/
void Translation::clear()
{
	for (unsigned a = 0; a < translations.size(); a++)
		delete translations[a];
	translations.clear();
	built_in_name = "";
	desat_amount = 0;
}

/* Translation::copy
 * Copies translation information from [copy]
 *******************************************************************/
void Translation::copy(Translation& copy)
{
	// Clear current definitions
	clear();

	// Copy translations
	for (unsigned a = 0; a < copy.translations.size(); a++)
	{
		if (copy.translations[a]->type == TRANS_PALETTE)
			translations.push_back(new TransRangePalette((TransRangePalette*)copy.translations[a]));
		if (copy.translations[a]->type == TRANS_COLOUR)
			translations.push_back(new TransRangeColour((TransRangeColour*)copy.translations[a]));
		if (copy.translations[a]->type == TRANS_DESAT)
			translations.push_back(new TransRangeDesat((TransRangeDesat*)copy.translations[a]));
		if (copy.translations[a]->type == TRANS_BLEND)
			translations.push_back(new TransRangeBlend((TransRangeBlend*)copy.translations[a]));
		if (copy.translations[a]->type == TRANS_TINT)
			translations.push_back(new TransRangeTint((TransRangeTint*)copy.translations[a]));
		if (copy.translations[a]->type == TRANS_SPECIAL)
			translations.push_back(new TransRangeSpecial((TransRangeSpecial*)copy.translations[a]));
	}

	built_in_name = copy.built_in_name;
	desat_amount = copy.desat_amount;
}

/* Translation::getRange
 * Returns the translation range at [index]
 *******************************************************************/
TransRange* Translation::getRange(unsigned index)
{
	if (index >= translations.size())
		return NULL;
	else
		return translations[index];
}

/* Translation::translate
 * Apply the translation to the given color
 *******************************************************************/
rgba_t Translation::translate(rgba_t col, Palette8bit * pal)
{
	rgba_t colour(col);
	colour.blend = -1;
	if (pal == NULL) pal = thePaletteChooser->getSelectedPalette();
	uint8_t i = (col.index == -1) ? pal->nearestColour(col) : col.index;

	// Handle ZDoom's predefined texture blending:
	// blue, gold, green, red, ice, inverse, and desaturate
	if (built_in_name.size())
	{
		uint8_t type = BLEND_INVALID;
		if (S_CMPNOCASE(built_in_name, "ice"))
			type = SpecialBlend::BLEND_ICE;
		else if (S_CMPNOCASE(built_in_name, "inverse"))
			type = SpecialBlend::BLEND_INVERSE;
		else if (S_CMPNOCASE(built_in_name, "red"))
			type = SpecialBlend::BLEND_RED;
		else if (S_CMPNOCASE(built_in_name, "green"))
			type = SpecialBlend::BLEND_GREEN;
		else if (S_CMPNOCASE(built_in_name, "blue"))
			type = SpecialBlend::BLEND_BLUE;
		else if (S_CMPNOCASE(built_in_name, "gold"))
			type = SpecialBlend::BLEND_GOLD;
		else if (S_CMPNOCASE(built_in_name, "desaturate"))
			type = desat_amount; // min 1, max 31 required

		return specialBlend(col, type, pal);
	}

	// Go through each translation component
	for (unsigned a = 0; a < nRanges(); a++)
	{
		TransRange* r = translations[a];

		// Check pixel is within translation range
		if (i < r->oStart() || i > r->oEnd())
			continue;


		// Palette range translation
		if (r->getType() == TRANS_PALETTE)
		{
			TransRangePalette* tp = (TransRangePalette*) translations[a];

			// Figure out how far along the range this colour is
			double range_frac = 0;
			if (tp->oStart() != tp->oEnd())
				range_frac = double(i - tp->oStart()) / double(tp->oEnd() - tp->oStart());

			// Determine destination palette index
			uint8_t di = tp->dStart() + range_frac * (tp->dEnd() - tp->dStart());

			// Apply new colour
			rgba_t c = pal->colour(di);
			colour.r = c.r;
			colour.g = c.g;
			colour.b = c.b;
			colour.a = c.a;
			colour.index = di;
		}

		// Colour range
		else if (r->getType() == TRANS_COLOUR)
		{
			TransRangeColour* tc = (TransRangeColour*)r;

			// Figure out how far along the range this colour is
			double range_frac = 0;
			if (tc->oStart() != tc->oEnd())
				range_frac = double(i - tc->oStart()) / double(tc->oEnd() - tc->oStart());

			// Apply new colour
			colour.r = tc->dStart().r + range_frac * (tc->dEnd().r - tc->dStart().r);
			colour.g = tc->dStart().g + range_frac * (tc->dEnd().g - tc->dStart().g);
			colour.b = tc->dStart().b + range_frac * (tc->dEnd().b - tc->dStart().b);
			colour.index = pal->nearestColour(colour);
		}

		// Desaturated colour range
		else if (r->getType() == TRANS_DESAT)
		{
			TransRangeDesat* td = (TransRangeDesat*)r;

			// Get greyscale colour
			rgba_t gcol = pal->colour(i);
			float grey = (gcol.r*0.3f + gcol.g*0.59f + gcol.b*0.11f) / 255.0f;

			// Apply new colour
			colour.r = MIN(255, int((td->dSr() + grey*(td->dEr() - td->dSr()))*255.0f));
			colour.g = MIN(255, int((td->dSg() + grey*(td->dEg() - td->dSg()))*255.0f));
			colour.b = MIN(255, int((td->dSb() + grey*(td->dEb() - td->dSb()))*255.0f));
			colour.index = pal->nearestColour(colour);
		}

		// Blended range
		else if (r->getType() == TRANS_BLEND)
		{
			TransRangeBlend* tc = (TransRangeBlend*)r;

			// Get colours
			rgba_t blend = tc->getColour();

			// Colourise
			float grey = (col.r*col_greyscale_r + col.g*col_greyscale_g + col.b*col_greyscale_b) / 255.0f;
			if (grey > 1.0) grey = 1.0;

			// Apply new colour
			colour.r = blend.r*grey;
			colour.g = blend.g*grey;
			colour.b = blend.b*grey;
			colour.index = pal->nearestColour(colour);
		}

		// Tinted range
		else if (r->getType() == TRANS_TINT)
		{
			TransRangeTint* tt = (TransRangeTint*)r;

			// Get colours
			rgba_t tint = tt->getColour();

			// Colourise
			float amount = tt->getAmount() * 0.01f;
			float inv_amt = 1.0f - amount;

			// Apply new colour
			colour.r = col.r*inv_amt + tint.r*amount;;
			colour.g = col.g*inv_amt + tint.g*amount;
			colour.b = col.b*inv_amt + tint.b*amount;
			colour.index = pal->nearestColour(colour);
		}

		// Special range
		else if (r->getType() == TRANS_SPECIAL)
		{
			TransRangeSpecial* ts = (TransRangeSpecial*)translations[a];
			string spec = ts->getSpecial();
			uint8_t type = BLEND_INVALID;
			if (S_CMPNOCASE(spec, "ice"))
				type = SpecialBlend::BLEND_ICE;
			else if (S_CMPNOCASE(spec, "inverse"))
				type = SpecialBlend::BLEND_INVERSE;
			else if (S_CMPNOCASE(spec, "red"))
				type = SpecialBlend::BLEND_RED;
			else if (S_CMPNOCASE(spec, "green"))
				type = SpecialBlend::BLEND_GREEN;
			else if (S_CMPNOCASE(spec, "blue"))
				type = SpecialBlend::BLEND_BLUE;
			else if (S_CMPNOCASE(spec, "gold"))
				type = SpecialBlend::BLEND_GOLD;
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

/* Translation::specialBlend
 * Apply one of the special colour blending modes from ZDoom:
 * Desaturate, Ice, Inverse, Blue, Gold, Green, Red.
 *******************************************************************/
rgba_t Translation::specialBlend(rgba_t col, uint8_t type, Palette8bit * pal)
{
	// Abort just in case
	if (type == SpecialBlend::BLEND_INVALID)
		return col;

	rgba_t colour = col;

	// Get greyscale using ZDoom formula
	float grey = (col.r*77 + col.g*143 + col.b*37) / 256.f;

	// Ice is a special case as it uses a colour range derived
	// from the Hexen palette instead of a linear gradient.
	if (type == BLEND_ICE)
	{
		// Determine destination palette index in IceRange
		uint8_t di = MIN(((int)grey >> 4), 15);
		rgba_t c = IceRange[di];
		colour.r = c.r;
		colour.g = c.g;
		colour.b = c.b;
		colour.a = c.a;
		colour.index = pal->nearestColour(colour);
	}
	// Desaturated blending goes from no effect to nearly fully desaturated
	else if (type >= BLEND_DESAT_FIRST && type <= BLEND_DESAT_LAST)
	{
		float amount = type - 1; // get value between 0 and 30

		colour.r = MIN(255, int((colour.r * (31 - amount) + grey * amount) / 31));
		colour.g = MIN(255, int((colour.g * (31 - amount) + grey * amount) / 31));
		colour.b = MIN(255, int((colour.b * (31 - amount) + grey * amount) / 31));
		colour.index = pal->nearestColour(colour);
	}
	// All others are essentially preset desaturated translations
	else
	{
		float sr, sg, sb, er, eg, eb;		// start and end ranges
		sr = sg = sb = er = eg = eb = 0.0;	// let's init them to 0.

		switch (type)
		{
		case BLEND_INVERSE:
			// inverted grayscale: Doom invulnerability, Strife sigil
			// start white, ends black
			sr = sg = sb = 1.0;
			break;
		case BLEND_GOLD:
			// Heretic invulnerability
			// starts black, ends reddish yellow
			er = 1.5; eg = 0.75;
			break;
		case BLEND_RED:
			// Skulltag doomsphere
			// starts black, ends red
			er = 1.5;
			break;
		case BLEND_GREEN:
			// Skulltag guardsphere
			// starts black, ends greenish-white
			er = 1.25; eg = 1.5; eb = 1.0;
			break;
		case BLEND_BLUE:
			// Hacx invulnerability
			// starts black, ends blue
			eb = 1.5;
		}
		// Apply new colour
		colour.r = MIN(255, int(sr + grey*(er - sr)));
		colour.g = MIN(255, int(sg + grey*(eg - sg)));
		colour.b = MIN(255, int(sb + grey*(eb - sb)));
		colour.index = pal->nearestColour(colour);
	}
	return colour;
}

/* Translation::addRange
 * Adds a new translation range of [type] at [pos] in the list
 *******************************************************************/
void Translation::addRange(int type, int pos)
{
	TransRange* tr = NULL;

	// Create range
	switch (type)
	{
	case TRANS_COLOUR:
		tr = new TransRangeColour();
		break;
	case TRANS_DESAT:
		tr = new TransRangeDesat();
		break;
	case TRANS_BLEND:
		tr = new TransRangeBlend();
		break;
	case TRANS_TINT:
		tr = new TransRangeTint();
		break;
	case TRANS_SPECIAL:
		tr = new TransRangeSpecial();
	default:
		tr = new TransRangePalette();
		break;
	};

	// Add to list
	if (pos < 0 || pos >= (int)translations.size())
		translations.push_back(tr);
	else
		translations.insert(translations.begin() + pos, tr);
}

/* Translation::removeRange
 * Removes the translation range at [pos]
 *******************************************************************/
void Translation::removeRange(int pos)
{
	// Check position
	if (pos < 0 || pos >= (int)translations.size())
		return;

	// Remove it
	delete translations[pos];
	translations.erase(translations.begin() + pos);
}

/* Translation::swapRanges
 * Swaps the translation range at [pos1] with the one at [pos2]
 *******************************************************************/
void Translation::swapRanges(int pos1, int pos2)
{
	// Check positions
	if (pos1 < 0 || pos2 < 0 || pos1 >= (int)translations.size() || pos2 >= (int)translations.size())
		return;

	// Swap them
	TransRange* temp = translations[pos1];
	translations[pos1] = translations[pos2];
	translations[pos2] = temp;
}
