
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PatchTablePanel.cpp
// Description: UI Panel that displays the list of patches in a patch table,
//              along with a preview and info about the selected patch
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
#include "PatchTablePanel.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Misc.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/SImage/SImage.h"
#include "Lists/PatchTableList.h"
#include "MainEditor/MainEditor.h"
#include "TextureEditor/TextureEditor.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/Canvas/GfxCanvasBase.h"
#include "UI/Layout.h"
#include "UI/SAuiToolBar.h"
#include "Utility/PropertyList.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace texeditor;


namespace
{
// -----------------------------------------------------------------------------
// PatchDragSource Class
//
// Drop source for dragging a patch out of the patch table list, need to do this
// to avoid the default wxDropSource behaviour of drawing a drag image of the
// dragged list item (as it makes no sense when dragging on to the canvas -
// the canvas drop target overlay shows the drop position of the patch instead)
// -----------------------------------------------------------------------------
class PatchDragSource : public wxDropSource
{
public:
	explicit PatchDragSource(wxWindow* win) :
#ifdef __WXGTK__
		wxDropSource(win)
#else
		wxDropSource(win, wxCursor(wxCURSOR_SIZING), wxCursor(wxCURSOR_SIZING), wxCursor(wxCURSOR_NO_ENTRY))
#endif
	{
	}
};
} // namespace


// -----------------------------------------------------------------------------
//
// PatchTablePanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchTablePanel class constructor
// -----------------------------------------------------------------------------
PatchTablePanel::PatchTablePanel(wxWindow* parent, TextureEditor& editor) : wxPanel(parent), editor_(&editor)
{
	auto lh    = ui::LayoutHelper(this);
	auto sizer = new wxGridBagSizer(lh.padSmall(), lh.padSmall());
	SetSizer(sizer);

	// Toolbar
	toolbar_ = new SAuiToolBar(this, true);
	toolbar_->loadLayoutFromResource("texturex_patch_table");
	sizer->Add(toolbar_, { 0, 0 }, { 3, 1 });

	// List
	patch_list_ = new PatchTableList(this, editor_->patchTable());
	patch_list_->EnableDragSource(wxDF_UNICODETEXT);
	sizer->Add(patch_list_, { 0, 1 }, { 1, 2 }, wxEXPAND);

	// Info label
	info_text_ = new wxStaticText(this, wxID_ANY, wxS("No patch selected"));
	sizer->Add(info_text_, { 1, 1 }, { 1, 1 }, wxALIGN_BOTTOM | wxTOP, lh.padSmall());

	// Patch preview
	preview_ = new GfxCanvas(this);
	preview_->SetWindowStyleFlag(wxBORDER_SIMPLE);
	preview_->SetInitialSize(wxSize(-1, FromDIP(144)));
	preview_->SetMinSize(wxSize(-1, FromDIP(144)));
	preview_->SetMaxSize(wxSize(-1, FromDIP(144)));
	preview_->setViewType(GfxView::Centered);
	preview_->allowDrag(false);
	preview_->allowScroll(false);
	sizer->Add(preview_, { 2, 1 }, { 1, 1 }, wxEXPAND);

	// "In Textures" List
	sizer->Add(
		new wxStaticText(this, wxID_ANY, wxS("Used in:")), { 1, 2 }, { 1, 1 }, wxALIGN_BOTTOM | wxTOP, lh.padSmall());
	list_in_textures_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(100), -1));
	sizer->Add(list_in_textures_, { 2, 2 }, { 1, 1 }, wxEXPAND);

	sizer->AddGrowableCol(1);
	sizer->AddGrowableRow(0);


	// Bind Events
	patch_list_->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &PatchTablePanel::onPatchTableSelectionChanged, this);
	patch_list_->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, &PatchTablePanel::onPatchTableBeginDrag, this);

	editor_->patchTable()->signals().modified.connect([this] { patch_list_->GetModel()->Cleared(); });
}

// -----------------------------------------------------------------------------
// Updates the patch preview and info text for the currently selected patch
// -----------------------------------------------------------------------------
void PatchTablePanel::updatePatchTablePreview() const
{
	auto index = patch_list_->selectedPatchIndex();

	if (index < 0)
	{
		preview_->image().clear();
		info_text_->SetLabel(wxS("No patch selected"));
		preview_->window()->Refresh();
		return;
	}

	auto& patch_table = *editor_->patchTable();
	auto& patch       = patch_table.patch(index);
	auto  entry       = patch_table.patchEntry(index);

	// Load patch image
	string info;
	if (entry && misc::loadImageFromEntry(&preview_->image(), entry))
	{
		preview_->setPalette(maineditor::currentPalette());
		preview_->zoomToFit();
		info += fmt::format("{} ({} x {})", patch.name, preview_->image().width(), preview_->image().height());
	}
	else
	{
		preview_->image().clear();
		info += fmt::format("{} (Unknown size)", patch.name);
	}
	preview_->resetViewOffsets();
	preview_->window()->Refresh();

	// Update info text
	info_text_->SetLabel(wxString::FromUTF8(info));

	// Update "Used in" list
	list_in_textures_->Clear();
	if (!patch.used_in.empty())
	{
		string last = patch.used_in[0];
		for (auto i = 0; i < patch.used_in.size(); ++i)
		{
			if (i > 0 && strutil::equalCI(patch.used_in[i], last))
				continue;

			list_in_textures_->Append(wxString::FromUTF8(patch.used_in[i]));
			last = patch.used_in[i];
		}
	}
}

// -----------------------------------------------------------------------------
// Handles the SAction [id]. Returns true if handled
// -----------------------------------------------------------------------------
bool PatchTablePanel::handleAction(string_view id)
{
	// Don't handle actions if hidden
	if (!IsShown())
		return false;

	// Patch actions
	if (id == "txed_pnames_add")
		addPatch();
	else if (id == "txed_pnames_addfile")
		addPatchFromFile();
	else if (id == "txed_pnames_delete")
		removePatch();
	else if (id == "txed_pnames_change")
		changePatch();

	// Unknown action
	else
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Called when the 'New Patch' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::addPatch()
{
	// Prompt for new patch name
	auto patch = wxGetTextFromUser(wxS("Enter patch entry name:"), wxS("Add Patch"), wxEmptyString, this);

	// Check something was entered
	if (patch.IsEmpty())
		return;

	// Add to patch table
	if (int index = editor_->addPatchToTable(patch.Upper().utf8_string()); index != -1)
	{
		patch_list_->UnselectAll();
		patch_list_->selectPatch(index, true, true); // Select the newly added patch in the list
	}
}

// -----------------------------------------------------------------------------
// Called when the 'New Patch from File' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::addPatchFromFile()
{
	// Get all entry types
	auto etypes = EntryType::allTypes();

	// Go through types
	string ext_filter = "All files (*.*)|*|";
	for (auto& etype : etypes)
	{
		// If the type is a valid image type, add its extension filter
		if (etype->extraProps().contains("image"))
		{
			ext_filter += etype->fileFilterString();
			ext_filter += "|";
		}
	}
	if (strutil::endsWith(ext_filter, "|"))
		ext_filter.pop_back();

	// Popup open file dialog that filters by valid image types
	auto fd_info = filedialog::openFiles("Choose file(s) to open", ext_filter, this);

	// Check that the user didn't cancel
	if (!fd_info.filenames.empty())
	{
		// Go through file selection
		patch_list_->UnselectAll();
		for (const auto& file : fd_info.filenames)
		{
			// Load the file into a temporary ArchiveEntry
			auto entry = std::make_shared<ArchiveEntry>();
			entry->importFile(file);

			// Determine type
			EntryType::detectEntryType(*entry);

			// If it's not a valid image type, ignore this file
			if (!entry->type()->extraProps().contains("image"))
			{
				log::warning("{} is not a valid image file", file);
				continue;
			}

			// Ask for name for patch
			wxFileName fn(wxString::FromUTF8(file));
			auto       name = fn.GetName().Upper().Truncate(8);
			name            = wxGetTextFromUser(
                WX_FMT("Enter a patch name for {}:", fn.GetFullName().utf8_string()), wxS("New Patch"), name);
			name = name.Truncate(8);

			// Add patch to archive
			entry->setName(name.utf8_string());
			entry->setExtensionByType();
			editor_->archive()->addEntry(entry, "patches");

			// Add patch to patch table
			if (auto index = editor_->addPatchToTable(name.utf8_string()); index != -1)
				patch_list_->selectPatch(index, true, true); // Select the newly added patch in the list
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove Patch' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::removePatch()
{
	// Check anything is selected
	auto selection = patch_list_->selectedPatchIndices();
	if (selection.empty())
		return;

	// TODO: Yes(to All) + No(to All) messagebox asking to delete entries along with patches

	// Go through patch list selection
	auto patch_table = editor_->patchTable();
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Check if patch is currently in use
		auto& patch = patch_table->patch(selection[a]);
		if (!patch.used_in.empty())
		{
			// In use, ask if it's ok to remove the patch
			int answer = wxMessageBox(
				WX_FMT(
					"The patch \"{}\" is currently used by {} texture(s), are you sure you wish to remove it?",
					patch.name,
					patch.used_in.size()),
				wxS("Confirm Remove Patch"),
				wxYES_NO | wxCANCEL | wxICON_QUESTION,
				this);
			if (answer == wxYES)
			{
				// Answered yes, remove the patch
				editor_->removePatchFromTable(selection[a]);
			}
		}
		else
		{
			// Not in use, just delete it
			editor_->removePatchFromTable(selection[a]);
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Change Patch' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::changePatch()
{
	// Check anything is selected
	auto selection = patch_list_->selectedPatchIndices();
	if (selection.empty())
		return;

	// Go through patch list selection
	auto patch_table = editor_->patchTable();
	for (auto index : selection)
	{
		auto& patch = patch_table->patch(index);

		// Prompt for new patch name
		auto newname = wxGetTextFromUser(
						   wxS("Enter new patch entry name:"),
						   wxS("Change Patch"),
						   wxString::FromUTF8(patch.name),
						   this)
						   .Upper()
						   .utf8_string();

		// Update the patch if it's not the Cancel button that was clicked
		if (!newname.empty())
			editor_->replacePatchInTable(index, newname);
	}
}

// -----------------------------------------------------------------------------
// Called when the selection in the patch table changes
// -----------------------------------------------------------------------------
void PatchTablePanel::onPatchTableSelectionChanged(wxDataViewEvent& e)
{
	updatePatchTablePreview();
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a drag begins on the patch table list. The default drag is
// vetoed and performed manually instead, to suppress the default drag cursor/
// rectangle (the canvas drop target shows its own overlay for the patch)
// -----------------------------------------------------------------------------
void PatchTablePanel::onPatchTableBeginDrag(wxDataViewEvent& e)
{
	auto index = patch_list_->selectedPatchIndex();
	if (index < 0)
	{
		e.Veto();
		return;
	}

	dragging_patch_ = editor_->patchTable()->patchName(index);

#ifdef __WXGTK__
	e.SetDataObject(new wxTextDataObject(wxString::FromUTF8(dragging_patch_)));
	e.Allow();
#else
	e.Veto();

	wxTextDataObject data(wxString::FromUTF8(dragging_patch_));
	PatchDragSource  drag_source(patch_list_);
	drag_source.SetData(data);
	drag_source.DoDragDrop(wxDrag_CopyOnly);
#endif
}
