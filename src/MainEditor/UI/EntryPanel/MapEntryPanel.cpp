
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "UI/Canvas/MapPreviewCanvas.h"
#include "UI/WxUtils.h"

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
MapEntryPanel::MapEntryPanel(wxWindow* parent) : EntryPanel(parent, "map")
{
	// Setup map canvas
	map_canvas_ = new MapPreviewCanvas(this);
	sizer_main_->Add(map_canvas_, 1, wxEXPAND, 0);

	// Setup map toolbar buttons
	auto group = new SToolBarGroup(toolbar_, "Map");
	group->addActionButton("save_image", "Save Map Image", "export", "Save map overview to an image", true);
	group->addActionButton("pmap_open_text", "", true);
	toolbar_->addGroup(group);

	// Remove save/revert buttons
	toolbar_->deleteGroup("Entry");
	stb_save_   = nullptr;
	stb_revert_ = nullptr;

	// Setup bottom panel
	sizer_bottom_->Add(label_stats_ = new wxStaticText(this, -1, ""), 0, wxALIGN_CENTER_VERTICAL);
	sizer_bottom_->AddStretchSpacer();
	sizer_bottom_->Add(cb_show_things_ = new wxCheckBox(this, -1, "Show Things"), 0, wxALIGN_CENTER_VERTICAL);
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
	map_canvas_->clearMap();

	// Find map definition for entry
	auto             maps = entry->parent()->detectMaps();
	Archive::MapDesc thismap;
	bool             found = false;
	for (auto& map : maps)
	{
		if (map.head.lock().get() == entry)
		{
			thismap = map;
			found   = true;
			break;
		}
	}

	// All errors = invalid map
	global::error = "Invalid map";

	// There is no map entry for the map marker.
	// This may happen if a map marker lump is copy/pasted without the rest of the map lumps.
	if (!found)
	{
		entry->setType(EntryType::unknownType());
		EntryType::detectEntryType(*entry);
		return false;
	}

	// Load map into preview canvas
	if (map_canvas_->openMap(thismap))
	{
		label_stats_->SetLabel(wxString::Format(
			"Vertices: %d, Sides: %d, Lines: %d, Sectors: %d, Things: %d, Total Size: %dx%d",
			map_canvas_->nVertices(),
			map_canvas_->nSides(),
			map_canvas_->nLines(),
			map_canvas_->nSectors(),
			map_canvas_->nThings(),
			map_canvas_->width(),
			map_canvas_->height()));
		return true;
	}

	label_stats_->SetLabel("");
	return false;
}

// -----------------------------------------------------------------------------
// Creates and saves a PNG image of the map preview
// -----------------------------------------------------------------------------
bool MapEntryPanel::createImage()
{
	auto entry = entry_.lock();
	if (!entry)
		return false;

	ArchiveEntry temp;

	// Stupid OpenGL grumble grumble grumble
	if (gl::fboSupport())
		map_canvas_->createImage(temp, map_image_width, map_image_height);
	else
		map_canvas_->createImage(
			temp,
			min<int>(map_image_width, map_canvas_->GetSize().x),
			min<int>(map_image_height, map_canvas_->GetSize().y));

	wxString   name = wxString::Format("%s_%s", entry->parent()->filename(false), entry->name());
	wxFileName fn(name);

	// Create save file dialog
	wxFileDialog dialog_save(
		this,
		wxString::Format("Save Map Preview \"%s\"", name),
		dir_last,
		fn.GetFullName(),
		"PNG (*.PNG)|*.png",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_save.ShowModal() == wxID_OK)
	{
		// If a filename was selected, export it
		bool ret = temp.exportFile(dialog_save.GetPath().ToStdString());

		// Save 'dir_last'
		dir_last = wxutil::strToView(dialog_save.GetDirectory());

		// Open the saved image
		wxLaunchDefaultApplication(dialog_save.GetPath());

		return ret;
	}
	return true;
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
