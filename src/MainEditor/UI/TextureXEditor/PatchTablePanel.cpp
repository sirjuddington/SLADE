
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PatchTablePanel.cpp
// Description: The UI for viewing/editing a patch table (PNAMES)
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
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Misc.h"
#include "General/Sigslot.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "TextureXEditor.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Canvas/GfxCanvasBase.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Controls/ZoomControl.h"
#include "UI/Layout.h"
#include "UI/Lists/VirtualListView.h"
#include "UI/SAuiToolBar.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)


// -----------------------------------------------------------------------------
//
// PatchTableListView Class
//
// -----------------------------------------------------------------------------
namespace slade
{
class PatchTableListView : public VirtualListView
{
public:
	PatchTableListView(wxWindow* parent, PatchTable* patch_table) : VirtualListView(parent), patch_table_{ patch_table }
	{
		// Add columns
		InsertColumn(0, wxS("#"));
		InsertColumn(1, wxS("Patch Name"));
		InsertColumn(2, wxS("Use Count"));
		InsertColumn(3, wxS("In Archive"));

		// Update list
		PatchTableListView::updateList();

		// Update the list when an archive is added/closed/modified or the patch table is modified
		auto& am_signals = app::archiveManager().signals();
		signal_connections_ += am_signals.archive_added.connect([this](unsigned) { updateList(); });
		signal_connections_ += am_signals.archive_closed.connect([this](unsigned) { updateList(); });
		signal_connections_ += am_signals.archive_modified.connect([this](unsigned, bool) { updateList(); });
		signal_connections_ += patch_table_->signals().modified.connect([this]() { updateList(); });
	}

	~PatchTableListView() override = default;

	PatchTable* patchTable() const { return patch_table_; }

	// Updates + refreshes the patch list
	void updateList(bool clear = false) override
	{
		if (clear)
			ClearAll();

		// Set list size
		items_.clear();
		if (patch_table_)
		{
			size_t count = patch_table_->nPatches();
			SetItemCount(count);
			for (unsigned a = 0; a < count; a++)
				items_.push_back(a);
		}
		else
			SetItemCount(0);

		sortItems();
		updateWidth();
		Refresh();
	}

	// Sorts the list items depending on the current sorting column
	void sortItems() override
	{
		lv_current_ = this;
		if (sort_column_ == 2)
			std::sort(items_.begin(), items_.end(), &PatchTableListView::usageSort);
		else
			std::sort(items_.begin(), items_.end(), &VirtualListView::defaultSort);
	}

	// Returns true if patch at index [left] is used less than [right]
	static bool usageSort(long left, long right)
	{
		auto& p1 = dynamic_cast<PatchTableListView*>(lv_current_)->patchTable()->patch(left);
		auto& p2 = dynamic_cast<PatchTableListView*>(lv_current_)->patchTable()->patch(right);

		if (p1.used_in.size() == p2.used_in.size())
			return left < right;
		else
			return lv_current_->sortDescend() ? p2.used_in.size() < p1.used_in.size()
											  : p1.used_in.size() < p2.used_in.size();
	}

protected:
	// Returns the string for [item] at [column]
	string itemText(long item, long column, long index) const override
	{
		// Check patch table exists
		if (!patch_table_)
			return "INVALID INDEX";

		// Check index is ok
		if (index < 0 || static_cast<unsigned>(index) > patch_table_->nPatches())
			return "INVALID INDEX";

		// Get associated patch
		auto& patch = patch_table_->patch(index);

		if (column == 0) // Index column
			return fmt::format("{:04d}", index);
		else if (column == 1) // Name column
			return patch.name;
		else if (column == 2) // Usage count column
			return fmt::format("{}", static_cast<unsigned>(patch.used_in.size()));
		else if (column == 3) // Archive column
		{
			// Get patch entry
			auto entry = patch_table_->patchEntry(index);

			// If patch entry can't be found return invalid
			if (entry)
				return entry->parent()->filename(false);
			else
				return "(!) NOT FOUND";
		}
		else // Invalid column
			return "INVALID COLUMN";
	}

	// Updates the item attributes for [item]
	void updateItemAttr(long item, long column, long index) const override
	{
		// Just set normal text colour
		item_attr_->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
	}

private:
	PatchTable*          patch_table_ = nullptr;
	ScopedConnectionList signal_connections_;
};
} // namespace slade


// -----------------------------------------------------------------------------
//
// PatchTablePanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PatchTablePanel class constructor
// -----------------------------------------------------------------------------
PatchTablePanel::PatchTablePanel(wxWindow* parent, PatchTable* patch_table, TextureXEditor* tx_editor) :
	wxPanel(parent, -1),
	patch_table_{ patch_table },
	parent_{ tx_editor }
{
	setupLayout();

	// Bind events
	list_patches_->Bind(wxEVT_LIST_ITEM_SELECTED, &PatchTablePanel::onDisplayChanged, this);

	// Update when main palette changed
	sc_palette_changed_ = theMainWindow->paletteChooser()->signals().palette_changed.connect([this]
																							 { updateDisplay(); });
}

// -----------------------------------------------------------------------------
// Lays out controls on the panel
// -----------------------------------------------------------------------------
void PatchTablePanel::setupLayout()
{
	auto lh = ui::LayoutHelper(this);

	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Patches List + actions
	auto frame    = new wxStaticBox(this, -1, wxS("Patch List (PNAMES)"));
	list_patches_ = new PatchTableListView(frame, patch_table_);
	list_patches_->setSearchColumn(1); // Want to search by patch name not index
	toolbar_ = new SAuiToolBar(frame, true);
	toolbar_->loadLayoutFromResource("texturex_patch_table");
	// toolbar_ = new SToolBar(frame, false, wxVERTICAL);
	// toolbar_->addActionGroup(
	// 	"_New", { "txed_pnames_add", "txed_pnames_addfile", "txed_pnames_delete", "txed_pnames_change" });
	auto framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	sizer->Add(framesizer, lh.sfWithBorder().Expand());
	framesizer->Add(toolbar_, lh.sfWithSmallBorder(0, wxTOP | wxBOTTOM).Expand());
	framesizer->AddSpacer(lh.padSmall());
	framesizer->Add(list_patches_, lh.sfWithBorder(1, wxTOP | wxRIGHT | wxBOTTOM).Expand());

	// Patch preview & info
	frame             = new wxStaticBox(this, -1, wxS("Patch Preview && Info"));
	label_dimensions_ = new wxStaticText(frame, -1, wxS("Size: N/A"));
	label_textures_   = new wxStaticText(
        frame, -1, wxS("In Textures: -"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
	patch_canvas_ = ui::createGfxCanvas(frame);
	patch_canvas_->setViewType(GfxView::Centered);
	patch_canvas_->allowDrag(true);
	patch_canvas_->allowScroll(true);
	zc_zoom_   = new ui::ZoomControl(frame, patch_canvas_);
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, lh.sfWithBorder(1, wxTOP | wxRIGHT | wxBOTTOM).Expand());
	framesizer->Add(zc_zoom_, lh.sfWithBorder());
	framesizer->Add(patch_canvas_->window(), lh.sfWithBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	framesizer->Add(label_dimensions_, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	framesizer->Add(label_textures_, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
}

// -----------------------------------------------------------------------------
// Called when a different patch or palette is selected
// -----------------------------------------------------------------------------
void PatchTablePanel::updateDisplay()
{
	// TODO: Separate palette changed and patch changed without breaking default
	// palette display; optimize label_textures display

	// Get selected patch
	int   index = list_patches_->itemIndex(list_patches_->lastSelected());
	auto& patch = patch_table_->patch(index);

	// Load the image
	auto entry = patch_table_->patchEntry(index);
	if (misc::loadImageFromEntry(&patch_canvas_->image(), entry))
	{
		theMainWindow->paletteChooser()->setGlobalFromArchive(entry->parent());
		patch_canvas_->setPalette(theMainWindow->paletteChooser()->selectedPalette());
		label_dimensions_->SetLabel(
			WX_FMT("Size: {} x {}", patch_canvas_->image().width(), patch_canvas_->image().height()));
	}
	else
	{
		patch_canvas_->image().clear();
		label_dimensions_->SetLabel(wxS("Size: ? x ?"));
	}
	patch_canvas_->resetViewOffsets();
	patch_canvas_->window()->Refresh();

	// List which textures use this patch
	if (!patch.used_in.empty())
	{
		string alltextures;
		int    count = 0;
		string previous;
		for (size_t a = 0; a < patch.used_in.size(); ++a)
		{
			string current = patch.used_in[a];

			// Is the use repeated for the same texture?
			if (!strutil::equalCI(current, previous))
			{
				count++;
				// Else it's a new texture
			}
			else
			{
				// First add the count to the previous texture if needed
				if (count)
				{
					alltextures += fmt::format(" ({})", count + 1);
					count = 0;
				}

				// Add a separator if appropriate
				if (a > 0)
					alltextures += ';';

				// Then print the new texture's name
				alltextures += fmt::format(" {}", patch.used_in[a]);

				// And set it for comparison with the next one
				previous = current;
			}
		}
		// If count is still non-zero, it's because the patch was repeated in the last texture
		if (count)
			alltextures += fmt::format(" ({})", count + 1);

		// Finally display the listing
		label_textures_->SetLabel(WX_FMT("In Textures:{}", alltextures));
	}
	else
		label_textures_->SetLabel(wxS("In Textures: -"));

	// Wrap the text label
	label_textures_->Wrap(label_textures_->GetSize().GetWidth());

	// Update layout
	Layout();
}

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
	patch_table_->addPatch(patch.utf8_string());

	// Update list
	list_patches_->updateList();
	parent_->pnamesModified(true);
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

	// Create open file dialog
	wxFileDialog dialog_open(
		this,
		wxS("Choose file(s) to open"),
		dir_last,
		wxEmptyString,
		wxString::FromUTF8(ext_filter),
		wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get file selection
		wxArrayString files;
		dialog_open.GetPaths(files);

		// Save 'dir_last'
		dir_last = dialog_open.GetDirectory().utf8_string();

		// Go through file selection
		for (const auto& file : files)
		{
			// Load the file into a temporary ArchiveEntry
			auto entry = std::make_shared<ArchiveEntry>();
			entry->importFile(file.utf8_string());

			// Determine type
			EntryType::detectEntryType(*entry);

			// If it's not a valid image type, ignore this file
			if (!entry->type()->extraProps().contains("image"))
			{
				log::warning("%s is not a valid image file", file.utf8_string());
				continue;
			}

			// Ask for name for patch
			wxFileName fn(file);
			auto       name = fn.GetName().Upper().Truncate(8);
			name            = wxGetTextFromUser(
                WX_FMT("Enter a patch name for {}:", fn.GetFullName().utf8_string()), wxS("New Patch"), name);
			name = name.Truncate(8);

			// Add patch to archive
			entry->setName(name.utf8_string());
			entry->setExtensionByType();
			parent_->archive()->addEntry(entry, "patches");

			// Add patch to patch table
			patch_table_->addPatch(name.utf8_string());
		}

		// Refresh patch list
		list_patches_->updateList();
		parent_->pnamesModified(true);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Remove Patch' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::removePatch()
{
	// Check anything is selected
	auto selection = list_patches_->selection(true);
	if (selection.empty())
		return;

	// TODO: Yes(to All) + No(to All) messagebox asking to delete entries along with patches

	// Go through patch list selection
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Check if patch is currently in use
		auto& patch = patch_table_->patch(selection[a]);
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
				parent_->removePatch(selection[a]);

				// Deselect it
				list_patches_->selectItem(selection[a], false);
			}
		}
		else
		{
			// Not in use, just delete it
			parent_->removePatch(selection[a]);

			// Deselect it
			list_patches_->selectItem(selection[a], false);
		}
	}

	// Update list
	list_patches_->updateList();
	parent_->pnamesModified(true);
}

// -----------------------------------------------------------------------------
// Called when the 'Change Patch' button is clicked
// -----------------------------------------------------------------------------
void PatchTablePanel::changePatch()
{
	// Check anything is selected
	auto selection = list_patches_->selection(true);
	if (selection.empty())
		return;

	// Go through patch list selection
	for (auto index : selection)
	{
		auto& patch = patch_table_->patch(index);

		// Prompt for new patch name
		auto newname = wxGetTextFromUser(
						   wxS("Enter new patch entry name:"),
						   wxS("Change Patch"),
						   wxString::FromUTF8(patch.name),
						   this)
						   .utf8_string();

		// Update the patch if it's not the Cancel button that was clicked
		if (!newname.empty())
			patch_table_->replacePatch(index, newname);

		// Update the list
		list_patches_->updateList();
		parent_->pnamesModified(true);
	}
}


// -----------------------------------------------------------------------------
//
// PatchTablePanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a different patch or palette is selected
// -----------------------------------------------------------------------------
void PatchTablePanel::onDisplayChanged(wxCommandEvent& e)
{
	// TODO: Separate palette changed and patch changed without breaking
	// default palette display; optimize label_textures display
	updateDisplay();
}
