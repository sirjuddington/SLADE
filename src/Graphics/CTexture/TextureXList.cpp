
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextureXList.cpp
// Description: Handles a collection of Composite Textures (ie, encapsulates a
//              TEXTUREx entry)
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
#include "TextureXList.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Structs
//
// -----------------------------------------------------------------------------
namespace
{
// Some structs for reading TEXTUREx data
struct TexDef
{
	// Just the data relevant to SLADE3
	char     name[8];
	uint16_t flags;
	uint8_t  scale[2];
	int16_t  width;
	int16_t  height;

	void cleanupName()
	{
		bool end = false;
		for (auto& c : name)
		{
			if (end)
				c = 0;
			else if (c == 0)
				end = true;
		}
	}
};

struct NamelessTexDef
{
	// The nameless version used by Doom Alpha 0.4
	uint16_t flags;
	uint8_t  scale[2];
	int16_t  width;
	int16_t  height;
	int16_t  columndir[2];
	int16_t  patchcount;
};

struct FullTexDef
{
	// The full version with some useless data
	char     name[8];
	uint16_t flags;
	uint8_t  scale[2];
	int16_t  width;
	int16_t  height;
	int16_t  columndir[2];
	int16_t  patchcount;
};

struct StrifeTexDef
{
	// The Strife version with less useless data
	char     name[8];
	uint16_t flags;
	uint8_t  scale[2];
	int16_t  width;
	int16_t  height;
	int16_t  patchcount;
};
} // namespace



// -----------------------------------------------------------------------------
//
// TextureXList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the texture at [index], or the 'invalid' texture if [index] is
// out of range
// -----------------------------------------------------------------------------
CTexture* TextureXList::texture(size_t index)
{
	// Check index
	if (index >= textures_.size())
		return &tex_invalid_;

	// Return texture at index
	return textures_[index].get();
}

// -----------------------------------------------------------------------------
// Returns the texture matching [name], or the 'invalid' texture if no match is
// found
// -----------------------------------------------------------------------------
CTexture* TextureXList::texture(string_view name)
{
	// Search for texture by name
	for (auto& texture : textures_)
	{
		if (strutil::equalCI(texture->name(), name))
			return texture.get();
	}

	// Not found
	return &tex_invalid_;
}

// -----------------------------------------------------------------------------
// Returns the index of the texture matching [name], or -1 if no match was found
// -----------------------------------------------------------------------------
int TextureXList::textureIndex(string_view name)
{
	// Search for texture by name
	for (unsigned a = 0; a < textures_.size(); a++)
	{
		if (strutil::equalCI(textures_[a]->name(), name))
		{
			textures_[a]->index_ = a;
			return a;
		}
	}

	// Not found
	return -1;
}

// -----------------------------------------------------------------------------
// Adds [tex] to the texture list at [position]
// -----------------------------------------------------------------------------
void TextureXList::addTexture(unique_ptr<CTexture> tex, int position)
{
	// Add it to the list at position if valid
	tex->in_list_ = this;
	if (position >= 0 && (unsigned)position < textures_.size())
	{
		tex->index_ = position;
		textures_.insert(textures_.begin() + position, std::move(tex));
	}
	else
	{
		tex->index_ = textures_.size();
		textures_.push_back(std::move(tex));
	}
}

// -----------------------------------------------------------------------------
// Removes the texture at [index] from the list and returns it
// -----------------------------------------------------------------------------
unique_ptr<CTexture> TextureXList::removeTexture(unsigned index)
{
	// Check index
	if (index >= textures_.size())
		return nullptr;

	// Remove the texture from the list
	auto removed = std::move(textures_[index]);
	textures_.erase(textures_.begin() + index);

	return removed;
}

// -----------------------------------------------------------------------------
// Swaps the texture at [index1] with the texture at [index2]
// -----------------------------------------------------------------------------
void TextureXList::swapTextures(unsigned index1, unsigned index2)
{
	// Check indices
	if (index1 >= textures_.size() || index2 >= textures_.size())
		return;

	// Swap them
	textures_[index1].swap(textures_[index2]);

	// Swap indices
	int ti                    = textures_[index1]->index_;
	textures_[index1]->index_ = textures_[index2]->index_;
	textures_[index2]->index_ = ti;
}

// -----------------------------------------------------------------------------
// Replaces the texture at [index] with [replacement].
// Returns the original texture that was replaced (or null if index was invalid)
// -----------------------------------------------------------------------------
unique_ptr<CTexture> TextureXList::replaceTexture(unsigned index, unique_ptr<CTexture> replacement)
{
	// Check index
	if (index >= textures_.size())
		return nullptr;

	// Replace texture
	auto replaced    = std::move(textures_[index]);
	textures_[index] = std::move(replacement);

	return replaced;
}

// -----------------------------------------------------------------------------
// Clears all textures
// -----------------------------------------------------------------------------
void TextureXList::clear(bool clear_patches)
{
	textures_.clear();
}

// -----------------------------------------------------------------------------
// Updates all textures in the list to 'remove' [patch]
// -----------------------------------------------------------------------------
void TextureXList::removePatch(string_view patch)
{
	// Go through all textures
	for (auto& texture : textures_)
		texture->removePatch(patch); // Remove patch from texture
}

// -----------------------------------------------------------------------------
// Reads in a doom-format TEXTUREx entry.
// Returns true on success, false otherwise
// -----------------------------------------------------------------------------
bool TextureXList::readTEXTUREXData(ArchiveEntry* texturex, const PatchTable& patch_table, bool add)
{
	// Check entries were actually given
	if (!texturex)
		return false;

	// Clear current textures if needed
	if (!add)
		clear();

	// Update palette
	maineditor::setGlobalPaletteFromArchive(texturex->parent());

	// Read TEXTUREx

	// Read header
	texturex->seek(0, SEEK_SET);

	// Number of textures
	int32_t n_tex = 0;
	if (!texturex->read(&n_tex, 4))
	{
		log::error("TEXTUREx entry is corrupt (can't read texture count)");
		return false;
	}
	n_tex = wxINT32_SWAP_ON_BE(n_tex);

	// If it's an empty TEXTUREx entry, stop here
	if (n_tex == 0)
		return true;

	// Texture definition offsets
	vector<int32_t> offsets(n_tex);
	if (!texturex->read(offsets.data(), n_tex * 4))
	{
		log::error("TEXTUREx entry is corrupt (can't read first offset)");
		return false;
	}

	// Read the first texture definition to try to identify the format
	if (!texturex->seek(wxINT32_SWAP_ON_BE(offsets[0]), SEEK_SET))
	{
		log::error("TEXTUREx entry is corrupt (can't read first definition)");
		return false;
	}
	// Look at the name field. Is it present or not?
	char tempname[8];
	if (!texturex->read(&tempname, 8))
	{
		log::error("TEXTUREx entry is corrupt (can't read first name)");
		return false;
	}
	// Let's pretend it is and see what happens.
	txformat_ = Format::Normal;

	// Only the characters A-Z (uppercase), 0-9, and [ ] - _ should be used in texture names.
	for (uint8_t a = 0; a < 8; ++a)
	{
		if (a > 0 && tempname[a] == 0)
			// We found a null-terminator for the string, so we can assume it's okay.
			break;
		if (tempname[a] >= 'a' && tempname[a] <= 'z')
		{
			txformat_ = Format::Jaguar;
			// log::info(1, "Jaguar texture");
			break;
		}
		else if (!((tempname[a] >= 'A' && tempname[a] <= '[') || (tempname[a] >= '0' && tempname[a] <= '9')
				   || tempname[a] == ']' || tempname[a] == '-' || tempname[a] == '_'))
		// We're out of character range, so this is probably not a texture name.
		{
			txformat_ = Format::Nameless;
			// log::info(1, "Nameless texture");
			break;
		}
	}

	// Now let's see if it is the abridged Strife format or not.
	if (txformat_ == Format::Normal)
	{
		// No need to test this again since it was already tested before.
		texturex->seek(offsets[0], SEEK_SET);
		FullTexDef temp;
		if (!texturex->read(&temp, 22))
		{
			log::error("TEXTUREx entry is corrupt (can't test definition)");
			return false;
		}
		// Test condition adapted from ZDoom; apparently the first two bytes of columndir
		// may be set to garbage values by some editors and are therefore unreliable.
		if (wxINT16_SWAP_ON_BE(temp.patchcount <= 0) || (temp.columndir[1] != 0))
			txformat_ = Format::Strife11;
	}

	// Read all texture definitions
	for (int32_t a = 0; a < n_tex; a++)
	{
		// Skip to texture definition
		if (!texturex->seek(offsets[a], SEEK_SET))
		{
			log::error("TEXTUREx entry is corrupt (can't find definition)");
			return false;
		}

		// Read definition
		TexDef tdef;
		if (txformat_ == Format::Nameless)
		{
			NamelessTexDef nameless;
			// Auto-naming mechanism taken from DeuTex
			if (a > 99999)
			{
				log::error("More than 100000 nameless textures");
				return false;
			}
			char temp[9] = "";
			sprintf(temp, "TEX%05d", a);
			memcpy(tdef.name, temp, 8);

			// Read texture info
			if (!texturex->read(&nameless, 8))
			{
				log::error("TEXTUREx entry is corrupt (can't read nameless definition #{})", a);
				return false;
			}

			// Copy data to permanent structure
			tdef.flags    = nameless.flags;
			tdef.scale[0] = nameless.scale[0];
			tdef.scale[1] = nameless.scale[1];
			tdef.width    = nameless.width;
			tdef.height   = nameless.height;
		}
		else if (!texturex->read(&tdef, 16))
		{
			log::error("TEXTUREx entry is corrupt, (can't read texture definition #{})", a);
			return false;
		}

		// Skip unused
		if (txformat_ != Format::Strife11)
		{
			if (!texturex->seek(4, SEEK_CUR))
			{
				log::error("TEXTUREx entry is corrupt (can't skip dummy data past #{})", a);
				return false;
			}
		}

		// Create texture
		tdef.cleanupName();
		auto tex      = std::make_unique<CTexture>();
		tex->name_    = strutil::viewFromChars(tdef.name, 8);
		tex->size_.x  = wxINT16_SWAP_ON_BE(tdef.width);
		tex->size_.y  = wxINT16_SWAP_ON_BE(tdef.height);
		tex->scale_.x = tdef.scale[0] / 8.0;
		tex->scale_.y = tdef.scale[1] / 8.0;

		// Set flags
		if (tdef.flags & Flags::WorldPanning)
			tex->world_panning_ = true;

		// Read patches
		int16_t n_patches = 0;
		if (!texturex->read(&n_patches, 2))
		{
			log::error("TEXTUREx entry is corrupt (can't read patchcount #{})", a);
			return false;
		}

		// log::info(1, "Texture #{}: {} patch%s", a, n_patches, n_patches == 1 ? "" : "es");

		for (uint16_t p = 0; p < n_patches; p++)
		{
			// Read patch definition
			Patch pdef;
			if (!texturex->read(&pdef, 6))
			{
				log::error("TEXTUREx entry is corrupt (can't read patch definition #{}:{})", a, p);
				log::error("Lump size {}, offset {}", texturex->size(), texturex->currentPos());
				return false;
			}

			// Skip unused
			if (txformat_ != Format::Strife11)
			{
				if (!texturex->seek(4, SEEK_CUR))
				{
					log::error("TEXTUREx entry is corrupt (can't skip dummy data past #{}:{})", a, p);
					return false;
				}
			}


			// Add it to the texture
			string patch;
			if (txformat_ == Format::Jaguar)
			{
				patch = strutil::upper(tex->name_);
			}
			else
			{
				patch = patch_table.patchName(pdef.patch);
			}
			if (patch.empty())
			{
				// log::info(1, "Warning: Texture %s contains patch %d which is invalid - may be incorrect PNAMES
				// entry", tex->getName(), pdef.patch);
				patch = fmt::format("INVPATCH{:04d}", pdef.patch);
			}

			tex->addPatch(patch, pdef.left, pdef.top);
		}

		// Add texture to list
		addTexture(std::move(tex));
	}

	return true;
}

#define SAFEFUNC(x) \
	if (!(x))       \
		return false;

// -----------------------------------------------------------------------------
// Writes the texture list in TEXTUREX format to [texturex], using [patch_table]
// for patch information.
// Returns true on success, false otherwise
// -----------------------------------------------------------------------------
bool TextureXList::writeTEXTUREXData(ArchiveEntry* texturex, const PatchTable& patch_table)
{
	// Check entry was given
	if (!texturex)
		return false;

	if (texturex->isLocked())
		return false;

	log::info("Writing " + textureXFormatString() + " format TEXTUREx entry");

	/* Total size of a TEXTUREx lump, in bytes:
		Header: 4 + (4 * numtextures)
		Textures:
			22 * numtextures (normal format)
			14 * numtextures (nameless format)
			18 * numtextures (Strife 1.1 format)
		Patches:
			10 * sum of patchcounts (normal and nameless formats)
			 6 * sum of patchcounts (Strife 1.1 format)
	*/
	size_t numpatchrefs = 0;
	size_t numtextures  = textures_.size();
	for (size_t i = 0; i < numtextures; ++i)
	{
		numpatchrefs += textures_[i]->nPatches();
	}
	log::info("{} patch references in {} textures", numpatchrefs, numtextures);

	size_t datasize   = 0;
	size_t headersize = 4 + (4 * numtextures);
	switch (txformat_)
	{
	case Format::Normal: datasize = 4 + (26 * numtextures) + (10 * numpatchrefs); break;
	case Format::Nameless: datasize = 4 + (18 * numtextures) + (10 * numpatchrefs); break;
	case Format::Strife11:
		datasize = 4 + (22 * numtextures) + (6 * numpatchrefs);
		break;
		// Some compilers insist on having default cases.
	default: return false;
	}

	MemChunk        txdata(datasize);
	vector<int32_t> offsets(numtextures);
	int32_t         foo = wxINT32_SWAP_ON_BE((signed)numtextures);

	// Write header
	txdata.seek(0, SEEK_SET);
	SAFEFUNC(txdata.write(&foo, 4));

	// Go to beginning of texture definitions
	SAFEFUNC(txdata.seek(4 + (numtextures * 4), SEEK_SET));

	// Write texture entries
	for (size_t i = 0; i < numtextures; ++i)
	{
		// Get texture to write
		auto tex = textures_[i].get();

		// Set offset
		offsets[i] = (signed)txdata.currentPos();

		// Write texture entry
		switch (txformat_)
		{
		case Format::Normal:
		{
			// Create 'normal' doom format texture definition
			FullTexDef txdef;
			memset(txdef.name, 0, 8); // Set texture name to all 0's (to ensure compatibility with XWE)
			strncpy(txdef.name, tex->name().data(), tex->name().size());
			for (auto& c : txdef.name)
				c = toupper(c);
			txdef.flags        = 0;
			txdef.scale[0]     = (tex->scaleX() * 8);
			txdef.scale[1]     = (tex->scaleY() * 8);
			txdef.width        = tex->width();
			txdef.height       = tex->height();
			txdef.columndir[0] = 0;
			txdef.columndir[1] = 0;
			txdef.patchcount   = tex->nPatches();

			// Check for WorldPanning flag
			if (tex->world_panning_)
				txdef.flags |= Flags::WorldPanning;

			// Write texture definition
			SAFEFUNC(txdata.write(&txdef, 22));

			break;
		}
		case Format::Nameless:
		{
			// Create nameless texture definition
			NamelessTexDef txdef;
			txdef.flags        = 0;
			txdef.scale[0]     = (tex->scaleX() * 8);
			txdef.scale[1]     = (tex->scaleY() * 8);
			txdef.width        = tex->width();
			txdef.height       = tex->height();
			txdef.columndir[0] = 0;
			txdef.columndir[1] = 0;
			txdef.patchcount   = tex->nPatches();

			// Write texture definition
			SAFEFUNC(txdata.write(&txdef, 8));

			break;
		}
		case Format::Strife11:
		{
			// Create strife format texture definition
			StrifeTexDef txdef;
			memset(txdef.name, 0, 8); // Set texture name to all 0's (to ensure compatibility with XWE)
			strncpy(txdef.name, tex->name().data(), tex->name().size());
			for (auto& c : txdef.name)
				c = toupper(c);
			txdef.flags      = 0;
			txdef.scale[0]   = (tex->scaleX() * 8);
			txdef.scale[1]   = (tex->scaleY() * 8);
			txdef.width      = tex->width();
			txdef.height     = tex->height();
			txdef.patchcount = tex->nPatches();

			// Check for WorldPanning flag
			if (tex->world_panning_)
				txdef.flags |= Flags::WorldPanning;

			// Write texture definition
			SAFEFUNC(txdata.write(&txdef, 18));

			break;
		}
		default: return false;
		}

		// Write patch references
		for (size_t k = 0; k < tex->nPatches(); ++k)
		{
			// Get patch to write
			auto patch = tex->patch(k);

			// Create patch definition
			Patch pdef;
			pdef.left = patch->xOffset();
			pdef.top  = patch->yOffset();

			// Check for 'invalid' patch
			if (strutil::startsWith(patch->name(), "INVPATCH"))
			{
				// Get raw patch index from name
				string_view number = patch->name();
				number.remove_prefix(8);
				pdef.patch = strutil::asInt(number);
			}
			else
				pdef.patch = patch_table.patchIndex(
					patch->name()); // Note this will be -1 if the patch doesn't exist in the patch table. This
									// should never happen with the texture editor, though.

			// Write common data
			SAFEFUNC(txdata.write(&pdef, 6));

			// In non-Strife formats, there's some added rubbish
			if (txformat_ != Format::Strife11)
			{
				foo = 0;
				SAFEFUNC(txdata.write(&foo, 4));
			}
		}
	}

	// Write offsets
	SAFEFUNC(txdata.seek(4, SEEK_SET));
	SAFEFUNC(txdata.write(offsets.data(), 4 * numtextures));

	// Write data to the TEXTUREx entry
	texturex->importMemChunk(txdata);

	// Update entry type
	EntryType::detectEntryType(*texturex);

	return true;
}

// -----------------------------------------------------------------------------
// Reads in a ZDoom-format TEXTURES entry.
// Returns true on success, false otherwise
// -----------------------------------------------------------------------------
bool TextureXList::readTEXTURESData(ArchiveEntry* textures)
{
	// Check for empty entry
	if (!textures)
	{
		global::error = "Attempt to read texture data from NULL entry";
		return false;
	}
	if (textures->size() == 0)
	{
		txformat_ = Format::Textures;
		return true;
	}

	// Get text to parse
	Tokenizer tz;
	tz.openMem(textures->data(), textures->name());

	// Parsing gogo
	while (!tz.atEnd())
	{
		// Texture definition
		if (tz.checkNC("Texture"))
		{
			auto tex = std::make_unique<CTexture>();
			if (tex->parse(tz, "Texture"))
				addTexture(std::move(tex));
		}

		// Sprite definition
		else if (tz.checkNC("Sprite"))
		{
			auto tex = std::make_unique<CTexture>();
			if (tex->parse(tz, "Sprite"))
				addTexture(std::move(tex));
		}

		// Graphic definition
		else if (tz.checkNC("Graphic"))
		{
			auto tex = std::make_unique<CTexture>();
			if (tex->parse(tz, "Graphic"))
				addTexture(std::move(tex));
		}

		// WallTexture definition
		else if (tz.checkNC("WallTexture"))
		{
			auto tex = std::make_unique<CTexture>();
			if (tex->parse(tz, "WallTexture"))
				addTexture(std::move(tex));
		}

		// Flat definition
		else if (tz.checkNC("Flat"))
		{
			auto tex = std::make_unique<CTexture>();
			if (tex->parse(tz, "Flat"))
				addTexture(std::move(tex));
		}

		// Old HIRESTEX "Define"
		else if (tz.checkNC("Define"))
		{
			auto tex = std::make_unique<CTexture>();
			if (tex->parseDefine(tz))
				addTexture(std::move(tex));
		}

		tz.adv();
	}

	txformat_ = Format::Textures;

	return true;
}

// -----------------------------------------------------------------------------
// Writes the texture list in TEXTURES format to [entry].
// Returns true on success, false otherwise
// -----------------------------------------------------------------------------
bool TextureXList::writeTEXTURESData(ArchiveEntry* textures)
{
	// Check format
	if (txformat_ != Format::Textures)
		return false;

	log::info("Writing ZDoom text format TEXTURES entry");

	// Generate a big string :P
	auto textures_data = "// Texture definitions generated by SLADE3\n// on " + wxNow().ToStdString() + "\n\n";
	for (auto& texture : textures_)
		textures_data += texture->asText();
	textures_data += "// End of texture definitions\n";

	log::info(
		"{} texture{} written on {} bytes", textures_.size(), textures_.size() < 2 ? "" : "s", textures_data.length());

	// Write it to the entry
	return textures->importMem(textures_data.data(), textures_data.size());
}

// -----------------------------------------------------------------------------
// Returns a string representation of the texture list format
// -----------------------------------------------------------------------------
string TextureXList::textureXFormatString() const
{
	switch (txformat_)
	{
	case Format::Normal: return "Doom TEXTUREx";
	case Format::Strife11: return "Strife TEXTUREx";
	case Format::Nameless: return "Nameless (Doom Alpha)";
	case Format::Textures: return "ZDoom TEXTURES";
	default: return "Unknown";
	}
}

// -----------------------------------------------------------------------------
// Converts all textures in the list to extended TEXTURES format
// -----------------------------------------------------------------------------
bool TextureXList::convertToTEXTURES()
{
	// Check format is appropriate
	if (txformat_ == Format::Textures)
	{
		global::error = "Already TEXTURES format";
		return false;
	}

	// Convert all textures to extended format
	for (auto& texture : textures_)
		texture->convertExtended();

	// First texture is null texture
	textures_[0]->null_texture_ = true;

	// Set new format
	txformat_ = Format::Textures;

	return true;
}

// -----------------------------------------------------------------------------
// Search for errors in texture list, return true if any are found
// -----------------------------------------------------------------------------
bool TextureXList::findErrors()
{
	bool ret = false;

	// Texture errors:
	// 1. A texture without any patch
	// 2. A texture with missing patches
	// 3. A texture with columns not covered by a patch

	for (unsigned a = 0; a < textures_.size(); a++)
	{
		if (textures_[a]->nPatches() == 0)
		{
			ret = true;
			log::warning("Texture {}: {} does not have any patch", a, textures_[a]->name());
		}
		else
		{
			vector<uint8_t> columns(textures_[a]->width());
			memset(columns.data(), 0, textures_[a]->width());
			for (size_t i = 0; i < textures_[a]->nPatches(); ++i)
			{
				auto patch = textures_[a]->patches_[i]->patchEntry();
				if (patch == nullptr)
				{
					ret = true;
					log::warning(
						"Texture {}: {}: patch {} cannot be found in any open archive",
						a,
						textures_[a]->name(),
						textures_[a]->patches_[i]->name());
					// Don't list missing columns when we don't know the size of the patch
					memset(columns.data(), 1, textures_[a]->width());
				}
				else
				{
					SImage img;
					img.open(patch->data());
					size_t start = std::max<size_t>(0, textures_[a]->patches_[i]->xOffset());
					size_t end   = std::min<size_t>(textures_[a]->width(), img.width() + start);
					for (size_t c = start; c < end; ++c)
						columns[c] = 1;
				}
			}
			for (size_t c = 0; c < textures_[a]->width(); ++c)
			{
				if (columns[c] == 0)
				{
					ret = true;
					log::warning("Texture {}: {}: column {} without a patch", a, textures_[a]->name(), c);
					break;
				}
			}
		}
	}
	return ret;
}

// -----------------------------------------------------------------------------
// Find and remove duplicates that exist in another texture list
// -----------------------------------------------------------------------------
bool TextureXList::removeDupesFoundIn(TextureXList& texture_list)
{
    vector<unsigned int> indicesToRemove;
    
    for (unsigned a = 0; a < textures_.size(); a++)
    {
        CTexture* thisTexture = textures_[a].get();
        int otherTextureIndex = texture_list.textureIndex(thisTexture->name());
        
        if (otherTextureIndex < 0)
        {
            // Other texture with this name not found
            log::info(wxString::Format("KEEP Texture: %s. It's NOT in the other list.", thisTexture->name()));
            continue;
        }
        
        CTexture* otherTexture = texture_list.texture(otherTextureIndex);
        
        // Compare the textures by simply checking if their asText values are identical
        // It may be slightly less fast to do it this way but it should be fairly future proof if more
        // things get added and it deals with textures being extended in one list and not extended in
        // the other list
        
        // Copy the textures over to a copy that is extended so asText works and we don't need to worry
        // about messing with original copies
        CTexture thisTextureCopy(true);
        CTexture otherTextureCopy(true);
        
        thisTextureCopy.copyTexture(*thisTexture, true);
        otherTextureCopy.copyTexture(*otherTexture, true);
        
        // Force a null texture because that value doesn't transfer from TEXTUREX defs
        if (a == 0 && otherTextureIndex == 0
            && (thisTexture->name() == "AASHITTY"
            || thisTexture->name() == "AASTINKY"
            || thisTexture->name() == "BADPATCH"
            || thisTexture->name() == "ABADONE"))
        {
            thisTextureCopy.setNullTexture(true);
            otherTextureCopy.setNullTexture(true);
        }
        
        string thisTextureText = thisTextureCopy.asText();
        string otherTextureText = otherTextureCopy.asText();
        
        if (thisTextureText == otherTextureText)
        {
            log::info(wxString::Format("DELETE Texture: %s. It's FOUND in the other list and IS identical.", thisTexture->name()));
            indicesToRemove.push_back(a);
        }
        else
        {
            log::info(wxString::Format("KEEP Texture: %s. It's FOUND in the other list but IS NOT identical.", thisTexture->name()));
        }
        
        log::info(thisTextureCopy.asText());
        log::info(otherTextureCopy.asText());
    }
    
    // Remove textures while going through the list back to front
    for (int a = indicesToRemove.size() - 1; a >= 0; a--)
    {
        removeTexture(indicesToRemove[a]);
    }
    
    return indicesToRemove.size();
}

// -----------------------------------------------------------------------------
// Remove texture entries from a zdoom format texture file that are redundant
// single patch textures with no special options
// Will also try to move patch entries to the textures namespace
// -----------------------------------------------------------------------------
bool TextureXList::cleanTEXTURESsinglePatch(Archive* currentArchive)
{
    // Check format is appropriate
    if (txformat_ != Format::Textures)
    {
        global::error = "Not TEXTURES format";
        return false;
    }
    
    std::map<ArchiveEntry*, unsigned int> patchEntryToSinglePatchTextures;
    std::set<ArchiveEntry*> patchEntriesToOmit;
    
    for (unsigned a = 0; a < textures_.size(); a++)
    {
        CTexture* texture = textures_[a].get();
        
        if (!texture->isExtended())
        {
            log::info(wxString::Format("KEEP Texture: %s. It's not extended.", texture->name()));
            continue;
        }
        
        // Check the number of patches
        if (texture->nPatches() != 1)
        {
            log::info(wxString::Format("KEEP Texture: %s. It has non-one number of patches.", texture->name()));
            continue;
        }
        
        // Check for any properties
        if (texture->scaleX() != 1.0
            || texture->scaleY() != 1.0
            || texture->offsetX() != 0
            || texture->offsetY() != 0
            || texture->worldPanning()
            || texture->isOptional()
            || texture->noDecals()
            || texture->nullTexture())
        {
            log::info(wxString::Format("KEEP Texture: %s. It has some special properties set.", texture->name()));
            continue;
        }
        
        // Check things about the single patch
        CTPatchEx* patch = dynamic_cast<CTPatchEx*>(texture->patch(0));
        
        // Check if the single patch is actually a patch in another archive
        ArchiveEntry* patchEntry = patch->patchEntry(nullptr);
        
        if (!patchEntry)
        {
            log::info(wxString::Format("KEEP Texture: %s. Its single patch %s failed to load.", texture->name(), patch->name()));
            continue;
        }
        
        if (patchEntry->parent() != currentArchive)
        {
            log::info(wxString::Format("KEEP Texture: %s. Its single patch is from a different archive.", texture->name()));
            continue;
        }
        
        // Check if this patch entry is used in another texture
        auto otherTextureIter = patchEntryToSinglePatchTextures.find(patchEntry);
        if (otherTextureIter != patchEntryToSinglePatchTextures.end())
        {
            log::info(wxString::Format("KEEP Textures: %s and %s. They are both using the same single patch %s.", texture->name(), textures_[otherTextureIter->second]->name(), patch->name()));
            patchEntriesToOmit.insert(patchEntry);
            continue;
        }
        
        // Check if the single patch is at 0,0 with no other special placement, and matches the texture size
        if (patch->xOffset() != 0
            || patch->yOffset() != 0)
        {
            log::info(wxString::Format("KEEP Texture: %s. Its single patch has non-zero offsets.", texture->name()));
            continue;
        }
        
        SImage img;
        img.open(patchEntry->data());
        
        // Check if the single patch size matches the texture size
        if (img.width() != texture->width()
            || img.height() != texture->height())
        {
            log::info(wxString::Format("KEEP Texture: %s. Its single patch has different dimensions from the texture.", texture->name()));
            continue;
        }
        
        // Check for any properties
        if (patch->flipX()
            || patch->flipY()
            || patch->useOffsets()
            || patch->rotation() != 0
            || patch->alpha() < 1.0f
            || !(strutil::equalCI(patch->style(), "Copy"))
            || patch->blendType() != CTPatchEx::BlendType::None)
        {
            log::info(wxString::Format("KEEP Texture: %s. Its single patch has some special properties set.", texture->name()));
            continue;
        }
        
        log::info(wxString::Format("MAYBE DELETE Texture: %s. It's a basic single patch texture.", texture->name()));
        patchEntryToSinglePatchTextures[patchEntry] = a;
    }
    
    // Remove all patchEntriesToOmit
    for (ArchiveEntry* patchEntryToOmit : patchEntriesToOmit)
    {
        patchEntryToSinglePatchTextures.erase(patchEntryToSinglePatchTextures.find(patchEntryToOmit));
    }
    
    patchEntriesToOmit.clear();
    
    // Now that it found all the single patch textures, make sure those patches aren't used in any other texture
    if (!patchEntryToSinglePatchTextures.size())
    {
        return false;
    }
    
    // Now load base resource archive textures into a single list
    TextureXList archiveTxList;
    
    Archive::SearchOptions opt;
    opt.match_type = EntryType::fromId("pnames");
    auto pnames = currentArchive->findLast(opt);
    
    // Load patch table
    PatchTable ptable;
    if (pnames)
    {
        ptable.loadPNAMES(pnames);
    
        // Load all Texturex entries
        Archive::SearchOptions texturexopt;
        texturexopt.match_type = EntryType::fromId("texturex");
        
        for (ArchiveEntry* texturexentry: currentArchive->findAll(texturexopt))
        {
            archiveTxList.readTEXTUREXData(texturexentry, ptable, true);
        }
    }
    
    // Load all zdtextures entries
    Archive::SearchOptions zdtexturesopt;
    zdtexturesopt.match_type = EntryType::fromId("zdtextures");
    
    for (ArchiveEntry* texturesentry: currentArchive->findAll(zdtexturesopt))
    {
        archiveTxList.readTEXTURESData(texturesentry);
    }
    
    // See if any other textures use any of the patch entries
    for (int a = 0; a < archiveTxList.textures_.size(); a++)
    {
        CTexture* texture = archiveTxList.textures_[a].get();
        
        for (int p = 0; p < texture->nPatches(); p++)
        {
            ArchiveEntry* patchEntry = texture->patches()[p]->patchEntry(nullptr);
            
            auto iter = patchEntryToSinglePatchTextures.find(patchEntry);
            
            // If we found a texture that isn't the texture the patch is associated to
            if (iter != patchEntryToSinglePatchTextures.end()
                && textures_[iter->second]->name() != texture->name())
            {
                log::info(wxString::Format("KEEP Textures: %s and %s. They are both using patch %s.", texture->name(), textures_[iter->second]->name(), texture->patches()[p]->name()));
                patchEntriesToOmit.insert(patchEntry);
                continue;
            }
        }
    }
    
    // Remove all patchEntriesToOmit
    for (ArchiveEntry* patchEntryToOmit : patchEntriesToOmit)
    {
        patchEntryToSinglePatchTextures.erase(patchEntryToSinglePatchTextures.find(patchEntryToOmit));
    }
    
    patchEntriesToOmit.clear();
    
    if (!patchEntryToSinglePatchTextures.size())
    {
        return false;
    }
    
    // Now remove the texture entries and convert the patches to textures themselves
    
    // Build a list of texture indices to remove so we remove it in back to front order
    vector<unsigned int> indicesToRemove;
    std::map<unsigned int, wxString> removalMessages;
    
    for (auto iter : patchEntryToSinglePatchTextures)
    {
        ArchiveEntry* patchEntry = iter.first;
        CTexture* texture = textures_[iter.second].get();
        
        indicesToRemove.push_back(iter.second);
        
        // Currently only supporting converting patch to texture in archives that support directories
        if(!currentArchive->formatDesc().supports_dirs)
        {
            removalMessages[iter.second] = wxString::Format("DELETE Texture: %s. Would Patch: %s to Texture File: %s but can't since archive doesn't support directories.", texture->name(), patchEntry->name(), texture->name());
        }
        else
        {
            string::size_type patchExtensionPos = patchEntry->name().find_last_of('.');
            string patchExtension = patchExtensionPos != string::npos
                ? patchEntry->name().substr(patchExtensionPos, patchEntry->name().size())
                : "";
            
            string textureFileName = texture->name();
            textureFileName.append(patchExtension);
            
            removalMessages[iter.second] = wxString::Format("DELETE Texture: %s. Convert Patch: %s to Texture File: %s.", texture->name(), patchEntry->name(), textureFileName);
            
            auto texturesDir = currentArchive->createDir("textures");
            patchEntry->rename(textureFileName);
            currentArchive->moveEntry(patchEntry, 0, texturesDir.get());
        }
    }
    
    std::sort(indicesToRemove.begin(), indicesToRemove.end());
    
    // Print the removal messages in original alphabetical texture order so it's easier to parse the output log
    for (int a = 0; a < indicesToRemove.size(); a++)
    {
        log::info(removalMessages[indicesToRemove[a]]);
    }
    
    // Remove textures while going through the list back to front
    for (int a = indicesToRemove.size() - 1; a >= 0; a--)
    {
        removeTexture(indicesToRemove[a]);
    }
    
    return true;
}
