
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2015 Alexey Lysiuk
 *
 * Email:       alexey.lysiuk@gmail.com
 * Filename:    ChasmBinArchive.h
 * Description: ChasmBinArchive, archive class to handle
 *              Chasm: The Rift bin file format
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

#ifndef __CHASM_BIN_ARCHIVE_H__
#define __CHASM_BIN_ARCHIVE_H__

#include "Archive/Archive.h"

class ChasmBinArchive : public Archive
{
public:
	ChasmBinArchive();

	// Archive type info
	string getFileExtensionString();
	string getFormat();

	// Opening/writing
	bool open(MemChunk& mc);
	bool write(MemChunk& mc, bool update = true);

	// Misc
	bool loadEntryData(ArchiveEntry* entry);

	// Detection
	virtual vector<mapdesc_t> detectMaps() { return vector<mapdesc_t>(); }

	// Static functions
	static bool isChasmBinArchive(MemChunk& mc);
	static bool isChasmBinArchive(string filename);
};

#endif // __CHASM_BIN_ARCHIVE_H__
