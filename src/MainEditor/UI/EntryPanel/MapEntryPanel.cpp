
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapEntryPanel.cpp
// Description: MapEntryPanel class. Shows a basic (lines-only) preview of a
//              map entry
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
#include "MapEntryPanel.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/EntryType/EntryType.h"
#include "Archive/MapDesc.h"
#include "General/MapPreviewData.h"
#include "UI/Canvas/Canvas.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, map_image_width, -5, CVar::Flag::Save)
CVAR(Int, map_image_height, -5, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)
EXTERN_CVAR(Bool, map_view_things)


// -----------------------------------------------------------------------------
//
// MapEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapEntryPanel class constructor
// -----------------------------------------------------------------------------
MapEntryPanel::MapEntryPanel(wxWindow* parent) : EntryPanel(parent, "map"), map_data_{ new MapPreviewData }
{
	// Setup map canvas
	map_canvas_ = ui::createMapPreviewCanvas(this, map_data_.get(), true, true);
	sizer_main_->Add(map_canvas_, wxSizerFlags(1).Expand());

	// Setup map toolbar buttons
	auto group = new SToolBarGroup(toolbar_, "Map");
	group->addActionButton("save_image", "Save Map Image", "export", "Save map overview to an image", true);
	group->addActionButton("pmap_open_text", "", true);
	tbb_open_archive_ = group->addActionButton("pmap_open_archive", "", true);
	toolbar_->addGroup(group);

	// Remove save/revert buttons
	toolbar_->deleteGroup("Entry");
	stb_save_   = nullptr;
	stb_revert_ = nullptr;

	// Setup bottom panel
	sizer_bottom_->Add(label_stats_ = new wxStaticText(this, -1, ""), wxSizerFlags().CenterVertical());
	sizer_bottom_->AddStretchSpacer();
	sizer_bottom_->Add(cb_show_things_ = new wxCheckBox(this, -1, "Show Things"), wxSizerFlags().CenterVertical());
	cb_show_things_->SetValue(map_view_things);

	// Bind events
	cb_show_things_->Bind(wxEVT_CHECKBOX, &MapEntryPanel::onCBShowThings, this);

	// Layout
	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Loads [entry] into the EntryPanel.
// Returns false if the map was invalid, true otherwise
// -----------------------------------------------------------------------------
bool MapEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Clear current map data
	map_data_->clear();

	MapDesc thismap;
	bool    found = false;

	// Check for wad archive (map in zip)
	unique_ptr<Archive> tmp_archive;
	if (entry->type()->formatId() == "archive_wad")
	{
		tmp_archive = std::make_unique<Archive>(ArchiveFormat::Wad);
		tmp_archive->open(entry);
		auto maps = tmp_archive->detectMaps();
		if (!maps.empty())
		{
			thismap = maps[0];
			found   = true;
		}
		tbb_open_archive_->Show(true);
	}
	else // Normal map entry
	{
		// Find map definition for entry
		auto maps = entry->parent()->detectMaps();
		for (auto& map : maps)
		{
			if (map.head.lock().get() == entry)
			{
				thismap = map;
				found   = true;
				break;
			}
		}
		tbb_open_archive_->Show(false);
	}

	// All errors = invalid map
	global::error = "Invalid map";

	// There is no map entry for the map marker.
	// This may happen if a map marker lump is copy/pasted without the rest of the map lumps.
	if (!found)
	{
		entry->setType(EntryType::unknownType());
		EntryType::detectEntryType(*entry);
		map_canvas_->Refresh();
		return false;
	}

	// Load map into preview canvas
	if (map_data_->openMap(thismap))
	{
		label_stats_->SetLabel(wxString::Format(
			"Vertices: %d, Sides: %d, Lines: %d, Sectors: %d, Things: %d, Total Size: %dx%d",
			static_cast<int>(map_data_->vertices.size()),
			map_data_->n_sides,
			static_cast<int>(map_data_->lines.size()),
			map_data_->n_sectors,
			static_cast<int>(map_data_->things.size()),
			static_cast<int>(map_data_->bounds.width()),
			static_cast<int>(map_data_->bounds.height())));
		map_canvas_->Refresh();
		return true;
	}

	label_stats_->SetLabel("");
	map_canvas_->Refresh();
	return false;
}

// -----------------------------------------------------------------------------
// Creates and saves a PNG image of the map preview
// -----------------------------------------------------------------------------
bool MapEntryPanel::createImage()
{
	auto name = fmt::format("{}_{}", entry()->parent()->filename(false), entry()->name());

	// Popup file save dialog
	filedialog::FDInfo inf;
	if (filedialog::saveFile(inf, fmt::format("Save Map Preview \"{}\"", name), "PNG (*.png)|*.png", this, name))
	{
		// Save the map preview as a png image at the selected path
		if (createMapImage(*map_data_, inf.filenames[0], map_image_width, map_image_height))
		{
			// Open the saved image
			wxLaunchDefaultApplication(inf.filenames[0]);
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Called when a (EntryPanel) toolbar button is clicked
// -----------------------------------------------------------------------------
void MapEntryPanel::toolbarButtonClick(const wxString& action_id)
{
	// Save Map Image
	if (action_id == "save_image")
		createImage();
}


// -----------------------------------------------------------------------------
//
// MapEntryPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Show Things' checkbox is changed
// -----------------------------------------------------------------------------
void MapEntryPanel::onCBShowThings(wxCommandEvent& e)
{
	map_view_things = cb_show_things_->GetValue();
	map_canvas_->Refresh();
}
