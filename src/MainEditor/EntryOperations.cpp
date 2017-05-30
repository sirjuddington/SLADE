
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    EntryOperations.cpp
 * Description: Functions that perform specific operations on entries
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
#include "UI/WxStuff.h"
#include "EntryOperations.h"
#include "General/Misc.h"
#include "General/Console/Console.h"
#include "Archive/ArchiveManager.h"
#include "UI/TextureXEditor/TextureXEditor.h"
#include "Archive/EntryType/EntryDataFormat.h"
#include "Dialogs/ExtMessageDialog.h"
#include "MainEditor/MainEditor.h"
#include "Utility/FileMonitor.h"
#include "Archive/Formats/WadArchive.h"
#include "Dialogs/Preferences/PreferencesDialog.h"
#include "Dialogs/ModifyOffsetsDialog.h"
#include "UI/PaletteChooser.h"
#include "App.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(String, path_acc, "", CVAR_SAVE);
CVAR(String, path_acc_libs, "", CVAR_SAVE);
CVAR(String, path_pngout, "", CVAR_SAVE);
CVAR(String, path_pngcrush, "", CVAR_SAVE);
CVAR(String, path_deflopt, "", CVAR_SAVE);
CVAR(String, path_db2, "", CVAR_SAVE)
CVAR(Bool, acc_always_show_output, false, CVAR_SAVE);


/*******************************************************************
 * STRUCTS
 *******************************************************************/
// Define some png chunk structures
struct ihdr_t
{
	uint32_t id;
	uint32_t width;
	uint32_t height;
	uint8_t cinfo[5];
};

struct grab_chunk_t
{
	char name[4];
	int32_t xoff;
	int32_t yoff;
};

struct alph_chunk_t
{
	char name[4];
};

struct trns_chunk_t
{
	char name[4];
	uint8_t entries[256];
};

struct trans_chunk_t
{
	char name[5];
};

struct chunk_size_t
{
	uint32_t size;
};

/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

/* EntryOperations::gfxConvert
 * Converts the image [entry] to [target_format], using conversion
 * options specified in [opt] and converting to [target_colformat]
 * colour format if possible. Returns false if the conversion failed,
 * true otherwise
 *******************************************************************/
bool EntryOperations::gfxConvert(ArchiveEntry* entry, string target_format, SIFormat::convert_options_t opt, int target_colformat)
{
	// Init variables
	SImage image;

	// Get target image format
	SIFormat* fmt = SIFormat::getFormat(target_format);
	if (fmt == SIFormat::unknownFormat())
		return false;

	// Check format and target colour type are compatible
	if (target_colformat >= 0 && !fmt->canWriteType((SIType)target_colformat))
	{
		if (target_colformat == RGBA)
			LOG_MESSAGE(1, "Format \"%s\" cannot be written as RGBA data", fmt->getName());
		else if (target_colformat == PALMASK)
			LOG_MESSAGE(1, "Format \"%s\" cannot be written as paletted data", fmt->getName());

		return false;
	}

	// Load entry to image
	Misc::loadImageFromEntry(&image, entry);

	// Check if we can write the image to the target format
	int writable = fmt->canWrite(image);
	if (writable == SIFormat::NOTWRITABLE)
	{
		LOG_MESSAGE(1, "Entry \"%s\" could not be converted to target format \"%s\"", entry->getName(), fmt->getName());
		return false;
	}
	else if (writable == SIFormat::CONVERTIBLE)
		fmt->convertWritable(image, opt);

	// Now we apply the target colour format (if any)
	if (target_colformat == PALMASK)
		image.convertPaletted(opt.pal_target, opt.pal_current);
	else if (target_colformat == RGBA)
		image.convertRGBA(opt.pal_current);

	// Finally, write new image data back to the entry
	fmt->saveImage(image, entry->getMCData(), opt.pal_target);

	return true;
}

/* EntryOperations::modifyGfxOffsets
 * Changes the offsets of the given gfx entry, based on settings
 * selected in [dialog]. Returns false if the entry is invalid or
 * not an offset-supported format, true otherwise
 *******************************************************************/
bool EntryOperations::modifyGfxOffsets(ArchiveEntry* entry, ModifyOffsetsDialog* dialog)
{
	if (entry == NULL || entry->getType() == NULL)
		return false;

	// Check entry type
	EntryType* type = entry->getType();
	string entryformat = type->getFormat();
	if (!(entryformat == "img_doom" || entryformat == "img_doom_arah" ||
		entryformat == "img_doom_alpha" || entryformat == "img_doom_beta" ||
		entryformat == "img_png"))
	{
		LOG_MESSAGE(1, "Entry \"%s\" is of type \"%s\" which does not support offsets", entry->getName(), entry->getType()->getName());
		return false;
	}

	// Doom gfx format, normal and beta version.
	// Also arah format from alpha 0.2 because it uses the same header format.
	if (entryformat == "img_doom" || entryformat == "img_doom_beta" || entryformat == "image_doom_arah")
	{
		// Get patch header
		patch_header_t header;
		entry->seek(0, SEEK_SET);
		entry->read(&header, 8);

		// Calculate new offsets
		point2_t offsets = dialog->calculateOffsets(header.left, header.top, header.width, header.height);

		// Apply new offsets
		header.left = wxINT16_SWAP_ON_BE((int16_t)offsets.x);
		header.top = wxINT16_SWAP_ON_BE((int16_t)offsets.y);

		// Write new header to entry
		entry->seek(0, SEEK_SET);
		entry->write(&header, 8);
	}

	// Doom alpha gfx format
	else if (entryformat == "img_doom_alpha")
	{
		// Get patch header
		entry->seek(0, SEEK_SET);
		oldpatch_header_t header;
		entry->read(&header, 4);

		// Calculate new offsets
		point2_t offsets = dialog->calculateOffsets(header.left, header.top, header.width, header.height);

		// Apply new offsets
		header.left = (int8_t)offsets.x;
		header.top = (int8_t)offsets.y;

		// Write new header to entry
		entry->seek(0, SEEK_SET);
		entry->write(&header, 4);
	}

	// PNG format
	else if (entryformat == "img_png")
	{
		// Read width and height from IHDR chunk
		const uint8_t* data = entry->getData(true);
		const ihdr_t* ihdr = (ihdr_t*)(data + 12);
		uint32_t w = wxINT32_SWAP_ON_LE(ihdr->width);
		uint32_t h = wxINT32_SWAP_ON_LE(ihdr->height);

		// Find existing grAb chunk
		uint32_t grab_start = 0;
		int32_t xoff = 0;
		int32_t yoff = 0;
		for (uint32_t a = 0; a < entry->getSize(); a++)
		{
			// Check for 'grAb' header
			if (data[a] == 'g' && data[a + 1] == 'r' &&
			        data[a + 2] == 'A' && data[a + 3] == 'b')
			{
				grab_start = a - 4;
				grab_chunk_t* grab = (grab_chunk_t*)(data + a);
				xoff = wxINT32_SWAP_ON_LE(grab->xoff);
				yoff = wxINT32_SWAP_ON_LE(grab->yoff);
				break;
			}

			// Stop when we get to the 'IDAT' chunk
			if (data[a] == 'I' && data[a + 1] == 'D' &&
			        data[a + 2] == 'A' && data[a + 3] == 'T')
				break;
		}

		// Calculate new offsets
		point2_t offsets = dialog->calculateOffsets(xoff, yoff, w, h);
		xoff = offsets.x;
		yoff = offsets.y;

		// Create new grAb chunk
		uint32_t csize = wxUINT32_SWAP_ON_LE(8);
		grab_chunk_t gc = { { 'g', 'r', 'A', 'b' }, wxINT32_SWAP_ON_LE(xoff), wxINT32_SWAP_ON_LE(yoff) };
		uint32_t dcrc = wxUINT32_SWAP_ON_LE(Misc::crc((uint8_t*)&gc, 12));

		// Build new PNG from the original w/ the new grAb chunk
		MemChunk npng;
		uint32_t rest_start = 33;

		// Init new png data size
		if (grab_start == 0)
			npng.reSize(entry->getSize() + 20);
		else
			npng.reSize(entry->getSize());

		// Write PNG header and IHDR chunk
		npng.write(data, 33);

		// If no existing grAb chunk was found, write new one here
		if (grab_start == 0)
		{
			npng.write(&csize, 4);
			npng.write(&gc, 12);
			npng.write(&dcrc, 4);
		}
		else
		{
			// Otherwise write any other data before the existing grAb chunk
			uint32_t to_write = grab_start - 33;
			npng.write(data + 33, to_write);
			rest_start = grab_start + 20;

			// And now write the new grAb chunk
			npng.write(&csize, 4);
			npng.write(&gc, 12);
			npng.write(&dcrc, 4);
		}

		// Write the rest of the PNG data
		uint32_t to_write = entry->getSize() - rest_start;
		npng.write(data + rest_start, to_write);

		// Load new png data to the entry
		entry->importMemChunk(npng);

		// Set its type back to png
		entry->setType(type);
	}
	else
		return false;

	return true;
}

/* EntryOperations::setGfxOffsets
 * Changes the offsets of the given gfx entry. Returns false if the
 * entry is invalid or not an offset-supported format, true otherwise
 *******************************************************************/
bool EntryOperations::setGfxOffsets(ArchiveEntry* entry, int x, int y)
{
	if (entry == NULL || entry->getType() == NULL)
		return false;

	// Check entry type
	EntryType* type = entry->getType();
	string entryformat = type->getFormat();
	if (!(entryformat == "img_doom" || entryformat == "img_doom_arah" ||
		entryformat == "img_doom_alpha" || entryformat == "img_doom_beta" ||
		entryformat == "img_png"))
	{
		LOG_MESSAGE(1, "Entry \"%s\" is of type \"%s\" which does not support offsets", entry->getName(), entry->getType()->getName());
		return false;
	}

	// Doom gfx format, normal and beta version.
	// Also arah format from alpha 0.2 because it uses the same header format.
	if (entryformat == "img_doom" || entryformat == "img_doom_beta" || entryformat == "image_doom_arah")
	{
		// Get patch header
		patch_header_t header;
		entry->seek(0, SEEK_SET);
		entry->read(&header, 8);

		// Apply new offsets
		header.left = wxINT16_SWAP_ON_BE((int16_t)x);
		header.top = wxINT16_SWAP_ON_BE((int16_t)y);

		// Write new header to entry
		entry->seek(0, SEEK_SET);
		entry->write(&header, 8);
	}

	// Doom alpha gfx format
	else if (entryformat == "img_doom_alpha")
	{
		// Get patch header
		entry->seek(0, SEEK_SET);
		oldpatch_header_t header;
		entry->read(&header, 4);

		// Apply new offsets
		header.left = (int8_t)x;
		header.top = (int8_t)y;

		// Write new header to entry
		entry->seek(0, SEEK_SET);
		entry->write(&header, 4);
	}

	// PNG format
	else if (entryformat == "img_png")
	{
		// Find existing grAb chunk
		const uint8_t* data = entry->getData(true);
		uint32_t grab_start = 0;
		for (uint32_t a = 0; a < entry->getSize(); a++)
		{
			// Check for 'grAb' header
			if (data[a] == 'g' && data[a + 1] == 'r' &&
				data[a + 2] == 'A' && data[a + 3] == 'b')
			{
				grab_start = a - 4;
				break;
			}

			// Stop when we get to the 'IDAT' chunk
			if (data[a] == 'I' && data[a + 1] == 'D' &&
				data[a + 2] == 'A' && data[a + 3] == 'T')
				break;
		}

		// Create new grAb chunk
		uint32_t csize = wxUINT32_SWAP_ON_LE(8);
		grab_chunk_t gc ={ { 'g', 'r', 'A', 'b' }, wxINT32_SWAP_ON_LE(x), wxINT32_SWAP_ON_LE(y) };
		uint32_t dcrc = wxUINT32_SWAP_ON_LE(Misc::crc((uint8_t*)&gc, 12));

		// Build new PNG from the original w/ the new grAb chunk
		MemChunk npng;
		uint32_t rest_start = 33;

		// Init new png data size
		if (grab_start == 0)
			npng.reSize(entry->getSize() + 20);
		else
			npng.reSize(entry->getSize());

		// Write PNG header and IHDR chunk
		npng.write(data, 33);

		// If no existing grAb chunk was found, write new one here
		if (grab_start == 0)
		{
			npng.write(&csize, 4);
			npng.write(&gc, 12);
			npng.write(&dcrc, 4);
		}
		else
		{
			// Otherwise write any other data before the existing grAb chunk
			uint32_t to_write = grab_start - 33;
			npng.write(data + 33, to_write);
			rest_start = grab_start + 20;

			// And now write the new grAb chunk
			npng.write(&csize, 4);
			npng.write(&gc, 12);
			npng.write(&dcrc, 4);
		}

		// Write the rest of the PNG data
		uint32_t to_write = entry->getSize() - rest_start;
		npng.write(data + rest_start, to_write);

		// Load new png data to the entry
		entry->importMemChunk(npng);

		// Set its type back to png
		entry->setType(type);
	}
	else
		return false;

	return true;
}

/* EntryOperations::openMapDB2
 * Opens the map at [entry] with Doom Builder 2, including all open
 * resource archives. Sets up a FileMonitor to update the map in the
 * archive if any changes are made to it in DB2
 *******************************************************************/
bool EntryOperations::openMapDB2(ArchiveEntry* entry)
{
#ifdef __WXMSW__	// Windows only
	string path = path_db2;

	if (path.IsEmpty())
	{
		// Check for DB2 location registry key
		wxRegKey key(wxRegKey::HKLM, "SOFTWARE\\CodeImp\\Doom Builder");
		key.QueryValue("Location", path);

		// Can't proceed if DB2 isn't installed
		if (path.IsEmpty())
		{
			wxMessageBox("Doom Builder 2 must be installed to use this feature.", "Doom Builder 2 Not Found");
			return false;
		}

		// Add default executable name
		path += "\\Builder.exe";
	}

	// Get map info for entry
	Archive::mapdesc_t map = entry->getParent()->getMapInfo(entry);

	// Check valid map
	if (map.format == MAP_UNKNOWN)
		return false;

	// Export the map to a temp .wad file
	string filename = App::path(entry->getParent()->getFilename(false) + "-" + entry->getName(true) + ".wad", App::Dir::Temp);
	filename.Replace("/", "-");
	if (map.archive)
	{
		entry->exportFile(filename);
		entry->lock();
	}
	else
	{
		// Write map entries to temporary wad archive
		if (map.head)
		{
			WadArchive archive;

			// Add map entries to archive
			ArchiveEntry* e = map.head;
			while (true)
			{
				archive.addEntry(e, "", true);
				e->lock();
				if (e == map.end) break;
				e = e->nextEntry();
			}

			// Write archive to file
			archive.save(filename);
		}
	}

	// Generate Doom Builder command line
	string cmd = S_FMT("%s \"%s\" -map %s", path, filename, entry->getName());

	// Add base resource archive to command line
	Archive* base = theArchiveManager->baseResourceArchive();
	if (base)
	{
		if (base->getType() == ARCHIVE_WAD)
			cmd += S_FMT(" -resource wad \"%s\"", base->getFilename());
		else if (base->getType() == ARCHIVE_ZIP)
			cmd += S_FMT(" -resource pk3 \"%s\"", base->getFilename());
	}

	// Add resource archives to command line
	for (int a = 0; a < theArchiveManager->numArchives(); ++a)
	{
		Archive* archive = theArchiveManager->getArchive(a);

		// Check archive type (only wad and zip supported by db2)
		if (archive->getType() == ARCHIVE_WAD)
			cmd += S_FMT(" -resource wad \"%s\"", archive->getFilename());
		else if (archive->getType() == ARCHIVE_ZIP)
			cmd += S_FMT(" -resource pk3 \"%s\"", archive->getFilename());
	}

	// Run DB2
	FileMonitor* fm = new DB2MapFileMonitor(filename, entry->getParent(), entry->getName(true));
	wxExecute(cmd, wxEXEC_ASYNC, fm->getProcess());

	return true;
#else
	return false;
#endif//__WXMSW__
}

/* EntryOperations::modifyalPhChunk
 * Add or remove the alPh chunk from a PNG entry
 *******************************************************************/
bool EntryOperations::modifyalPhChunk(ArchiveEntry* entry, bool value)
{
	if (!entry || !entry->getType())
		return false;

	// Don't bother if the entry is locked.
	if (entry->isLocked())
		return false;

	// Check entry type
	if (!(entry->getType()->getFormat() == "img_png"))
	{
		LOG_MESSAGE(1, "Entry \"%s\" is of type \"%s\" rather than PNG", entry->getName(), entry->getType()->getName());
		return false;
	}

	// Read width and height from IHDR chunk
	const uint8_t* data = entry->getData(true);
	const ihdr_t* ihdr = (ihdr_t*)(data + 12);

	// Find existing alPh chunk
	uint32_t alph_start = 0;
	for (uint32_t a = 0; a < entry->getSize(); a++)
	{
		// Check for 'alPh' header
		if (data[a] == 'a' && data[a + 1] == 'l' &&
		        data[a + 2] == 'P' && data[a + 3] == 'h')
		{
			alph_start = a - 4;
			break;
		}

		// Stop when we get to the 'IDAT' chunk
		if (data[a] == 'I' && data[a + 1] == 'D' &&
		        data[a + 2] == 'A' && data[a + 3] == 'T')
			break;
	}

	// We want to set alPh, and it is already there: nothing to do.
	if (value && alph_start > 0)
		return false;

	// We want to unset alPh, and it is already not there: nothing to do either.
	else if (!value && alph_start == 0)
		return false;

	// We want to set alPh, which is missing: create it.
	else if (value && alph_start == 0)
	{
		// Build new PNG from the original w/ the new alPh chunk
		MemChunk npng;

		// Init new png data size
		npng.reSize(entry->getSize() + 12);

		// Write PNG header and IHDR chunk
		npng.write(data, 33);

		// Create new alPh chunk
		uint32_t csize = wxUINT32_SWAP_ON_LE(0);
		alph_chunk_t gc = { 'a', 'l', 'P', 'h' };
		uint32_t dcrc = wxUINT32_SWAP_ON_LE(Misc::crc((uint8_t*)&gc, 4));

		// Create alPh chunk
		npng.write(&csize, 4);
		npng.write(&gc, 4);
		npng.write(&dcrc, 4);

		// Write the rest of the PNG data
		uint32_t to_write = entry->getSize() - 33;
		npng.write(data + 33, to_write);

		// Load new png data to the entry
		entry->importMemChunk(npng);
	}

	// We want to unset alPh, which is present: delete it.
	else if (!value && alph_start > 0)
	{
		// Build new PNG from the original without the alPh chunk
		MemChunk npng;
		uint32_t rest_start = alph_start + 12;

		// Init new png data size
		npng.reSize(entry->getSize() - 12);

		// Write PNG info before alPh chunk
		npng.write(data, alph_start);

		// Write the rest of the PNG data
		uint32_t to_write = entry->getSize() - rest_start;
		npng.write(data + rest_start, to_write);

		// Load new png data to the entry
		entry->importMemChunk(npng);
	}

	// We don't know what we want, but it can't be good, so we do nothing.
	else
		return false;

	return true;
}

/* EntryOperations::modifytRNSChunk
 * Add or remove the tRNS chunk from a PNG entry
 * Returns true if the entry was altered
 *******************************************************************/
bool EntryOperations::modifytRNSChunk(ArchiveEntry* entry, bool value)
{
	// Avoid NULL pointers, they're annoying.
	if (!entry || !entry->getType())
		return false;

	// Don't bother if the entry is locked.
	if (entry->isLocked())
		return false;

	// Check entry type
	if (!(entry->getType()->getFormat() == "img_png"))
	{
		LOG_MESSAGE(1, "Entry \"%s\" is of type \"%s\" rather than PNG", entry->getName(), entry->getTypeString());
		return false;
	}

	// Read width and height from IHDR chunk
	const uint8_t* data = entry->getData(true);
	const ihdr_t* ihdr = (ihdr_t*)(data + 12);

	// tRNS chunks are only valid for paletted PNGs, and must be before the first IDAT.
	// Specs say they must be after PLTE chunk as well, so to play it safe, we'll insert
	// them just before the first IDAT.
	uint32_t trns_start = 0;
	uint32_t trns_size	= 0;
	uint32_t idat_start = 0;
	for (uint32_t a = 0; a < entry->getSize(); a++)
	{

		// Check for 'tRNS' header
		if (data[a] == 't' && data[a + 1] == 'R' &&
		        data[a + 2] == 'N' && data[a + 3] == 'S')
		{
			trns_start = a - 4;
			trans_chunk_t* trns = (trans_chunk_t*)(data + a);
			trns_size = 12 + READ_B32(data, a-4);
		}

		// Stop when we get to the 'IDAT' chunk
		if (data[a] == 'I' && data[a + 1] == 'D' &&
		        data[a + 2] == 'A' && data[a + 3] == 'T')
		{
			idat_start = a - 4;
			break;
		}
	}

	// The IDAT chunk starts before the header is finished, this doesn't make sense, abort.
	if (idat_start < 33)
		return false;

	// We want to set tRNS, and it is already there: nothing to do.
	if (value && trns_start > 0)
		return false;

	// We want to unset tRNS, and it is already not there: nothing to do either.
	else if (!value && trns_start == 0)
		return false;

	// We want to set tRNS, which is missing: create it. We're just going to set index 0 to 0,
	// and leave the rest of the palette indices alone.
	else if (value && trns_start == 0)
	{
		// Build new PNG from the original w/ the new tRNS chunk
		MemChunk npng;

		// Init new png data size
		npng.reSize(entry->getSize() + 13);

		// Write PNG header stuff up to the first IDAT chunk
		npng.write(data, idat_start);

		// Create new tRNS chunk
		uint32_t csize = wxUINT32_SWAP_ON_LE(1);
		trans_chunk_t gc = { 't', 'R', 'N', 'S', '\0' };
		uint32_t dcrc = wxUINT32_SWAP_ON_LE(Misc::crc((uint8_t*)&gc, 5));

		// Write tRNS chunk
		npng.write(&csize, 4);
		npng.write(&gc, 5);
		npng.write(&dcrc, 4);

		// Write the rest of the PNG data
		uint32_t to_write = entry->getSize() - idat_start;
		npng.write(data + idat_start, to_write);

		// Load new png data to the entry
		entry->importMemChunk(npng);
	}

	// We want to unset tRNS, which is present: delete it.
	else if (!value && trns_start > 0)
	{
		// Build new PNG from the original without the tRNS chunk
		MemChunk npng;
		uint32_t rest_start = trns_start + trns_size;

		// Init new png data size
		npng.reSize(entry->getSize() - trns_size);

		// Write PNG header and stuff up to tRNS start
		npng.write(data, trns_start);

		// Write the rest of the PNG data
		uint32_t to_write = entry->getSize() - rest_start;
		npng.write(data + rest_start, to_write);

		// Load new png data to the entry
		entry->importMemChunk(npng);
	}

	// We don't know what we want, but it can't be good, so we do nothing.
	else
		return false;

	return true;
}

/* EntryOperations::getalPhChunk
 * Tell whether a PNG entry has an alPh chunk or not
 *******************************************************************/
bool EntryOperations::getalPhChunk(ArchiveEntry* entry)
{
	if (!entry || !entry->getType())
		return false;

	// Check entry type
	if (entry->getType()->getFormat() != "img_png")
	{
		LOG_MESSAGE(1, "Entry \"%s\" is of type \"%s\" rather than PNG", entry->getName(), entry->getTypeString());
		return false;
	}

	// Find existing alPh chunk
	const uint8_t* data = entry->getData(true);
	for (uint32_t a = 0; a < entry->getSize(); a++)
	{
		// Check for 'alPh' header
		if (data[a] == 'a' && data[a + 1] == 'l' &&
		        data[a + 2] == 'P' && data[a + 3] == 'h')
		{
			return true;
		}

		// Stop when we get to the 'IDAT' chunk
		if (data[a] == 'I' && data[a + 1] == 'D' &&
		        data[a + 2] == 'A' && data[a + 3] == 'T')
			break;
	}
	return false;
}

/* EntryOperations::gettRNSChunk
 * Add or remove the tRNS chunk from a PNG entry
 *******************************************************************/
bool EntryOperations::gettRNSChunk(ArchiveEntry* entry)
{
	if (!entry || !entry->getType())
		return false;

	// Check entry type
	if (entry->getType()->getFormat() != "img_png")
	{
		LOG_MESSAGE(1, "Entry \"%s\" is of type \"%s\" rather than PNG", entry->getName(), entry->getTypeString());
		return false;
	}

	// tRNS chunks are only valid for paletted PNGs, and the chunk must before the first IDAT.
	// Specs say it should be after a PLTE chunk, but that's not always the case (e.g., sgrna7a3.png).
	const uint8_t* data = entry->getData(true);
	for (uint32_t a = 0; a < entry->getSize(); a++)
	{

		// Check for 'tRNS' header
		if (data[a] == 't' && data[a + 1] == 'R' &&
		        data[a + 2] == 'N' && data[a + 3] == 'S')
		{
			return true;
		}

		// Stop when we get to the 'IDAT' chunk
		if (data[a] == 'I' && data[a + 1] == 'D' &&
		        data[a + 2] == 'A' && data[a + 3] == 'T')
			break;
	}
	return false;
}

/* EntryOperations::readgrAbChunk
 * Tell whether a PNG entry has a grAb chunk or not and loads the
 * offset values in the given references
 *******************************************************************/
bool EntryOperations::readgrAbChunk(ArchiveEntry* entry, point2_t& offsets)
{
	if (!entry || !entry->getType())
		return false;

	// Check entry type
	if (entry->getType()->getFormat() != "img_png")
	{
		LOG_MESSAGE(1, "Entry \"%s\" is of type \"%s\" rather than PNG", entry->getName(), entry->getTypeString());
		return false;
	}

	// Find existing grAb chunk
	const uint8_t* data = entry->getData(true);
	for (uint32_t a = 0; a < entry->getSize(); a++)
	{
		// Check for 'grAb' header
		if (data[a] == 'g' && data[a + 1] == 'r' &&
		        data[a + 2] == 'A' && data[a + 3] == 'b')
		{
			offsets.x = READ_B32(data, a + 4);
			offsets.y = READ_B32(data, a + 8);
			return true;
		}

		// Stop when we get to the 'IDAT' chunk
		if (data[a] == 'I' && data[a + 1] == 'D' &&
		        data[a + 2] == 'A' && data[a + 3] == 'T')
			break;
	}
	return false;
}

/* EntryOperations::addToPatchTable
 * Adds all [entries] to their parent archive's patch table, if it
 * exists. If not, the user is prompted to create or import texturex
 * entries
 *******************************************************************/
bool EntryOperations::addToPatchTable(vector<ArchiveEntry*> entries)
{
	// Check any entries were given
	if (entries.size() == 0)
		return true;

	// Get parent archive
	Archive* parent = entries[0]->getParent();
	if (parent == NULL)
		return true;

	// Find patch table in parent archive
	Archive::search_options_t opt;
	opt.match_type = EntryType::getType("pnames");
	ArchiveEntry* pnames = parent->findLast(opt);

	// Check it exists
	if (!pnames)
	{
		// Create texture entries
		if (!TextureXEditor::setupTextureEntries(parent))
			return false;

		pnames = parent->findLast(opt);

		// If the archive already has ZDoom TEXTURES, it might still
		// not have a PNAMES lump; so create an empty one.
		if (!pnames)
		{
			pnames = new ArchiveEntry("PNAMES.lmp", 4);
			uint32_t nada = 0;
			pnames->write(&nada, 4);
			pnames->seek(0, SEEK_SET);
			parent->addEntry(pnames);
		}
	}

	// Check it isn't locked (texturex editor open or iwad)
	if (pnames->isLocked())
	{
		if (parent->isReadOnly())
			wxMessageBox("Cannot perform this action on an IWAD", "Error", wxICON_ERROR);
		else
			wxMessageBox("Cannot perform this action because one or more texture related entries is locked. Please close the archive's texture editor if it is open.", "Error", wxICON_ERROR);

		return false;
	}

	// Load to patch table
	PatchTable ptable;
	ptable.loadPNAMES(pnames);

	// Add entry names to patch table
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Check entry type
		if (!(entries[a]->getType()->extraProps().propertyExists("image")))
		{
			LOG_MESSAGE(1, "Entry %s is not a valid image", entries[a]->getName());
			continue;
		}

		// Check entry name
		if (entries[a]->getName(true).Length() > 8)
		{
			LOG_MESSAGE(1, "Entry %s has too long a name to add to the patch table (name must be 8 characters max)", entries[a]->getName());
			continue;
		}

		ptable.addPatch(entries[a]->getName(true));
	}

	// Write patch table data back to pnames entry
	return ptable.writePNAMES(pnames);
}

/* EntryOperations::createTexture
 * Same as addToPatchTable, but also creates a single-patch texture
 * from each added patch
 *******************************************************************/
bool EntryOperations::createTexture(vector<ArchiveEntry*> entries)
{
	// Check any entries were given
	if (entries.size() == 0)
		return true;

	// Get parent archive
	Archive* parent = entries[0]->getParent();

	// Create texture entries if needed
	if (!TextureXEditor::setupTextureEntries(parent))
		return false;

	// Find texturex entry to add to
	Archive::search_options_t opt;
	opt.match_type = EntryType::getType("texturex");
	ArchiveEntry* texturex = parent->findFirst(opt);

	// Check it exists
	bool zdtextures = false;
	if (!texturex)
	{
		opt.match_type = EntryType::getType("zdtextures");
		texturex = parent->findFirst(opt);

		if (!texturex)
			return false;
		else
			zdtextures = true;
	}

	// Find patch table in parent archive
	ArchiveEntry* pnames = NULL;
	if (!zdtextures)
	{
		opt.match_type = EntryType::getType("pnames");
		pnames = parent->findLast(opt);

		// Check it exists
		if (!pnames)
			return false;
	}

	// Check entries aren't locked (texture editor open or iwad)
	if ((pnames && pnames->isLocked()) || texturex->isLocked())
	{
		if (parent->isReadOnly())
			wxMessageBox("Cannot perform this action on an IWAD", "Error", wxICON_ERROR);
		else
			wxMessageBox("Cannot perform this action because one or more texture related entries is locked. Please close the archive's texture editor if it is open.", "Error", wxICON_ERROR);

		return false;
	}

	TextureXList tx;
	PatchTable ptable;
	if (zdtextures)
	{
		// Load TEXTURES
		tx.readTEXTURESData(texturex);
	}
	else
	{
		// Load patch table
		ptable.loadPNAMES(pnames);

		// Load TEXTUREx
		tx.readTEXTUREXData(texturex, ptable);
	}

	// Create textures from entries
	SImage image;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Check entry type
		if (!(entries[a]->getType()->extraProps().propertyExists("image")))
		{
			LOG_MESSAGE(1, "Entry %s is not a valid image", entries[a]->getName());
			continue;
		}

		// Check entry name
		string name = entries[a]->getName(true);
		if (name.Length() > 8)
		{
			LOG_MESSAGE(1, "Entry %s has too long a name to add to the patch table (name must be 8 characters max)", entries[a]->getName());
			continue;
		}

		// Add to patch table
		if (!zdtextures)
			ptable.addPatch(name);

		// Load patch to temp image
		Misc::loadImageFromEntry(&image, entries[a]);

		// Create texture
		CTexture* ntex = new CTexture(zdtextures);
		ntex->setName(name);
		ntex->addPatch(name, 0, 0);
		ntex->setWidth(image.getWidth());
		ntex->setHeight(image.getHeight());

		// Setup texture scale
		if (tx.getFormat() == TXF_TEXTURES)
			ntex->setScale(1, 1);
		else
			ntex->setScale(0, 0);

		// Add to texture list
		tx.addTexture(ntex);
	}

	if (zdtextures)
	{
		// Write texture data back to textures entry
		tx.writeTEXTURESData(texturex);
	}
	else
	{
		// Write patch table data back to pnames entry
		ptable.writePNAMES(pnames);

		// Write texture data back to texturex entry
		tx.writeTEXTUREXData(texturex, ptable);
	}

	return true;
}

/* EntryOperations::convertTextures
 * Converts multiple TEXTURE1/2 entries to a single ZDoom text-based
 * TEXTURES entry
 *******************************************************************/
bool EntryOperations::convertTextures(vector<ArchiveEntry*> entries)
{
	// Check any entries were given
	if (entries.size() == 0)
		return false;

	// Get parent archive of entries
	Archive* parent = entries[0]->getParent();

	// Can't do anything if entry isn't in an archive
	if (!parent)
		return false;

	// Find patch table in parent archive
	Archive::search_options_t opt;
	opt.match_type = EntryType::getType("pnames");
	ArchiveEntry* pnames = parent->findLast(opt);

	// Check it exists
	if (!pnames)
		return false;

	// Load patch table
	PatchTable ptable;
	ptable.loadPNAMES(pnames);

	// Read all texture entries to a single list
	TextureXList tx;
	for (unsigned a = 0; a < entries.size(); a++)
		tx.readTEXTUREXData(entries[a], ptable, true);

	// Convert to extended (TEXTURES) format
	tx.convertToTEXTURES();

	// Create new TEXTURES entry and write to it
	ArchiveEntry* textures = parent->addNewEntry("TEXTURES", parent->entryIndex(entries[0]));
	if (textures)
	{
		bool ok = tx.writeTEXTURESData(textures);
		EntryType::detectEntryType(textures);
		textures->setExtensionByType();
		return ok;
	}
	else
		return false;
}

/* EntryOperations::findTextureErrors
 * Detect errors in a TEXTUREx entry
 *******************************************************************/
bool EntryOperations::findTextureErrors(vector<ArchiveEntry*> entries)
{
	// Check any entries were given
	if (entries.size() == 0)
		return false;

	// Get parent archive of entries
	Archive* parent = entries[0]->getParent();

	// Can't do anything if entry isn't in an archive
	if (!parent)
		return false;

	// Find patch table in parent archive
	Archive::search_options_t opt;
	opt.match_type = EntryType::getType("pnames");
	ArchiveEntry* pnames = parent->findLast(opt);

	// Check it exists
	if (!pnames)
		return false;

	// Load patch table
	PatchTable ptable;
	ptable.loadPNAMES(pnames);

	// Read all texture entries to a single list
	TextureXList tx;
	for (unsigned a = 0; a < entries.size(); a++)
		tx.readTEXTUREXData(entries[a], ptable, true);

	// Detect errors
	tx.findErrors();

	return true;
}

/* EntryOperations::compileACS
 * Attempts to compile [entry] as an ACS script. If the entry is
 * named SCRIPTS, the compiled data is imported to the BEHAVIOR
 * entry previous to it, otherwise it is imported to a same-name
 * compiled library entry in the acs namespace
 *******************************************************************/
bool EntryOperations::compileACS(ArchiveEntry* entry, bool hexen, ArchiveEntry* target, wxFrame* parent)
{
	// Check entry was given
	if (!entry)
		return false;

	// Check entry has a parent (this is useless otherwise)
	if (!target && !entry->getParent())
		return false;

	// Check entry is text
	if (!EntryDataFormat::getFormat("text")->isThisFormat(entry->getMCData()))
	{
		wxMessageBox("Error: Entry does not appear to be text", "Error", wxOK|wxCENTRE|wxICON_ERROR);
		return false;
	}

	// Check if the ACC path is set up
	string accpath = path_acc;
	if (accpath.IsEmpty() || !wxFileExists(accpath))
	{
		wxMessageBox("Error: ACC path not defined, please configure in SLADE preferences", "Error", wxOK|wxCENTRE|wxICON_ERROR);
		PreferencesDialog::openPreferences(parent, "ACS");
		return false;
	}

	// Setup some path strings
	string srcfile = App::path(entry->getName(true) + ".acs", App::Dir::Temp);
	string ofile = App::path(entry->getName(true) + ".o", App::Dir::Temp);
	wxArrayString include_paths = wxSplit(path_acc_libs, ';');

	// Setup command options
	string opt;
	if (hexen)
		opt += " -h";
	if (!include_paths.IsEmpty())
	{
		for (unsigned a = 0; a < include_paths.size(); a++)
			opt += S_FMT(" -i \"%s\"", include_paths[a]);
	}

	// Find/export any resource libraries
	Archive::search_options_t sopt;
	sopt.match_type = EntryType::getType("acs");
	sopt.search_subdirs = true;
	vector<ArchiveEntry*> entries = theArchiveManager->findAllResourceEntries(sopt);
	wxArrayString lib_paths;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Ignore SCRIPTS
		if (S_CMPNOCASE(entries[a]->getName(true), "SCRIPTS"))
			continue;

		// Ignore entries from other archives
		if (entry->getParent() &&
			(entry->getParent()->getFilename(true) != entries[a]->getParent()->getFilename(true)))
			continue;

		string path = App::path(entries[a]->getName(true) + ".acs", App::Dir::Temp);
		entries[a]->exportFile(path);
		lib_paths.Add(path);
		LOG_MESSAGE(2, "Exporting ACS library %s", entries[a]->getName());
	}

	// Export script to file
	entry->exportFile(srcfile);

	// Execute acc
	string command = path_acc + " " + opt + " \"" + srcfile + "\" \"" + ofile + "\"";
	wxArrayString output;
	wxArrayString errout;
	wxTheApp->SetTopWindow(parent);
	wxExecute(command, output, errout, wxEXEC_SYNC);
	wxTheApp->SetTopWindow(MainEditor::windowWx());

	// Log output
	Log::console("ACS compiler output:");
	string output_log;
	if (!output.IsEmpty())
	{
		const char *title1 = "=== Log: ===\n";
		Log::console(title1);
		output_log += title1;
		for (unsigned a = 0; a < output.size(); a++)
		{
			Log::console(output[a]);
			output_log += output[a];
		}
	}

	if (!errout.IsEmpty())
	{
		const char *title2 = "\n=== Error log: ===\n";
		Log::console(title2);
		output_log += title2;
		for (unsigned a = 0; a < errout.size(); a++)
		{
			Log::console(errout[a]);
			output_log += errout[a] + "\n";
		}
	}

	// Delete source file
	wxRemoveFile(srcfile);

	// Delete library files
	for (unsigned a = 0; a < lib_paths.size(); a++)
		wxRemoveFile(lib_paths[a]);

	// Check it compiled successfully
	bool success = wxFileExists(ofile);
	if (success)
	{
		// If no target entry was given, find one
		if (!target)
		{
			// Check if the script is a map script (BEHAVIOR)
			if (S_CMPNOCASE(entry->getName(), "SCRIPTS"))
			{
				// Get entry before SCRIPTS
				ArchiveEntry* prev = entry->prevEntry();

				// Create a new entry there if it isn't BEHAVIOR
				if (!prev || !(S_CMPNOCASE(prev->getName(), "BEHAVIOR")))
					prev = entry->getParent()->addNewEntry("BEHAVIOR", entry->getParent()->entryIndex(entry));

				// Import compiled script
				prev->importFile(ofile);
			}
			else
			{
				// Otherwise, treat it as a library

				// See if the compiled library already exists as an entry
				Archive::search_options_t opt;
				opt.match_namespace = "acs";
				opt.match_name = entry->getName(true);
				if (entry->getParent()->getDesc().names_extensions)
				{
					opt.match_name += ".o";
					opt.ignore_ext = false;
				}
				ArchiveEntry* lib = entry->getParent()->findLast(opt);

				// If it doesn't exist, create it
				if (!lib)
					lib = entry->getParent()->addEntry(new ArchiveEntry(entry->getName(true) + ".o"), "acs");

				// Import compiled script
				lib->importFile(ofile);
			}
		}
		else
			target->importFile(ofile);

		// Delete compiled script file
		wxRemoveFile(ofile);
	}

	if (!success || acc_always_show_output)
	{
		string errors;
		if (wxFileExists(App::path("acs.err", App::Dir::Temp)))
		{
			// Read acs.err to string
			wxFile file(App::path("acs.err", App::Dir::Temp));
			char* buf = new char[file.Length()];
			file.Read(buf, file.Length());
			errors = wxString::From8BitData(buf, file.Length());
			delete[] buf;
		}
		else
			errors = output_log;

		if (errors != "" || !success)
		{
			ExtMessageDialog dlg(NULL, success ? "ACC Output" : "Error Compiling");
			dlg.setMessage(success ?
				"The following errors were encountered while compiling, please fix them and recompile:" :
				"Compiler output shown below: "
			);
			dlg.setExt(errors);
			dlg.ShowModal();
		}

		return success;
	}

	return true;
}

/* EntryOperations::exportAsPNG
 * Converts [entry] to a PNG image (if possible) and saves the PNG
 * data to a file [filename]. Does not alter the entry data itself
 *******************************************************************/
bool EntryOperations::exportAsPNG(ArchiveEntry* entry, string filename)
{
	// Check entry was given
	if (!entry)
		return false;

	// Create image from entry
	SImage image;
	if (!Misc::loadImageFromEntry(&image, entry))
	{
		LOG_MESSAGE(1, "Error converting %s: %s", entry->getName(), Global::error);
		return false;
	}

	// Write png data
	MemChunk png;
	SIFormat* fmt_png = SIFormat::getFormat("png");
	if (!fmt_png->saveImage(image, png, MainEditor::currentPalette(entry)))
	{
		LOG_MESSAGE(1, "Error converting %s", entry->getName());
		return false;
	}

	// Export file
	return png.exportFile(filename);
}

/* EntryOperations::optimizePNG
 * Attempts to optimize [entry] using external PNG optimizers.
 *******************************************************************/
bool EntryOperations::optimizePNG(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Check entry has a parent (this is useless otherwise)
	if (!entry->getParent())
		return false;

	// Check entry is PNG
	if (!EntryDataFormat::getFormat("img_png")->isThisFormat(entry->getMCData()))
	{
		wxMessageBox("Error: Entry does not appear to be PNG", "Error", wxOK|wxCENTRE|wxICON_ERROR);
		return false;
	}

	// Check if the PNG tools path are set up, at least one of them should be
	string pngpathc = path_pngcrush;
	string pngpatho = path_pngout;
	string pngpathd = path_deflopt;
	if ((pngpathc.IsEmpty() || !wxFileExists(pngpathc)) &&
	        (pngpatho.IsEmpty() || !wxFileExists(pngpatho)) &&
	        (pngpathd.IsEmpty() || !wxFileExists(pngpathd)))
	{
		LOG_MESSAGE(1, "PNG tool paths not defined or invalid, no optimization done.");
		return false;
	}

	// Save special chunks
	point2_t offsets;
	bool alphchunk = getalPhChunk(entry);
	bool grabchunk = readgrAbChunk(entry, offsets);
	string errormessages = "";
	wxArrayString output;
	wxArrayString errors;
	size_t oldsize = entry->getSize();
	size_t crushsize = 0, outsize = 0, deflsize = 0;
	bool crushed = false, outed = false;

	// Run PNGCrush
	if (!pngpathc.IsEmpty() && wxFileExists(pngpathc))
	{
		wxFileName fn(pngpathc);
		fn.SetExt("opt");
		string pngfile = fn.GetFullPath();
		fn.SetExt("png");
		string optfile = fn.GetFullPath();
		entry->exportFile(pngfile);

		string command = path_pngcrush + " -brute \"" + pngfile + "\" \"" + optfile + "\"";
		output.Empty(); errors.Empty();
		wxExecute(command, output, errors, wxEXEC_SYNC);

		if (wxFileExists(optfile))
		{
			if (optfile.size() < oldsize)
			{
				entry->importFile(optfile);
				wxRemoveFile(optfile);
				wxRemoveFile(pngfile);
			}
			else errormessages += "PNGCrush failed to reduce file size further.\n";
			crushed = true;
		}
		else errormessages += "PNGCrush failed to create optimized file.\n";
		crushsize = entry->getSize();

		// send app output to console if wanted
		if (0)
		{
			string crushlog = "";
			if (errors.GetCount())
			{
				crushlog += "PNGCrush error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					crushlog += errors[i] + "\n";
				errormessages += crushlog;
			}
			if (output.GetCount())
			{
				crushlog += "PNGCrush output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					crushlog += output[i] + "\n";
			}
			LOG_MESSAGE(1, crushlog);
		}
	}

	// Run PNGOut
	if (!pngpatho.IsEmpty() && wxFileExists(pngpatho))
	{
		wxFileName fn(pngpatho);
		fn.SetExt("opt");
		string pngfile = fn.GetFullPath();
		fn.SetExt("png");
		string optfile = fn.GetFullPath();
		entry->exportFile(pngfile);

		string command = path_pngout + " /y \"" + pngfile + "\" \"" + optfile + "\"";
		output.Empty(); errors.Empty();
		wxExecute(command, output, errors, wxEXEC_SYNC);

		if (wxFileExists(optfile))
		{
			if (optfile.size() < oldsize)
			{
				entry->importFile(optfile);
				wxRemoveFile(optfile);
				wxRemoveFile(pngfile);
			}
			else errormessages += "PNGout failed to reduce file size further.\n";
			outed = true;
		}
		else if (!crushed)
			// Don't treat it as an error if PNGout couldn't create a smaller file than PNGCrush
			errormessages += "PNGout failed to create optimized file.\n";
		outsize = entry->getSize();

		// send app output to console if wanted
		if (0)
		{
			string pngoutlog = "";
			if (errors.GetCount())
			{
				pngoutlog += "PNGOut error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					pngoutlog += errors[i] + "\n";
				errormessages += pngoutlog;
			}
			if (output.GetCount())
			{
				pngoutlog += "PNGOut output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					pngoutlog += output[i] + "\n";
			}
			LOG_MESSAGE(1, pngoutlog);
		}
	}

	// Run deflopt
	if (!pngpathd.IsEmpty() && wxFileExists(pngpathd))
	{
		wxFileName fn(pngpathd);
		fn.SetExt("png");
		string pngfile = fn.GetFullPath();
		entry->exportFile(pngfile);

		string command = path_deflopt + " /sf \"" + pngfile + "\"";
		output.Empty(); errors.Empty();
		wxExecute(command, output, errors, wxEXEC_SYNC);

		entry->importFile(pngfile);
		wxRemoveFile(pngfile);
		deflsize = entry->getSize();

		// send app output to console if wanted
		if (0)
		{
			string defloptlog = "";
			if (errors.GetCount())
			{
				defloptlog += "DeflOpt error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					defloptlog += errors[i] + "\n";
				errormessages += defloptlog;
			}
			if (output.GetCount())
			{
				defloptlog += "DeflOpt output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					defloptlog += output[i] + "\n";
			}
			LOG_MESSAGE(1, defloptlog);
		}
	}
	output.Clear(); errors.Clear();

	// Rewrite special chunks
	if (alphchunk) modifyalPhChunk(entry, true);
	if (grabchunk) setGfxOffsets(entry, offsets.x, offsets.y);

	LOG_MESSAGE(1, "PNG %s size %i =PNGCrush=> %i =PNGout=> %i =DeflOpt=> %i =+grAb/alPh=> %i",
	             entry->getName(), oldsize, crushsize, outsize, deflsize, entry->getSize());


	if (!crushed && !outed && !errormessages.IsEmpty())
	{
		ExtMessageDialog dlg(NULL, "Optimizing Report");
		dlg.setMessage("The following issues were encountered while optimizing:");
		dlg.setExt(errormessages);
		dlg.ShowModal();

		return false;
	}

	return true;
}


void fixpngsrc(ArchiveEntry* entry)
{
	if (!entry)
		return;
	const uint8_t* source = entry->getData();
	uint8_t* data = new uint8_t[entry->getSize()];
	memcpy(data, source, entry->getSize());

	// Last check that it's a PNG
	uint32_t header1 = READ_B32(data, 0);
	uint32_t header2 = READ_B32(data, 4);
	if (header1 != 0x89504E47 || header2 != 0x0D0A1A0A)
		return;

	// Loop through each chunk and recompute CRC
	uint32_t pointer = 8;
	bool neededchange = false;
	while (pointer < entry->getSize())
	{
		if (pointer + 12 > entry->getSize())
		{
			LOG_MESSAGE(1, "Entry %s cannot be repaired.", entry->getName());
			delete[] data;
			return;
		}
		uint32_t chsz = READ_B32(data, pointer);
		if (pointer + 12 + chsz > entry->getSize())
		{
			LOG_MESSAGE(1, "Entry %s cannot be repaired.", entry->getName());
			delete[] data;
			return;
		}
		uint32_t crc = Misc::crc(data + pointer + 4, 4 + chsz);
		if (crc != READ_B32(data, pointer + 8 + chsz))
		{
			LOG_MESSAGE(1, "Chunk %c%c%c%c has bad CRC", data[pointer+4], data[pointer+5], data[pointer+6], data[pointer+7]);
			neededchange = true;
			data[pointer +  8 + chsz] = crc >> 24;
			data[pointer +  9 + chsz] = (crc & 0x00ffffff) >> 16;
			data[pointer + 10 + chsz] = (crc & 0x0000ffff) >> 8;
			data[pointer + 11 + chsz] = (crc & 0x000000ff);
		}
		pointer += (chsz + 12);
	}
	// Import new data with fixed CRC
	if (neededchange)
	{
		entry->importMem(data, entry->getSize());
	}
	delete[] data;
	return;
}

/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

CONSOLE_COMMAND(fixpngcrc, 0, true)
{
	vector<ArchiveEntry*> selection = MainEditor::currentEntrySelection();
	if (selection.size() == 0)
	{
		LOG_MESSAGE(1, "No entry selected");
		return;
	}
	for (size_t a = 0; a < selection.size(); ++a)
	{
		if (selection[a]->getType()->getFormat() == "img_png")
			fixpngsrc(selection[a]);
	}
}
