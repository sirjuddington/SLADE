
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
	// Open definition string for processing w/tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters("[]:%,=#@");
	tz.openString(def);

	//wxLogMessage("Parse translation \"%s\"", def);
	
	// Test for ZDoom built-in translation
	string test = tz.peekToken().Lower();
	if (test == "inverse")
	{
		// TODO: Inverse translation
		built_in_name = "Inverse";
		return;
	}
	else if (test == "gold")
	{
		// TODO: Gold translation
		built_in_name = "Gold";
		return;
	}
	else if (test == "red")
	{
		// TODO: Red translation
		built_in_name = "Red";
		return;
	}
	else if (test == "green")
	{
		// TODO: Green translation
		built_in_name = "Green";
		return;
	}
	else if (test == "ice")
	{
		// TODO: Ice translation
		built_in_name = "Ice";
		return;
	}
	else if (test == "desaturate")
	{
		// TODO: Desaturate translation
		built_in_name = "Desaturate";
		return;
	}

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
		TransRangeColourise* tr = new TransRangeColourise();
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
 * the same length, so theindex value is only ever increased by 1. 
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
		if (copy.translations[a]->type == TRANS_COLOURISE)
			translations.push_back(new TransRangeColourise((TransRangeColourise*)copy.translations[a]));
		if (copy.translations[a]->type == TRANS_TINT)
			translations.push_back(new TransRangeTint((TransRangeTint*)copy.translations[a]));
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
	case TRANS_COLOURISE:
		tr = new TransRangeColourise();
		break;
	case TRANS_TINT:
		tr = new TransRangeTint();
		break;
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
