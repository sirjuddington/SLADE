
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    MapBackupPanel.cpp
// Description: User interface for selecting a map backup to restore
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
#include "MapBackupPanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormatHandler.h"
#include "General/MapPreviewData.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Layout.h"
#include "UI/Lists/ListView.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// MapBackupPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapBackupPanel class constructor
// -----------------------------------------------------------------------------
MapBackupPanel::MapBackupPanel(wxWindow* parent) :
	wxPanel{ parent, -1 },
	map_data_{ new MapPreviewData },
	archive_backups_{ new Archive(ArchiveFormat::Zip) }
{
	// Setup Sizer
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Backups list
	sizer->Add(list_backups_ = new ListView(this, -1), ui::LayoutHelper(this).sfWithBorder(0, wxRIGHT).Expand());

	// Map preview
	sizer->Add(canvas_map_ = ui::createMapPreviewCanvas(this, map_data_.get()), 1, wxEXPAND);

	// Bind events
	list_backups_->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent&) { updateMapPreview(); });

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Opens the map backup file for [map_name] in [archive_name] and populates the
// list
// -----------------------------------------------------------------------------
bool MapBackupPanel::loadBackups(string archive_name, string_view map_name)
{
	// Open backup file
	strutil::replaceIP(archive_name, ".", "_");
	auto backup_file = app::path("backups", app::Dir::User) + "/" + archive_name + "_backup.zip";
	if (!archive_backups_->open(backup_file))
		return false;

	// Get backup dir for map
	dir_current_ = archive_backups_->dirAtPath(map_name);
	if (dir_current_ == archive_backups_->rootDir().get() || !dir_current_)
		return false;

	// Populate backups list
	list_backups_->ClearAll();
	list_backups_->AppendColumn(wxS("Backup Date"));
	list_backups_->AppendColumn(wxS("Time"));

	int index = 0;
	for (int a = dir_current_->numSubdirs() - 1; a >= 0; a--)
	{
		auto           timestamp = dir_current_->subdirAt(a)->name();
		vector<string> cols;

		// Date
		cols.push_back(strutil::beforeFirst(timestamp, '_'));

		// Time
		auto time = strutil::afterFirst(timestamp, '_');
		std::replace(time.begin(), time.end(), '.', ':');
		cols.push_back(time);

		// Add to list
		list_backups_->addItem(index++, cols);
	}

	if (list_backups_->GetItemCount() > 0)
		list_backups_->selectItem(0);

	return true;
}

// -----------------------------------------------------------------------------
// Updates the map preview with the currently selected backup
// -----------------------------------------------------------------------------
void MapBackupPanel::updateMapPreview()
{
	// Clear current preview
	map_data_->clear();

	// Check for selection
	if (list_backups_->selectedItems().IsEmpty())
		return;
	int selection = (list_backups_->GetItemCount() - 1) - list_backups_->selectedItems()[0];

	// Load map data to temporary wad
	archive_mapdata_ = std::make_unique<Archive>(ArchiveFormat::Wad);
	auto dir         = dir_current_->subdirAt(selection);
	for (unsigned a = 0; a < dir->numEntries(); a++)
		archive_mapdata_->addEntry(std::make_shared<ArchiveEntry>(*dir->entryAt(a)), "");

	// Open map preview
	auto maps = archive_mapdata_->detectMaps();
	if (!maps.empty())
		map_data_->openMap(maps[0]);
}
