
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    CTexture.cpp
 * Description: C(omposite)Texture class, represents a composite
 *              texture as described in TEXTUREx entries
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
#include "CTexture.h"
#include "Archive/ArchiveManager.h"
#include "General/ResourceManager.h"
#include "General/Misc.h"
#include "Graphics/SImage/SImage.h"
#include "TextureXList.h"


/*******************************************************************
 * CTPATCH CLASS FUNCTIONS
 *******************************************************************/

/* CTPatch::CTPatch
 * CTPatch class default constructor
 *******************************************************************/
CTPatch::CTPatch()
{
	this->offset_x = 0;
	this->offset_y = 0;
}

/* CTPatch::CTPatch
 * CTPatch class constructor w/initial values
 *******************************************************************/
CTPatch::CTPatch(string name, int16_t offset_x, int16_t offset_y)
{
	this->name = name;
	this->offset_x = offset_x;
	this->offset_y = offset_y;
}

/* CTPatch::CTPatch
 * CTPatch class constructor copying another CTPatch
 *******************************************************************/
CTPatch::CTPatch(CTPatch* copy)
{
	if (copy)
	{
		name = copy->name;
		offset_x = copy->offset_x;
		offset_y = copy->offset_y;
	}
}

/* CTPatch::~CTPatch
 * CTPatch class destructor
 *******************************************************************/
CTPatch::~CTPatch()
{
}

/* CTPatch::getPatchEntry
 * Returns the entry (if any) associated with this patch via the
 * resource manager. Entries in [parent] will be prioritised over
 * entries in any other open archive
 *******************************************************************/
ArchiveEntry* CTPatch::getPatchEntry(Archive* parent)
{
	// Default patches should be in patches namespace
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "patches", parent);

	// Not found in patches, check in graphics namespace
	if (!entry) entry = theResourceManager->getPatchEntry(name, "graphics", parent);

	// Not found in patches, check in stand-alone texture namespace
	if (!entry) entry = theResourceManager->getPatchEntry(name, "textures", parent);

	return entry;
}


/*******************************************************************
 * CTPATCHEX CLASS FUNCTIONS
 *******************************************************************/

/* CTPatchEx::CTPatchEx
 * CTPatchEx class default constructor
 *******************************************************************/
CTPatchEx::CTPatchEx()
{
	flip_x = false;
	flip_y = false;
	use_offsets = false;
	rotation = 0;
	alpha = 1.0f;
	style = "Copy";
	blendtype = 0;
	type = PTYPE_PATCH;
}

/* CTPatchEx::CTPatchEx
 * CTPatchEx class constructor w/basic initial values
 *******************************************************************/
CTPatchEx::CTPatchEx(string name, int16_t offset_x, int16_t offset_y, uint8_t type)
	: CTPatch(name, offset_x, offset_y)
{
	flip_x = false;
	flip_y = false;
	use_offsets = false;
	rotation = 0;
	alpha = 1.0f;
	style = "Copy";
	blendtype = 0;
	this->type = type;
}

/* CTPatchEx::CTPatchEx
 * CTPatchEx class constructor copying a regular CTPatch
 *******************************************************************/
CTPatchEx::CTPatchEx(CTPatch* copy)
{
	if (copy)
	{
		flip_x = false;
		flip_y = false;
		use_offsets = false;
		rotation = 0;
		alpha = 1.0f;
		style = "Copy";
		blendtype = 0;
		offset_x = copy->xOffset();
		offset_y = copy->yOffset();
		name = copy->getName();
		type = PTYPE_PATCH;
	}
}

/* CTPatchEx::CTPatchEx
 * CTPatchEx class constructor copying another CTPatchEx
 *******************************************************************/
CTPatchEx::CTPatchEx(CTPatchEx* copy)
{
	if (copy)
	{
		flip_x = copy->flip_x;
		flip_y = copy->flip_y;
		use_offsets = copy->useOffsets();
		rotation = copy->rotation;
		alpha = copy->alpha;
		style = copy->style;
		blendtype = copy->blendtype;
		colour = copy->colour;
		offset_x = copy->offset_x;
		offset_y = copy->offset_y;
		name = copy->name;
		type = copy->type;
		translation.copy(copy->translation);
	}
}

/* CTPatchEx::~CTPatchEx
 * CTPatchEx class destructor
 *******************************************************************/
CTPatchEx::~CTPatchEx()
{
}

/* CTPatchEx::getPatchEntry
 * Returns the entry (if any) associated with this patch via the
 * resource manager. Entries in [parent] will be prioritised over
 * entries in any other open archive
 *******************************************************************/
ArchiveEntry* CTPatchEx::getPatchEntry(Archive* parent)
{
	// 'Patch' type: patches > graphics
	if (type == PTYPE_PATCH)
	{
		ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "patches", parent);
		if (!entry) entry = theResourceManager->getFlatEntry(name, parent);
		if (!entry) entry = theResourceManager->getPatchEntry(name, "graphics", parent);
		return entry;
	}

	// 'Graphic' type: graphics > patches
	if (type == PTYPE_GRAPHIC)
	{
		ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "graphics", parent);
		if (!entry) entry = theResourceManager->getPatchEntry(name, "patches", parent);
		if (!entry) entry = theResourceManager->getFlatEntry(name, parent);
		return entry;
	}
	// Silence warnings
	return NULL;
}

/* CTPatchEx::parse
 * Parses a ZDoom TEXTURES format patch definition
 *******************************************************************/
bool CTPatchEx::parse(Tokenizer& tz, uint8_t type)
{
	// Read basic info
	this->type = type;
	name = tz.getToken().Upper();
	tz.skipToken();	// Skip ,
	offset_x = tz.getInteger();
	tz.skipToken();	// Skip ,
	offset_y = tz.getInteger();

	// Check if there is any extended info
	if (tz.peekToken() == "{")
	{
		// Skip {
		tz.skipToken();

		// Parse extended info
		string property = tz.getToken();
		while (property != "}")
		{
			// FlipX
			if (S_CMPNOCASE(property, "FlipX"))
				flip_x = true;

			// FlipY
			if (S_CMPNOCASE(property, "FlipY"))
				flip_y = true;

			// UseOffsets
			if (S_CMPNOCASE(property, "UseOffsets"))
				use_offsets = true;

			// Rotate
			if (S_CMPNOCASE(property, "Rotate"))
				rotation = tz.getInteger();

			// Translation
			if (S_CMPNOCASE(property, "Translation"))
			{
				// Add first translation string
				translation.parse(tz.getToken());

				if (tz.peekToken() == "," && translation.builtInName() == "Desaturate")
				{
					// Desaturation, get amount
					tz.skipToken(); // Skip ,
					int amount = tz.getInteger();
					LOG_MESSAGE(2, "%d", amount);
					translation.setDesaturationAmount(amount);
				}
				else
				{
					// Add any subsequent translations (separated by commas)
					while (tz.peekToken() == ",")
					{
						tz.skipToken();	// Skip ,
						translation.parse(tz.getToken());
					}
				}

				blendtype = 1;
			}

			// Blend
			if (S_CMPNOCASE(property, "Blend"))
			{
				double val;
				wxColour col;
				blendtype = 2;

				// Read first value
				string first = tz.getToken();

				// If no second value, it's just a colour string
				if (tz.peekToken() != ",")
				{
					col.Set(first);
					colour.set(col.Red(), col.Green(), col.Blue());
				}
				else
				{
					// Second value could be alpha or green
					tz.skipToken();	// Skip ,
					double second = tz.getDouble();

					// If no third value, it's an alpha value
					if (tz.peekToken() != ",")
					{
						col.Set(first);
						colour.set(col.Red(), col.Green(), col.Blue(), second*255);
						blendtype = 3;
					}
					else
					{
						// Third value exists, must be R,G,B,A format
						// RGB are ints in the 0-255 range; A is float in the 0.0-1.0 range
						tz.skipToken();	// Skip ,
						first.ToDouble(&val);
						colour.r = val;
						colour.g = second;
						colour.b = tz.getInteger();
						if (tz.peekToken() != ",")
						{
							LOG_MESSAGE(1, "Invalid TEXTURES definition, expected ',', got '%s'", tz.getToken());
							return false;
						}
						tz.skipToken();	// Skip ,
						colour.a = tz.getDouble()*255;
						blendtype = 3;
					}
				}
			}

			// Alpha
			if (S_CMPNOCASE(property, "Alpha"))
				alpha = tz.getFloat();

			// Style
			if (S_CMPNOCASE(property, "Style"))
				style = tz.getToken();

			// Read next property name
			property = tz.getToken();
		}
	}

	return true;
}

/* CTPatchEx::asText
 * Returns a text representation of the patch in ZDoom TEXTURES
 * format
 *******************************************************************/
string CTPatchEx::asText()
{
	// Init text string
	string typestring = "Patch";
	if (type == PTYPE_GRAPHIC) typestring = "Graphic";
	string text = S_FMT("\t%s \"%s\", %d, %d\n", typestring, name, offset_x, offset_y);

	// Check if we need to write any extra properties
	if (!flip_x && !flip_y && !use_offsets && rotation == 0 && blendtype == 0 && alpha == 1.0f && S_CMPNOCASE(style, "Copy"))
		return text;
	else
		text += "\t{\n";

	// Write patch properties
	if (flip_x)
		text += "\t\tFlipX\n";
	if (flip_y)
		text += "\t\tFlipY\n";
	if (use_offsets)
		text += "\t\tUseOffsets\n";
	if (rotation != 0)
		text += S_FMT("\t\tRotate %d\n", rotation);
	if (blendtype == 1 && !translation.isEmpty())
	{
		text += "\t\tTranslation ";
		text += translation.asText();
		text += "\n";
	}
	if (blendtype >= 2)
	{
		wxColour col(colour.r, colour.g, colour.b);
		text += S_FMT("\t\tBlend \"%s\"", col.GetAsString(wxC2S_HTML_SYNTAX));

		if (blendtype == 3)
			text += S_FMT(", %1.1f\n", (double)colour.a / 255.0);
		else
			text += "\n";
	}
	if (alpha < 1.0f)
		text += S_FMT("\t\tAlpha %1.2f\n", alpha);
	if (!(S_CMPNOCASE(style, "Copy")))
		text += S_FMT("\t\tStyle %s\n", style);

	// Write ending
	text += "\t}\n";

	return text;
}


/*******************************************************************
 * CTEXTURE CLASS FUNCTIONS
 *******************************************************************/

/* CTexture::CTexture
 * CTexture class constructor
 *******************************************************************/
CTexture::CTexture(bool extended)
{
	this->width = 0;
	this->height = 0;
	this->def_width = 0;
	this->def_height = 0;
	this->name = "";
	this->scale_x = 1.0;
	this->scale_y = 1.0;
	this->world_panning = false;
	this->extended = extended;
	this->defined = false;
	this->optional = false;
	this->no_decals = false;
	this->null_texture = false;
	this->offset_x = 0;
	this->offset_y = 0;
	this->type = "Texture";
	this->state = 0;
	this->in_list = NULL;
	this->index = -1;
}

/* CTexture::~CTexture
 * CTexture class destructor
 *******************************************************************/
CTexture::~CTexture()
{
	for (unsigned a = 0; a < patches.size(); a++)
		delete patches[a];
}

/* CTexture::copyTexture
 * Copies the texture [tex] to this texture. If [keep_type] is true,
 * the current texture type (extended/regular) will be kept,
 * otherwise it will be converted to the type of [tex]
 *******************************************************************/
void CTexture::copyTexture(CTexture* tex, bool keep_type)
{
	// Check texture was given
	if (!tex)
		return;

	// Clear current texture
	clear();

	// Copy texture info
	this->name = tex->name;
	this->width = tex->width;
	this->height = tex->height;
	this->def_width = tex->def_width;
	this->def_height = tex->def_height;
	this->scale_x = tex->scale_x;
	this->scale_y = tex->scale_y;
	this->world_panning = tex->world_panning;
	if (!keep_type)
	{
		this->extended = tex->extended;
		this->defined = tex->defined;
	}
	this->optional = tex->optional;
	this->no_decals = tex->no_decals;
	this->null_texture = tex->null_texture;
	this->offset_x = tex->offset_x;
	this->offset_y = tex->offset_y;
	this->type = tex->type;

	// Update scaling
	if (extended)
	{
		if (scale_x == 0)
			scale_x = 1;
		if (scale_y == 0)
			scale_y = 1;
	}
	else if (!extended && tex->extended)
	{
		if (scale_x == 1)
			scale_x = 0;
		if (scale_y == 1)
			scale_y = 0;
	}

	// Copy patches
	for (unsigned a = 0; a < tex->nPatches(); a++)
	{
		CTPatch* patch = tex->getPatch(a);

		if (extended)
		{
			if (tex->extended)
				patches.push_back(new CTPatchEx((CTPatchEx*)patch));
			else
				patches.push_back(new CTPatchEx(patch));
		}
		else
			addPatch(patch->getName(), patch->xOffset(), patch->yOffset());
	}
}

/* CTexture::getPatch
 * Returns the patch at [index], or NULL if [index] is out of bounds
 *******************************************************************/
CTPatch* CTexture::getPatch(size_t index)
{
	// Check index
	if (index >= patches.size())
		return NULL;

	// Return patch at index
	return patches[index];
}

/* CTexture::getIndex
 * Returns the index of this texture within its parent list
 *******************************************************************/
int CTexture::getIndex()
{
	// Check if a parent TextureXList exists
	if (!in_list)
		return index;

	// Find this texture in the parent list
	return in_list->textureIndex(this->getName());
}

/* CTexture::clear
 * Clears all texture data
 *******************************************************************/
void CTexture::clear()
{
	this->name = "";
	this->width = 0;
	this->height = 0;
	this->def_width = 0;
	this->def_height = 0;
	this->scale_x = 1.0;
	this->scale_y = 1.0;
	this->defined = false;
	this->world_panning = false;
	this->optional = false;
	this->no_decals = false;
	this->null_texture = false;
	this->offset_x = 0;
	this->offset_y = 0;

	// Clear patches
	this->patches.clear();
	for (unsigned a = 0; a < patches.size(); a++)
		delete patches[a];
}

/* CTexture::addPatch
 * Adds a patch to the texture with the given attributes, at [index].
 * If [index] is -1, the patch is added to the end of the list.
 *******************************************************************/
bool CTexture::addPatch(string patch, int16_t offset_x, int16_t offset_y, int index)
{
	// Create new patch
	CTPatch* np;
	if (extended)
		np = new CTPatchEx(patch, offset_x, offset_y);
	else
		np = new CTPatch(patch, offset_x, offset_y);

	// Add it either after [index] or at the end
	if (index >= 0 && (unsigned) index < patches.size())
		patches.insert(patches.begin() + index, np);
	else
		patches.push_back(np);

	// Cannot be a simple define anymore
	this->defined = false;

	// Announce
	announce("patches_modified");

	return true;
}

/* CTexture::removePatch
 * Removes the patch at [index]. Returns false if [index] is invalid,
 * true otherwise
 *******************************************************************/
bool CTexture::removePatch(size_t index)
{
	// Check index
	if (index >= patches.size())
		return false;

	// Remove the patch
	delete patches[index];
	patches.erase(patches.begin() + index);

	// Cannot be a simple define anymore
	this->defined = false;

	// Announce
	announce("patches_modified");

	return true;
}

/* CTexture::removePatch
 * Removes all instances of [patch] from the texture. Returns true if
 * any were removed, false otherwise
 *******************************************************************/
bool CTexture::removePatch(string patch)
{
	// Go through patches
	bool removed = false;
	vector<CTPatch*>::iterator i = patches.begin();
	while (i != patches.end())
	{
		if (S_CMP((*i)->getName(), patch))
		{
			delete (*i);
			patches.erase(i);
			removed = true;
		}
		else
			i++;
	}

	// Cannot be a simple define anymore
	this->defined = false;

	if (removed)
		announce("patches_modified");

	return removed;
}

/* CTexture::replacePatch
 * Replaces the patch at [index] with [newpatch], and updates its
 * associated ArchiveEntry with [newentry]. Returns false if [index]
 * is out of bounds, true otherwise
 *******************************************************************/
bool CTexture::replacePatch(size_t index, string newpatch)
{
	// Check index
	if (index >= patches.size())
		return false;

	// Replace patch at [index] with new
	patches[index]->setName(newpatch);

	// Announce
	announce("patches_modified");

	return true;
}

/* CTexture::duplicatePatch
 * Duplicates the patch at [index], placing the duplicated patch
 * at [offset_x],[offset_y] from the original. Returns false if
 * [index] is out of bounds, true otherwise
 *******************************************************************/
bool CTexture::duplicatePatch(size_t index, int16_t offset_x, int16_t offset_y)
{
	// Check index
	if (index >= patches.size())
		return false;

	// Get patch info
	CTPatch* dp = patches[index];

	// Add duplicate patch
	if (extended)
		patches.insert(patches.begin() + index, new CTPatchEx((CTPatchEx*)patches[index]));
	else
		patches.insert(patches.begin() + index, new CTPatch(patches[index]));

	// Offset patch by given amount
	patches[index+1]->setOffsetX(dp->xOffset() + offset_x);
	patches[index+1]->setOffsetY(dp->yOffset() + offset_y);

	// Cannot be a simple define anymore
	this->defined = false;

	// Announce
	announce("patches_modified");

	return true;
}

/* CTexture::swapPatches
 * Swaps the patches at [p1] and [p2]. Returns false if either index
 * is invalid, true otherwise
 *******************************************************************/
bool CTexture::swapPatches(size_t p1, size_t p2)
{
	// Check patch indices are correct
	if (p1 >= patches.size() || p2 >= patches.size())
		return false;

	// Swap the patches
	CTPatch* temp = patches[p1];
	patches[p1] = patches[p2];
	patches[p2] = temp;

	// Announce
	announce("patches_modified");

	return true;
}

/* CTexture::parse
 * Parses a TEXTURES format texture definition
 *******************************************************************/
bool CTexture::parse(Tokenizer& tz, string type)
{
	// Check if optional
	if (S_CMPNOCASE(tz.peekToken(), "optional"))
	{
		tz.getToken();	// Skip it
		optional = true;
	}

	// Read basic info
	this->type = type;
	this->extended = true;
	this->defined = false;
	name = tz.getToken().Upper();
	tz.skipToken();	// Skip ,
	width = tz.getInteger();
	tz.skipToken();	// Skip ,
	height = tz.getInteger();

	// Check for extended info
	if (tz.peekToken() == "{")
	{
		tz.skipToken();	// Skip {

		// Read properties
		string property = tz.getToken();
		while (property != "}")
		{
			// Check if end of text is reached (error)
			if (property.IsEmpty())
			{
				LOG_MESSAGE(1, "Error parsing texture %s: End of text found, missing } perhaps?", name);
				return false;
			}

			// XScale
			if (S_CMPNOCASE(property, "XScale"))
				scale_x = tz.getFloat();

			// YScale
			if (S_CMPNOCASE(property, "YScale"))
				scale_y = tz.getFloat();

			// Offset
			if (S_CMPNOCASE(property, "Offset"))
			{
				offset_x = tz.getInteger();
				tz.skipToken();	// Skip ,
				offset_y = tz.getInteger();
			}

			// WorldPanning
			if (S_CMPNOCASE(property, "WorldPanning"))
				world_panning = true;

			// NoDecals
			if (S_CMPNOCASE(property, "NoDecals"))
				no_decals = true;

			// NullTexture
			if (S_CMPNOCASE(property, "NullTexture"))
				null_texture = true;

			// Patch
			if (S_CMPNOCASE(property, "Patch"))
			{
				CTPatchEx* patch = new CTPatchEx();
				patch->parse(tz);
				patches.push_back(patch);
			}

			// Graphic
			if (S_CMPNOCASE(property, "Graphic"))
			{
				CTPatchEx* patch = new CTPatchEx();
				patch->parse(tz, PTYPE_GRAPHIC);
				patches.push_back(patch);
			}

			// Read next property
			property = tz.getToken();
		}
	}

	return true;
}

/* CTexture::parseDefine
 * Parses a HIRESTEX define block
 *******************************************************************/
bool CTexture::parseDefine(Tokenizer& tz)
{
	this->type = "Define";
	this->extended = true;
	this->defined = true;
	name = tz.getToken().Upper();
	def_width = tz.getInteger();
	def_height = tz.getInteger();
	width = def_width;
	height = def_height;
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name);
	if (entry)
	{
		SImage image;
		if (image.open(entry->getMCData()))
		{
			width = image.getWidth();
			height = image.getHeight();
			scale_x = (double)width / (double)def_width;
			scale_y = (double)height / (double)def_height;
		}
	}
	CTPatchEx* patch = new CTPatchEx(name);
	patches.push_back(patch);
	return true;
}

/* CTexture::asText
 * Returns a string representation of the texture, in ZDoom TEXTURES
 * format
 *******************************************************************/
string CTexture::asText()
{
	// Can't write non-extended texture as text
	if (!extended)
		return "";

	// Define block
	if (defined)
		return S_FMT("define \"%s\" %d %d\n", name, def_width, def_height);

	// Init text string
	string text;
	if (optional)
		text = S_FMT("%s Optional \"%s\", %d, %d\n{\n", type, name, width, height);
	else
		text = S_FMT("%s \"%s\", %d, %d\n{\n", type, name, width, height);

	// Write texture properties
	if (scale_x != 1.0)
		text += S_FMT("\tXScale %1.3f\n", scale_x);
	if (scale_y != 1.0)
		text += S_FMT("\tYScale %1.3f\n", scale_y);
	if (offset_x != 0 || offset_y != 0)
		text += S_FMT("\tOffset %d, %d\n", offset_x, offset_y);
	if (world_panning)
		text += "\tWorldPanning\n";
	if (no_decals)
		text += "\tNoDecals\n";
	if (null_texture)
		text += "\tNullTexture\n";

	// Write patches
	for (unsigned a = 0; a < patches.size(); a++)
		text += ((CTPatchEx*)patches[a])->asText();

	// Write ending
	text += "}\n\n";

	return text;
}

/* CTexture::convertExtended
 * Converts the texture to 'extended' (ZDoom TEXTURES) format
 *******************************************************************/
bool CTexture::convertExtended()
{
	// Simple conversion system for defines
	if (defined)
		defined = false;

	// Don't convert if already extended
	if (extended)
		return true;

	// Convert scale if needed
	if (scale_x == 0) scale_x = 1;
	if (scale_y == 0) scale_y = 1;

	// Convert all patches over to extended format
	for (unsigned a = 0; a < patches.size(); a++)
	{
		CTPatchEx* expatch = new CTPatchEx(patches[a]);
		delete patches[a];
		patches[a] = expatch;
	}

	// Set extended flag
	extended = true;

	return true;
}

/* CTexture::convertRegular
 * Converts the texture to 'regular' (TEXTURE1/2) format
 *******************************************************************/
bool CTexture::convertRegular()
{
	// Don't convert if already regular
	if (!extended)
		return true;

	// Convert scale
	if (scale_x == 1)
		scale_x = 0;
	else
		scale_x *= 8;
	if (scale_y == 1)
		scale_y = 0;
	else
		scale_y *= 8;

	// Convert all patches over to normal format
	for (unsigned a = 0; a < patches.size(); a++)
	{
		CTPatch* npatch = new CTPatch(patches[a]->getName(), patches[a]->xOffset(), patches[a]->yOffset());
		delete patches[a];
		patches[a] = npatch;
	}

	// Unset extended flag
	extended = false;
	defined = false;

	return true;
}

/* CTexture::toImage
 * Generates a SImage representation of this texture, using patches
 * from [parent] primarily, and the palette [pal]
 *******************************************************************/
bool CTexture::toImage(SImage& image, Archive* parent, Palette8bit* pal, bool force_rgba)
{
	// Init image
	image.clear();
	image.resize(width, height);

	// Add patches
	SImage p_img(PALMASK);
	si_drawprops_t dp;
	dp.src_alpha = false;
	if (defined)
	{
		CTPatchEx* patch = (CTPatchEx*)patches[0];
		if (!loadPatchImage(0, p_img, parent, pal))
			return false;
		width = p_img.getWidth();
		height = p_img.getHeight();
		image.resize(width, height);
		scale_x = (double)width / (double)def_width;
		scale_y = (double)height / (double)def_height;
		image.drawImage(p_img, 0, 0, dp, pal, pal);
	}
	else if (extended)
	{
		// Extended texture

		// Add each patch to image
		for (unsigned a = 0; a < patches.size(); a++)
		{
			CTPatchEx* patch = (CTPatchEx*)patches[a];

			// Load patch entry
			if (!loadPatchImage(a, p_img, parent, pal))
				continue;

			// Handle offsets
			int ofs_x = patch->xOffset();
			int ofs_y = patch->yOffset();
			if (patch->useOffsets())
			{
				ofs_x -= p_img.offset().x;
				ofs_y -= p_img.offset().y;
			}

			// Apply translation before anything in case we're forcing rgba (can't translate rgba images)
			if (patch->getBlendType() == 1)
				p_img.applyTranslation(&(patch->getTranslation()), pal);

			// Convert to RGBA if forced
			if (force_rgba)
				p_img.convertRGBA(pal);

			// Flip/rotate if needed
			if (patch->flipX())
				p_img.mirror(false);
			if (patch->flipY())
				p_img.mirror(true);
			if (patch->getRotation() != 0)
				p_img.rotate(patch->getRotation());

			// Setup transparency blending
			dp.blend = NORMAL;
			dp.alpha = 1.0f;
			dp.src_alpha = false;
			if (patch->getStyle() == "CopyAlpha" || patch->getStyle() == "Overlay")
				dp.src_alpha = true;
			else if (patch->getStyle() == "Translucent" || patch->getStyle() == "CopyNewAlpha")
				dp.alpha = patch->getAlpha();
			else if (patch->getStyle() == "Add")
			{
				dp.blend = ADD;
				dp.alpha = patch->getAlpha();
			}
			else if (patch->getStyle() == "Subtract")
			{
				dp.blend = SUBTRACT;
				dp.alpha = patch->getAlpha();
			}
			else if (patch->getStyle() == "ReverseSubtract")
			{
				dp.blend = REVERSE_SUBTRACT;
				dp.alpha = patch->getAlpha();
			}
			else if (patch->getStyle() == "Modulate")
			{
				dp.blend = MODULATE;
				dp.alpha = patch->getAlpha();
			}

			// Setup patch colour
			if (patch->getBlendType() == 2)
				p_img.colourise(patch->getColour(), pal);
			else if (patch->getBlendType() == 3)
				p_img.tint(patch->getColour(), patch->getColour().fa(), pal);


			// Add patch to texture image
			image.drawImage(p_img, ofs_x, ofs_y, dp, pal, pal);
		}
	}
	else
	{
		// Normal texture

		// Add each patch to image
		for (unsigned a = 0; a < patches.size(); a++)
		{
			CTPatch* patch = patches[a];
			if (Misc::loadImageFromEntry(&p_img, patch->getPatchEntry(parent)))
				image.drawImage(p_img, patch->xOffset(), patch->yOffset(), dp, pal, pal);
		}
	}

	return true;
}

/* CTexture::loadPatchImage
 * Loads the image for the patch at [pindex] into [image]. Can deal
 * with textures-as-patches
 *******************************************************************/
bool CTexture::loadPatchImage(unsigned pindex, SImage& image, Archive* parent, Palette8bit* pal)
{
	// Check patch index
	if (pindex >= patches.size())
		return false;

	CTPatch* patch = patches[pindex];

	// If the texture is extended, search for textures-as-patches first
	// (as long as the patch name is different from this texture's name)
	if (extended && !(S_CMPNOCASE(patch->getName(), name)))
	{
		// Search the texture list we're in first
		if (in_list)
		{
			for (unsigned a = 0; a < in_list->nTextures(); a++)
			{
				CTexture* tex = in_list->getTexture(a);

				// Don't look past this texture in the list
				if (tex->getName() == name)
					break;

				// Check for name match
				if (S_CMPNOCASE(tex->getName(), patch->getName()))
				{
					// Load texture to image
					return tex->toImage(image, parent, pal);
				}
			}
		}

		// Otherwise, try the resource manager
		// TODO: Something has to be ignored here. The entire archive or just the current list?
		CTexture* tex = theResourceManager->getTexture(patch->getName(), parent);
		if (tex)
			return tex->toImage(image, parent, pal);
	}

	// Get patch entry
	ArchiveEntry* entry = patch->getPatchEntry(parent);

	// Load entry to image if valid
	if (entry)
		return Misc::loadImageFromEntry(&image, entry);
	else
		return false;
}
