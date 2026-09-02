
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
#include "General/Misc.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "PatchTableList.h"
#include "TextureEditor/TextureEditor.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/Canvas/GfxCanvasBase.h"
#include "UI/Layout.h"
#include "UI/SAuiToolBar.h"
#include "Utility/PropertyList.h"

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
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Toolbar
	toolbar_ = new SAuiToolBar(this, true);
	toolbar_->loadLayoutFromResource("texturex_patch_table");
	sizer->Add(toolbar_, lh.sfWithSmallBorder(0, wxRIGHT).Expand());

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, wxSizerFlags(1).Expand());

	// List
	patch_list_ = new PatchTableList(this, editor_->patchTable());
	patch_list_->EnableDragSource(wxDF_UNICODETEXT);
	vbox->Add(patch_list_, lh.sfWithBorder(1, wxBOTTOM).Expand());

	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, wxSizerFlags(0).Expand());

	// Patch preview
	auto preview_size = FromDIP(96);
	preview_          = new GfxCanvas(this);
	preview_->SetWindowStyleFlag(wxBORDER_SIMPLE);
	preview_->SetInitialSize(wxSize(preview_size, preview_size));
	preview_->SetMinSize(wxSize(preview_size, preview_size));
	preview_->SetMaxSize(wxSize(preview_size, preview_size));
	preview_->setViewType(GfxView::Centered);
	preview_->allowDrag(false);
	preview_->allowScroll(false);
	hbox->Add(preview_, lh.sfWithSmallBorder(0, wxTOP));
	hbox->AddSpacer(lh.pad());

	// Patch info
	info_text_ = new wxTextCtrl(
		this, wxID_ANY, wxS(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	hbox->Add(info_text_, lh.sfWithSmallBorder(1, wxTOP).Expand());


	// Bind Events
	patch_list_->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &PatchTablePanel::onPatchTableSelectionChanged, this);
	patch_list_->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, &PatchTablePanel::onPatchTableBeginDrag, this);
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
		info_text_->SetValue(wxString());
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
		info += fmt::format("Size: {} x {}\n", preview_->image().width(), preview_->image().height());
	}
	else
	{
		preview_->image().clear();
		info += "Size: ? x ?\n";
	}
	preview_->resetViewOffsets();
	preview_->window()->Refresh();

	// List which textures use this patch
	if (!patch.used_in.empty())
	{
		info += "Used in: ";
		int    count = 0;
		string previous;
		for (const auto& tex_name : patch.used_in)
		{
			// Same texture as previous use, just increment the count
			if (strutil::equalCI(tex_name, previous))
			{
				++count;
				continue;
			}

			if (!previous.empty())
				info += count > 0 ? fmt::format(" (x{}), ", count + 1) : ", ";

			info += tex_name;
			previous = tex_name;
			count    = 0;
		}
		if (count > 0)
			info += fmt::format(" (x{})", count + 1);
	}
	else
		info += "\nNot used in any textures";

	info_text_->SetValue(wxString::FromUTF8(info));
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
