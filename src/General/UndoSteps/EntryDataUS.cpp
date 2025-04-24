
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    EntryDataUS.cpp
// Description: UndoStep for undo/redo of ArchiveEntry data modification
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
#include "EntryDataUS.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// EntryDataUS Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// EntryDataUS class constructor
// -----------------------------------------------------------------------------
EntryDataUS::EntryDataUS(ArchiveEntry* entry) :
	path_{ entry->path() },
	index_{ entry->index() },
	archive_{ entry->parent() }
{
	data_.importMem(entry->rawData(), entry->size());
}

// -----------------------------------------------------------------------------
// Swaps data between the entry and the undo step
// -----------------------------------------------------------------------------
bool EntryDataUS::swapData()
{
	// log::info(1, "Entry data swap...");

	// Get parent dir
	auto dir = archive_->dirAtPath(path_);
	if (dir)
	{
		// Get entry
		auto entry = dir->entryAt(index_);
		if (!entry)
			return false;

		// Backup data
		MemChunk temp_data;
		temp_data.importMem(entry->rawData(), entry->size());
		// log::info(1, "Backup current data, size %d", entry->getSize());

		// Restore entry data
		if (data_.size() == 0)
		{
			entry->clearData();
			// log::info(1, "Clear entry data");
		}
		else
		{
			entry->importMemChunk(data_);
			// log::info(1, "Restored entry data, size %d", data.getSize());
		}

		// Store previous entry data
		if (temp_data.size() > 0)
			data_.importMem(temp_data.data(), temp_data.size());
		else
			data_.clear();

		return true;
	}

	return false;
}
