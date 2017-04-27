/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    BinaryControlLump.cpp
 * Description: Various classes used to represent data objects from
 *				Boom's ANIMATED lumps.
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
#include "General/Misc.h"
#include "AnimatedList.h"
#include "UI/Lists/ListView.h"
#include "Utility/Tokenizer.h"

/*******************************************************************
 * ANIMATEDENTRY CLASS FUNCTIONS
 *******************************************************************/

/* AnimatedEntry::AnimatedEntry
 * AnimatedEntry class constructor
 *******************************************************************/
AnimatedEntry::AnimatedEntry(animated_t entry)
{
	// Init variables
	this->first = wxString::From8BitData(entry.first, 8);
	this->last = wxString::From8BitData(entry.last, 8);
	this->type = entry.type & ANIM_MASK;
	this->speed = wxINT32_SWAP_ON_BE(entry.speed);
	this->decals = !!(entry.type & ANIM_DECALS);
	this->status = LV_STATUS_NORMAL;
}

/* AnimatedEntry::~AnimatedEntry
 * AnimatedEntry class destructor
 *******************************************************************/
AnimatedEntry::~AnimatedEntry()
{
}


/*******************************************************************
 * ANIMATEDLIST CLASS FUNCTIONS
 *******************************************************************/

/* AnimatedList::AnimatedList
 * AnimatedList class constructor
 *******************************************************************/
AnimatedList::AnimatedList()
{
}

/* AnimatedList::~AnimatedList
 * AnimatedList class destructor
 *******************************************************************/
AnimatedList::~AnimatedList()
{
	clear();
}

/* AnimatedList::getEntry
 * Returns the AnimatedEntry at [index], or NULL if [index] is out of
 * range
 *******************************************************************/
AnimatedEntry* AnimatedList::getEntry(size_t index)
{
	// Check index range
	if (index > nEntries())
		return NULL;

	return entries[index];
}

/* AnimatedList::getEntry
 * Returns an AnimatedEntry matching [name], or NULL if no match
 * found; looks for name at both the first and the last frames
 *******************************************************************/
AnimatedEntry* AnimatedList::getEntry(string name)
{
	for (size_t a = 0; a < nEntries(); a++)
	{
		if (entries[a]->getFirst().CmpNoCase(name) == 0 ||
		        entries[a]->getLast().CmpNoCase(name) == 0)
			return entries[a];
	}

	// No match found
	return NULL;
}

/* AnimatedList::clear
 * Clears all entries
 *******************************************************************/
void AnimatedList::clear()
{
	for (size_t a = 0; a < entries.size(); a++)
		delete entries[a];
	entries.clear();

}

/* AnimatedList::readANIMATEDData
 * Reads in a Boom-format ANIMATED entry. Returns true on success,
 * false otherwise
 *******************************************************************/
bool AnimatedList::readANIMATEDData(ArchiveEntry* animated)
{
	// Check entries were actually given
	if (!animated || animated->getSize() == 0)
		return false;

	uint8_t* data = new uint8_t[animated->getSize()];
	memcpy(data, animated->getData(), animated->getSize());
	uint8_t* cursor = data;
	uint8_t* eodata = cursor + animated->getSize();
	animated_t* type;

	while (cursor < eodata && *cursor != ANIM_STOP)
	{
		// reads an entry
		if (cursor + sizeof(animated_t) > eodata)
		{
			LOG_MESSAGE(1, "Error: ANIMATED entry is corrupt");
			delete[] data;
			return false;
		}
		type = (animated_t*) cursor;
		AnimatedEntry* entry = new AnimatedEntry(*type);
		cursor += sizeof(animated_t);

		// Add texture to list
		entries.push_back(entry);
	}
	delete[] data;
	return true;
}

/* AnimatedList::addEntry
 * Adds an entry at the given position
 *******************************************************************/
bool AnimatedList::addEntry(AnimatedEntry* entry, size_t pos)
{
	if (entry == NULL)
		return false;
	if (pos >= nEntries())
	{
		entries.push_back(entry);
	}
	else
	{
		vector<AnimatedEntry*>::iterator it = entries.begin() + pos;
		entries.insert(it, entry);
	}
	return true;
}

/* AnimatedList::removeEntry
 * Removes the entry at the given position
 *******************************************************************/
bool AnimatedList::removeEntry(size_t pos)
{
	if (pos >= nEntries())
	{
		entries.pop_back();
	}
	else
	{
		vector<AnimatedEntry*>::iterator it = entries.begin() + pos;
		entries.erase(it);
	}
	return true;
}

/* AnimatedList::swapEntries
 * Swaps the entries at the given positions
 *******************************************************************/
bool AnimatedList::swapEntries(size_t pos1, size_t pos2)
{
	if (pos1 >= nEntries())
	{
		pos1 = nEntries() - 1;
	}
	if (pos2 >= nEntries())
	{
		pos2 = nEntries() - 1;
	}
	if (pos1 == pos2)
	{
		return false;
	}
	AnimatedEntry* swap = entries[pos1];
	entries[pos1] = entries[pos2];
	entries[pos2] = swap;
	return true;
}

/* AnimatedList::convertAnimated
 * Converts ANIMATED data in [entry] to ANIMDEFS format, written to
 * [animdata]
 *******************************************************************/
bool AnimatedList::convertAnimated(ArchiveEntry* entry, MemChunk* animdata, bool animdefs)
{
	const uint8_t* cursor = entry->getData(true);
	const uint8_t* eodata = cursor + entry->getSize();
	const animated_t* animation;
	string conversion;
	int lasttype = -1;

	while (cursor < eodata && *cursor != ANIM_STOP)
	{
		// reads an entry
		if (cursor + sizeof(animated_t) > eodata)
		{
			LOG_MESSAGE(1, "Error: ANIMATED entry is corrupt");
			return false;
		}
		animation = (animated_t*) cursor;
		cursor += sizeof(animated_t);

		// Create animation string
		if (animdefs)
		{
			conversion = S_FMT("%s\tOptional\t%-8s\tRange\t%-8s\tTics %i%s",
							   (animation->type ? "Texture" : "Flat"),
							   animation->first, animation->last, animation->speed,
							   (animation->type == ANIM_DECALS ? " AllowDecals\n" : "\n"));
		}
		else
		{
			if ((animation->type > 1 ? 1 : animation->type) != lasttype)
			{
				conversion = S_FMT("#animated %s, spd is number of frames between changes\n"
					"[%s]\n#spd    last        first\n",
					animation->type ? "textures" : "flats", animation->type ? "TEXTURES" : "FLATS");
				lasttype = animation->type;
				if (lasttype > 1) lasttype = 1;
				animdata->reSize(animdata->getSize() + conversion.length(), true);
				animdata->write(conversion.To8BitData().data(), conversion.length());
			}
			conversion = S_FMT("%-8d%-12s%-12s\n", animation->speed, animation->last, animation->first);


		}

		// Write string to animdata
		animdata->reSize(animdata->getSize() + conversion.length(), true);
		animdata->write(conversion.To8BitData().data(), conversion.length());
	}
	return true;
}

/* AnimatedList::convertSwanTbls
 * Converts SWANTBLS data in [entry] to binary format, written to
 * [animdata]
 *******************************************************************/
bool AnimatedList::convertSwanTbls(ArchiveEntry* entry, MemChunk* animdata)
{
	Tokenizer tz(HCOMMENTS);
	tz.openMem(&(entry->getMCData()), entry->getName());

	string token;
	char buffer[23];
	while ((token = tz.getToken()).length())
	{
		// We should only treat animated flats and textures, and ignore switches
		if (S_CMP(token, "[FLATS]") || S_CMP(token, "[TEXTURES]"))
		{
			bool texture = S_CMP(token, "[TEXTURES]");
			do
			{
				int speed = tz.getInteger();
				string last = tz.getToken();
				string first= tz.getToken();
				if (last.length() > 8)
				{
					LOG_MESSAGE(1, "Error: string %s is too long for an animated %s name!",
									last, (texture ? "texture" : "flat"));
					return false;
				}
				if (first.length() > 8)
				{
					LOG_MESSAGE(1, "Error: string %s is too long for an animated %s name!",
									first, (texture ? "texture" : "flat"));
					return false;
				}

				// reset buffer
				int limit;
				memset(buffer, 0, 23);

				// Write animation type
				buffer[0] = texture;

				// Write last texture name
				limit = MIN(8, last.length());
				for (int a = 0; a < limit; ++a)
					buffer[1+a] = last[a];

				// Write first texture name
				limit = MIN(8, first.length());
				for (int a = 0; a < limit; ++a)
					buffer[10+a] = first[a];

				// Write animation duration
				buffer[19] = (uint8_t) (speed % 256);
				buffer[20] = (uint8_t) (speed>>8) % 256;
				buffer[21] = (uint8_t) (speed>>16) % 256;
				buffer[22] = (uint8_t) (speed>>24) % 256;

				// Save buffer to MemChunk
				if (!animdata->reSize(animdata->getSize() + 23, true))
					return false;
				if (!animdata->write(buffer, 23))
					return false;

				// Look for possible end of loop
				token = tz.peekToken();
			} while (token.length() && token[0] != '[');
		}
	}
	return true;
	// Note that we do not terminate the list here!
}