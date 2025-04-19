
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextureEditorPanel.cpp
// Description: The UI for viewing/editing a composite texture
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
#include "TextureEditorPanel.h"
#include "General/KeyBind.h"
#include "Graphics/CTexture/TextureXList.h"
#include "TextureXEditor.h"
#include "UI/Canvas/CTextureCanvas.h"
#include "UI/Controls/ZoomControl.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
// Hack to stop the drag event being erroneously triggered when
// double-clicking a patch in the patch browser to select it
bool hack_nodrag = false;
} // namespace
CVAR(Bool, tx_apply_scale, true, CVar::Flag::Save)
CVAR(Bool, tx_show_outside, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, tx_arc)


// -----------------------------------------------------------------------------
//
// TextureEditorPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureEditorPanel class constructor
// -----------------------------------------------------------------------------
TextureEditorPanel::TextureEditorPanel(wxWindow* parent, TextureXEditor* tx_editor) :
	wxPanel(parent, -1),
	tx_editor_{ tx_editor }
{
	// Create controls
	tex_canvas_      = new CTextureCanvas(this, -1);
	zc_zoom_         = new ui::ZoomControl(this, tex_canvas_);
	cb_tex_scale_    = new wxCheckBox(this, -1, wxS("Apply Scale"));
	cb_tex_arc_      = new wxCheckBox(this, -1, wxS("Aspect Ratio Correction"));
	cb_draw_outside_ = new wxCheckBox(this, -1, wxS("Show Outside"));
	choice_viewtype_ = new wxChoice(this, -1);
	label_viewtype_  = new wxStaticText(this, -1, wxS("Offset Type:"));
	cb_blend_rgba_   = new wxCheckBox(this, -1, wxS("Truecolour Preview"));
}

// -----------------------------------------------------------------------------
// Initialises the panel layout (must be called after the constructor to work
// correctly for ZTextureEditorPanel)
// -----------------------------------------------------------------------------
void TextureEditorPanel::setupLayout()
{
	// Init controls
	cb_tex_scale_->SetValue(tx_apply_scale);
	cb_tex_arc_->SetValue(tx_arc);
	cb_draw_outside_->SetValue(tx_show_outside);
	choice_viewtype_->SetSelection(0);
	tex_canvas_->setViewType(CTextureCanvas::View::Normal);
	cb_blend_rgba_->SetValue(false);
	choice_viewtype_->Set(wxutil::arrayStringStd({ "None", "Sprite", "HUD" }));

	// Only show these on ZTextureEditorPanel
	cb_blend_rgba_->Show(false);
	choice_viewtype_->Show(false);
	label_viewtype_->Show(false);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Setup left section (view controls + texture canvas + texture controls)
	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND | wxRIGHT, ui::pad());

	// Add view controls
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND | wxBOTTOM | wxTOP, ui::px(ui::Size::PadMinimum));
	hbox->Add(zc_zoom_, 0, wxEXPAND | wxRIGHT, ui::pad());
	hbox->AddStretchSpacer();
	hbox->Add(cb_tex_scale_, 0, wxEXPAND | wxRIGHT, ui::pad());
	hbox->Add(cb_tex_arc_, 0, wxEXPAND | wxRIGHT, ui::pad());
	hbox->Add(cb_draw_outside_, 0, wxEXPAND);

	// Add texture canvas
	vbox->Add(tex_canvas_, 1, wxEXPAND);

	// Apply texture canvas options
	tex_canvas_->applyTexScale(tx_apply_scale);
	tex_canvas_->drawOutside(tx_show_outside);

	// Add extra view controls
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND | wxBOTTOM | wxTOP, ui::pad());
	hbox->Add(label_viewtype_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, ui::pad());
	hbox->Add(choice_viewtype_, 0, wxEXPAND);
	hbox->AddStretchSpacer();
	hbox->Add(cb_blend_rgba_, 0, wxEXPAND);

	// Add texture controls
	vbox->Add(createTextureControls(this), 0, wxEXPAND);

	// Setup right section (patch controls)
	vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 0, wxEXPAND);

	// Add patch controls
	vbox->Add(createPatchControls(this), 1, wxEXPAND);


	// Bind events
	cb_draw_outside_->Bind(wxEVT_CHECKBOX, &TextureEditorPanel::onDrawOutsideChanged, this);
	tex_canvas_->Bind(wxEVT_LEFT_DOWN, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->Bind(wxEVT_LEFT_DCLICK, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->Bind(wxEVT_LEFT_UP, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->Bind(wxEVT_RIGHT_UP, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->Bind(wxEVT_MOTION, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas_->Bind(EVT_DRAG_END, &TextureEditorPanel::onTexCanvasDragEnd, this);
	tex_canvas_->Bind(wxEVT_KEY_DOWN, &TextureEditorPanel::onTexCanvasKeyDown, this);
	text_tex_name_->Bind(wxEVT_TEXT, &TextureEditorPanel::onTexNameChanged, this);
	spin_tex_width_->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexWidthChanged, this);
	spin_tex_height_->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexHeightChanged, this);
	spin_tex_width_->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onTexWidthChanged, this);
	spin_tex_height_->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onTexHeightChanged, this);
	list_patches_->Bind(wxEVT_LIST_ITEM_SELECTED, &TextureEditorPanel::onPatchListSelect, this);
	list_patches_->Bind(wxEVT_LIST_ITEM_DESELECTED, &TextureEditorPanel::onPatchListDeSelect, this);
	spin_patch_left_->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onPatchPositionXChanged, this);
	spin_patch_top_->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onPatchPositionYChanged, this);
	spin_patch_left_->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onPatchPositionXChanged, this);
	spin_patch_top_->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onPatchPositionYChanged, this);
	cb_tex_scale_->Bind(wxEVT_CHECKBOX, &TextureEditorPanel::onApplyScaleChanged, this);
	cb_tex_arc_->Bind(wxEVT_CHECKBOX, &TextureEditorPanel::onARCChanged, this);

	// Init layout
	Layout();
}

// -----------------------------------------------------------------------------
// Creates/sets up a panel with controls to edit texture properties
// -----------------------------------------------------------------------------
wxPanel* TextureEditorPanel::createTextureControls(wxWindow* parent)
{
	// Create controls
	const auto     spinsize  = wxSize{ ui::px(ui::Size::SpinCtrlWidth), -1 };
	constexpr auto spinflags = wxSP_ARROW_KEYS | wxALIGN_RIGHT | wxTE_PROCESS_ENTER;
	auto           panel     = new wxPanel(parent, -1);
	text_tex_name_           = new wxTextCtrl(panel, -1);
	text_tex_name_->SetMaxLength(8);
	spin_tex_width_    = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, spinsize, spinflags, 0, SHRT_MAX);
	spin_tex_height_   = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, spinsize, spinflags, 0, SHRT_MAX);
	spin_tex_scalex_   = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, spinsize, spinflags, 0, UCHAR_MAX);
	spin_tex_scaley_   = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, spinsize, spinflags, 0, UCHAR_MAX);
	label_scaled_size_ = new wxStaticText(panel, -1, wxS("Scaled Size: N/A"));
	cb_tex_world_panning_ = new wxCheckBox(panel, -1, wxS("World Panning"));

	// Setup tex controls panel sizer
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

	// "Texture Properties" frame
	auto frame      = new wxStaticBox(panel, -1, wxS("Texture Properties"));
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 1, wxEXPAND);

	auto gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	framesizer->Add(gb_sizer, 1, wxALL, ui::pad());

	// Layout
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Name:")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_tex_name_, { 0, 1 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Size:")), { 0, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_width_, { 0, 3 }, { 1, 1 });
	gb_sizer->Add(spin_tex_height_, { 0, 4 }, { 1, 1 });
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Scale:")), { 1, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_scalex_, { 1, 3 }, { 1, 1 });
	gb_sizer->Add(spin_tex_scaley_, { 1, 4 }, { 1, 1 });
	gb_sizer->Add(label_scaled_size_, { 1, 5 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_tex_world_panning_, { 1, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL);


	// Bind events
	spin_tex_scalex_->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexScaleXChanged, this);
	spin_tex_scaley_->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexScaleYChanged, this);
	cb_tex_world_panning_->Bind(wxEVT_CHECKBOX, &TextureEditorPanel::onTexWorldPanningChanged, this);

	return panel;
}

// -----------------------------------------------------------------------------
// Updates all texture editing controls with values from the texture
// -----------------------------------------------------------------------------
void TextureEditorPanel::updateTextureControls()
{
	// Check texture is open
	if (!tex_current_)
		return;

	bool modified = tex_modified_;
	text_tex_name_->SetValue(wxString::FromUTF8(tex_current_->name()));
	spin_tex_width_->SetValue(tex_current_->width());
	spin_tex_height_->SetValue(tex_current_->height());
	spin_tex_scalex_->SetValue(tex_current_->scaleX() * 8);
	spin_tex_scaley_->SetValue(tex_current_->scaleY() * 8);
	cb_tex_world_panning_->SetValue(tex_current_->worldPanning());
	updateTextureScaleLabel();
	tex_modified_ = modified;
}

// -----------------------------------------------------------------------------
// Updates the 'Scaled Size' label according to the current texture's
// dimensions and scale
// -----------------------------------------------------------------------------
void TextureEditorPanel::updateTextureScaleLabel()
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Determine scaled X value
	uint32_t scaled_x = tex_current_->width();
	if (tex_current_->scaleX() != 0)
		scaled_x /= tex_current_->scaleX();

	// Determine scaled Y value
	uint32_t scaled_y = tex_current_->height();
	if (tex_current_->scaleY() != 0)
		scaled_y /= tex_current_->scaleY();

	// Update the label
	label_scaled_size_->SetLabel(WX_FMT("Scaled Size: {}x{}", scaled_x, scaled_y));
}

// -----------------------------------------------------------------------------
// Creates/sets up a panel with controls to edit a texture's patches
// -----------------------------------------------------------------------------
wxPanel* TextureEditorPanel::createPatchControls(wxWindow* parent)
{
	auto panel = new wxPanel(parent, -1);

	// Setup panel sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// -- Texture Patches frame --
	auto frame      = new wxStaticBox(panel, -1, wxS("Patches"));
	auto framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	sizer->Add(framesizer, 0, wxEXPAND | wxBOTTOM, ui::pad());

	// Create patches list
	list_patches_ = new ListView(panel, -1);
	list_patches_->enableSizeUpdate(false);

	// Create patches toolbar
	tb_patches_ = new SToolBar(panel, false, wxVERTICAL);
	tb_patches_->addActionGroup(
		"_Patch",
		{ "txed_patch_add",
		  "txed_patch_remove",
		  "txed_patch_back",
		  "txed_patch_forward",
		  "txed_patch_replace",
		  "txed_patch_duplicate" });
	tb_patches_->group("_Patch")->setAllButtonsEnabled(false);
	tb_patches_->findActionButton("txed_patch_add")->Enable();

	// Layout
	list_patches_->SetInitialSize(wxutil::scaledSize(100, tb_patches_->group("_Patch")->GetBestSize().y));
	framesizer->Add(list_patches_, 1, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, ui::pad());
	framesizer->Add(tb_patches_, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, ui::px(ui::Size::PadMinimum));


	// -- Patch Properties frame --
	frame      = new wxStaticBox(panel, -1, wxS("Patch Properties"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 1, wxEXPAND);

	// X Position
	const auto     spinsize  = wxSize{ ui::px(ui::Size::SpinCtrlWidth), -1 };
	constexpr auto spinflags = wxSP_ARROW_KEYS | wxALIGN_RIGHT | wxTE_PROCESS_ENTER;
	auto           hbox      = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, 0, wxEXPAND | wxALL, ui::pad());
	spin_patch_left_ = new wxSpinCtrl(
		panel, -1, wxEmptyString, wxDefaultPosition, spinsize, spinflags, SHRT_MIN, SHRT_MAX);
	hbox->Add(new wxStaticText(panel, -1, wxS("X Position:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, ui::pad());
	hbox->Add(spin_patch_left_, 1);

	// Y Position
	hbox = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, ui::pad());
	spin_patch_top_ = new wxSpinCtrl(
		panel, -1, wxEmptyString, wxDefaultPosition, spinsize, spinflags, SHRT_MIN, SHRT_MAX);
	hbox->Add(new wxStaticText(panel, -1, wxS("Y Position:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, ui::pad());
	hbox->Add(spin_patch_top_, 1);

	return panel;
}

// -----------------------------------------------------------------------------
// Populates the patch list with all patches present in the texture
// -----------------------------------------------------------------------------
void TextureEditorPanel::populatePatchList()
{
	// Clear current list
	list_patches_->ClearAll();
	list_patches_->Show(false);

	// Add columns
	list_patches_->InsertColumn(0, wxS("Name"));

	// Check a texture is currently opened for editing
	if (!tex_current_)
		return;

	// Add each patch to the list
	for (size_t a = 0; a < tex_current_->nPatches(); a++)
		list_patches_->addItem(a, tex_current_->patch(a)->name());

	// Update list width
	list_patches_->Show(true);
	list_patches_->GetParent()->Layout();
}

// -----------------------------------------------------------------------------
// Updates all patch editing controls with values from the currently selected
// patch. Behaves differently depending on the number of patches selected
// -----------------------------------------------------------------------------
void TextureEditorPanel::updatePatchControls()
{
	// Get selected patches
	auto selection = list_patches_->selectedItems();

	// If nothing is selected, disable patch controls
	if (selection.empty())
	{
		spin_patch_left_->Enable(false);
		spin_patch_top_->Enable(false);
	}
	else
	{
		// Something is selected, enable the controls
		spin_patch_left_->Enable(true);
		spin_patch_top_->Enable(true);

		// If only 1 patch is selected, just set the controls to this patch
		if (selection.size() == 1)
		{
			auto patch = tex_current_->patch(selection[0]);
			if (!patch)
			{
				log::error("Selected patch does not exist in texture");
				return;
			}

			spin_patch_left_->SetValue(patch->xOffset());
			spin_patch_top_->SetValue(patch->yOffset());
		}
		else
		{
			// Multiple selection, only enable some controls
			spin_patch_left_->Enable(false);
			spin_patch_top_->Enable(false);
		}
	}
}

// ----------------------------------------------------------------------------
// Updates the texture name textbox with [new_name]
// ----------------------------------------------------------------------------
void TextureEditorPanel::updateTextureName(const string& new_name) const
{
	text_tex_name_->SetValue(wxString::FromUTF8(new_name));
}

// ----------------------------------------------------------------------------
// Loads a TEXTUREX format texture into the editor
// -----------------------------------------------------------------------------
bool TextureEditorPanel::openTexture(const CTexture* tex, TextureXList* list)
{
	// Check texture was given
	if (!tex)
	{
		clearTexture();
		return false;
	}

	// Set as current texture
	if (!tex_current_)
		tex_current_ = std::make_unique<CTexture>();
	tex_current_->copyTexture(*tex);
	tex_current_->setList(list);

	// Open texture in canvas
	tex_canvas_->openTexture(tex_current_.get(), tx_editor_->archive());

	// Set control values
	updateTextureControls();
	populatePatchList();
	updatePatchControls();

	tex_modified_ = false;

	return true;
}

// -----------------------------------------------------------------------------
// Clears the current texture
// -----------------------------------------------------------------------------
void TextureEditorPanel::clearTexture()
{
	// Clear texture
	tex_current_.reset(nullptr);
	tex_canvas_->clearTexture();

	// Update variables
	tex_modified_ = false;

	// Set control values
	updateTextureControls();
	populatePatchList();
	updatePatchControls();
}

// -----------------------------------------------------------------------------
// Sets the texture canvas' palette and refreshes it
// -----------------------------------------------------------------------------
void TextureEditorPanel::setPalette(Palette* pal) const
{
	tex_canvas_->setPalette(pal);
	tex_canvas_->updatePatchTextures();
	tex_canvas_->Refresh();
}

// -----------------------------------------------------------------------------
// Returns the texture canvas' palette
// -----------------------------------------------------------------------------
Palette* TextureEditorPanel::palette() const
{
	return &tex_canvas_->palette();
}

// -----------------------------------------------------------------------------
// Returns true if the texture uses RGBA blending
// -----------------------------------------------------------------------------
bool TextureEditorPanel::blendRGBA() const
{
	return tex_canvas_->blendRGBA();
}

// -----------------------------------------------------------------------------
// Prompts the user to select a patch from the patch table to be added to the
// current texture
// -----------------------------------------------------------------------------
void TextureEditorPanel::addPatch()
{
	// Do nothing if patch list is empty
	if (tx_editor_->patchTable().nPatches() == 0 || !tex_current_)
		return;

	// Browse for patch
	tx_editor_->setFullPath(false);
	int patch = tx_editor_->browsePatchTable();
	if (patch >= 0)
	{
		// Add new patch
		tex_current_->addPatch(tx_editor_->patchTable().patchName(patch), 0, 0);

		// Update UI
		populatePatchList();
		updatePatchControls();
	}

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Removes selected patch(es) from the current texture
// -----------------------------------------------------------------------------
void TextureEditorPanel::removePatch()
{
	// Get selection
	auto selection = list_patches_->selectedItems();

	// Do nothing if no patches are selected
	if (selection.empty())
		return;

	// Remove each selected patch
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		int index = selection[a];

		// Remove patch from texture
		tex_current_->removePatch(index);

		// Remove patch from list
		list_patches_->DeleteItem(index);
	}

	// Update UI
	updatePatchControls();
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Moves selected patch(es) 'back' in the current texture
// -----------------------------------------------------------------------------
void TextureEditorPanel::patchBack()
{
	// Get selected patch(es)
	auto selection = list_patches_->selectedItems();

	// Do nothing if nothing is selected
	if (selection.empty())
		return;

	// Do nothing if first patch is selected
	if (selection[0] == 0)
		return;

	// Go through selection
	for (int index : selection)
	{
		// Swap in list
		list_patches_->swapItems(index, index - 1);

		// Swap in texture
		tex_canvas_->swapPatches(index, index - 1);
	}

	// Update UI
	updatePatchControls();
	tex_canvas_->redraw(true);

	// Restore selection in texture canvas
	selection = list_patches_->selectedItems();
	for (int index : selection)
		tex_canvas_->selectPatch(index);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Moves selected patch(es) 'forward' in the current texture
// -----------------------------------------------------------------------------
void TextureEditorPanel::patchForward()
{
	// Get selected patch(es)
	auto selection = list_patches_->selectedItems();

	// Do nothing if nothing is selected
	if (selection.empty())
		return;

	// Do nothing if last patch is selected
	if (selection.back() == list_patches_->GetItemCount() - 1)
		return;

	// Go through selection from bottom up
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Swap in list
		list_patches_->swapItems(selection[a], selection[a] + 1);

		// Swap in texture
		tex_canvas_->swapPatches(selection[a], selection[a] + 1);
	}

	// Update UI
	updatePatchControls();
	tex_canvas_->redraw(true);

	// Restore selection in texture canvas
	selection = list_patches_->selectedItems();
	for (int index : selection)
		tex_canvas_->selectPatch(index);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Prompts the user to select a patch from the patch table to replace selected
// patch(es) with
// -----------------------------------------------------------------------------
void TextureEditorPanel::replacePatch()
{
	// Get selection
	auto selection = list_patches_->selectedItems();

	// Do nothing if no patches are selected
	if (selection.empty())
		return;

	// Get first selected patch name (for browser)
	auto pname = tex_canvas_->texture()->patch(selection[0])->name();

	// Browse for patch
	tx_editor_->setFullPath(false);
	int patch = tx_editor_->browsePatchTable(pname);
	if (patch >= 0)
	{
		// Go through selection and replace each patch
		for (int index : selection)
			tex_current_->replacePatch(index, tx_editor_->patchTable().patchName(patch));
	}

	// Repopulate patch list
	populatePatchList();

	// Restore selection
	for (int item : selection)
		list_patches_->selectItem(item);

	// Update UI
	updatePatchControls();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Duplicates selected patch(es) in the current texture (each duplication is
// placed 8 units right+down from its original patch)
// -----------------------------------------------------------------------------
void TextureEditorPanel::duplicatePatch(int xoff, int yoff)
{
	// Get selection
	auto selection = list_patches_->selectedItems();

	// Do nothing if no patches are selected
	if (selection.empty())
		return;

	// Go through selection backwards
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Duplicate selected patch
		tex_current_->duplicatePatch(selection[a], xoff, yoff);
	}

	// Repopulate patch list
	populatePatchList();

	// Update selection
	int offset = 1;
	for (int index : selection)
	{
		list_patches_->selectItem(index + offset);
		offset++;
	}

	// Update UI
	updatePatchControls();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool TextureEditorPanel::handleSAction(string_view id)
{
	// Add Patch
	if (id == "txed_patch_add")
		addPatch();

	// Remove Patch
	else if (id == "txed_patch_remove")
		removePatch();

	// Send Patch Back
	else if (id == "txed_patch_back")
		patchBack();

	// Bring Patch Forward
	else if (id == "txed_patch_forward")
		patchForward();

	// Replace Patch
	else if (id == "txed_patch_replace")
		replacePatch();

	// Duplicate Patch
	else if (id == "txed_patch_duplicate")
		duplicatePatch();

	// Unknown action
	else
		return false;

	// Action was handled
	return true;
}


// -----------------------------------------------------------------------------
//
// TextureEditorPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'show outside' checkbox is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onDrawOutsideChanged(wxCommandEvent& e)
{
	// Set texture canvas 'show outside' option depending on checkbox value
	tx_show_outside = cb_draw_outside_->GetValue();
	tex_canvas_->drawOutside(cb_draw_outside_->GetValue());

	// Update UI
	tex_canvas_->redraw(false);
}

// -----------------------------------------------------------------------------
// Called on any mouse event within the texture canvas
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexCanvasMouseEvent(wxMouseEvent& e)
{
	// Get mouse position relative to texture
	auto pos = tex_canvas_->screenToTexPosition(e.GetX() * GetContentScaleFactor(), e.GetY() * GetContentScaleFactor());

	// Get patch that the mouse is over (if any)
	int patch = tex_canvas_->patchAt(pos.x, pos.y);

	// LEFT DOUBLE CLICK
	if (e.ButtonDClick(wxMOUSE_BTN_LEFT))
	{
		replacePatch();
	}

	// LEFT MOUSE DOWN
	else if (e.LeftDown())
	{
		// If not dragging
		if (e.ShiftDown())
		{
			// Shift is down, add to selection
			if (patch >= 0)
				list_patches_->selectItem(patch);
		}
		else if (e.ControlDown())
		{
			// Control is down, remove from selection
			if (patch >= 0)
				list_patches_->deSelectItem(patch);
		}
		else
		{
			// Clear selection only if patch clicked was not already selected
			if (!tex_canvas_->patchSelected(patch))
				list_patches_->clearSelection();

			// Select patch
			if (patch >= 0)
				list_patches_->selectItem(patch);
		}
	}

	// LEFT MOUSE UP
	else if (e.LeftUp())
	{
		// Hide texture grid
		tex_canvas_->showGrid(false);

		// If mouse up over an already-selected patch, and shift/ctrl aren't down,
		// select only that patch (this mimics 'normal' drag-and-drop/selection behaviour)
		if (!e.ShiftDown() && !e.ControlDown() && tex_canvas_->patchSelected(patch) && !tex_canvas_->isDragging())
		{
			list_patches_->clearSelection();
			list_patches_->selectItem(patch);
		}

		// Redraw texture canvas
		tex_canvas_->redraw(false);
		updateTextureControls();
	}

	// RIGHT MOUSE UP
	else if (e.RightUp())
	{
		// Create context menu
		wxMenu popup;
		SAction::fromId("txed_patch_add")->addToMenu(&popup, true);
		SAction::fromId("txed_patch_remove")->addToMenu(&popup, true);
		SAction::fromId("txed_patch_replace")->addToMenu(&popup, true);
		SAction::fromId("txed_patch_back")->addToMenu(&popup, true);
		SAction::fromId("txed_patch_forward")->addToMenu(&popup, true);
		SAction::fromId("txed_patch_duplicate")->addToMenu(&popup, true);

		hack_nodrag = true;
		PopupMenu(&popup);
	}

	// MOUSE DRAGGING
	else if (e.Dragging())
	{
		// Drag selected patches if left button is down and any patch is selected
		if (hack_nodrag)
			hack_nodrag = false;
		else if (e.LeftIsDown())
		{
			if (list_patches_->GetSelectedItemCount() > 0)
			{
				// Get drag amount according to texture
				Vec2i tex_cur = tex_canvas_->screenToTexPosition(
					e.GetX() * GetContentScaleFactor(), e.GetY() * GetContentScaleFactor());
				Vec2i tex_prev = tex_canvas_->screenToTexPosition(
					tex_canvas_->mousePrevPos().x, tex_canvas_->mousePrevPos().y);
				Vec2i diff = tex_cur - tex_prev;

				// Move any selected patches
				auto selected_patches = list_patches_->selectedItems();
				for (int index : selected_patches)
				{
					auto sel_patch = tex_current_->patch(index);
					if (!sel_patch)
						continue;
					int16_t cx = sel_patch->xOffset();
					int16_t cy = sel_patch->yOffset();
					sel_patch->setOffsetX(cx + diff.x);
					sel_patch->setOffsetY(cy + diff.y);
					tex_modified_ = true;
				}

				// Refresh texture canvas
				tex_canvas_->showGrid(true);
				tex_canvas_->redraw(false);
			}
			else if (
				tex_current_ && tex_current_->isExtended() && tex_canvas_->viewType() != CTextureCanvas::View::Normal)
			{
				// Get drag amount according to texture
				Vec2i tex_cur = tex_canvas_->screenToTexPosition(
					e.GetX() * GetContentScaleFactor(), e.GetY() * GetContentScaleFactor());
				Vec2i tex_prev = tex_canvas_->screenToTexPosition(
					tex_canvas_->mousePrevPos().x, tex_canvas_->mousePrevPos().y);
				Vec2i diff = tex_cur - tex_prev;

				// Modify offsets
				tex_current_->setOffsetX(tex_current_->offsetX() - diff.x);
				tex_current_->setOffsetY(tex_current_->offsetY() - diff.y);
				tex_modified_ = true;

				// Refresh texture canvas
				tex_canvas_->redraw(false);
			}
		}
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a mouse drag action ends on the texture canvas
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexCanvasDragEnd(wxCommandEvent& e)
{
	// If patch dragging ended (left button)
	if (e.GetInt() == wxMOUSE_BTN_LEFT)
	{
		// Update patch controls
		updatePatchControls();
	}
}

// -----------------------------------------------------------------------------
// Called when a key is pressed within the texture canvas
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexCanvasKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	auto binds = KeyBind::bindsForKey(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Check for alt key
	if (e.GetKeyCode() == WXK_ALT)
		alt_press_ = true;

	// Go through matching binds
	int  x_movement = 0;
	int  y_movement = 0;
	bool handled    = false;
	for (const auto& name : binds)
	{
		// Move patch left
		if (name == "txed_patch_left")
			x_movement = -1;
		else if (name == "txed_patch_left8")
			x_movement = -8;

		// Move patch up
		else if (name == "txed_patch_up")
			y_movement = -1;
		else if (name == "txed_patch_up8")
			y_movement = -8;

		// Move patch right
		else if (name == "txed_patch_right")
			x_movement = 1;
		else if (name == "txed_patch_right8")
			x_movement = 8;

		// Move patch down
		else if (name == "txed_patch_down")
			y_movement = 1;
		else if (name == "txed_patch_down8")
			y_movement = 8;

		// Add patch
		else if (name == "txed_patch_add")
		{
			hack_nodrag = true;
			addPatch();
			handled = true;
		}

		// Delete patch
		else if (name == "txed_patch_delete")
		{
			removePatch();
			handled = true;
		}

		// Replace patch
		else if (name == "txed_patch_replace")
		{
			hack_nodrag = true;
			replacePatch();
			handled = true;
		}

		// Duplicate patch
		else if (name == "txed_patch_duplicate")
		{
			duplicatePatch();
			handled = true;
		}

		// Bring patch forward
		else if (name == "txed_patch_forward")
		{
			patchForward();
			handled = true;
		}

		// Send patch back
		else if (name == "txed_patch_back")
		{
			patchBack();
			handled = true;
		}
	}

	// Move patches if needed
	if (x_movement != 0 || y_movement != 0)
	{
		// Do patch duplicate if alt is pressed
		if (e.GetModifiers() == wxMOD_ALT && alt_press_)
		{
			duplicatePatch(0, 0);
			alt_press_ = false;
		}

		auto selected_patches = list_patches_->selectedItems();
		for (int selected_patche : selected_patches)
		{
			auto patch = tex_current_->patch(selected_patche);
			if (!patch)
				continue;
			int16_t cx = patch->xOffset();
			int16_t cy = patch->yOffset();
			patch->setOffsetX(cx + x_movement);
			patch->setOffsetY(cy + y_movement);
			tex_modified_ = true;
		}

		tex_canvas_->redraw(true);
		handled = true;
	}

	if (!e.AltDown())
		alt_press_ = false;

	if (!handled)
		e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the texture name entry box is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexNameChanged(wxCommandEvent& e)
{
	// Change texture name
	if (tex_current_)
		tex_current_->setName(text_tex_name_->GetValue().utf8_string());

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture width spin control is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexWidthChanged(wxCommandEvent& e)
{
	// Set texture's width
	if (tex_current_)
		tex_current_->setWidth(spin_tex_width_->GetValue());

	// Update UI
	tex_canvas_->redraw(true);
	updateTextureScaleLabel();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture height spin control is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexHeightChanged(wxCommandEvent& e)
{
	// Set texture's height
	if (tex_current_)
		tex_current_->setHeight(spin_tex_height_->GetValue());

	// Update UI
	tex_canvas_->redraw(true);
	updateTextureScaleLabel();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture x scale spin control is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexScaleXChanged(wxCommandEvent& e)
{
	// Set texture's x scale
	if (tex_current_)
		tex_current_->setScaleX((double)spin_tex_scalex_->GetValue() / 8.0);

	// Update UI
	updateTextureScaleLabel();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture y scale spin control is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexScaleYChanged(wxCommandEvent& e)
{
	// Set texture's y scale
	if (tex_current_)
		tex_current_->setScaleY((double)spin_tex_scaley_->GetValue() / 8.0);

	// Update UI
	updateTextureScaleLabel();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture world panning checkbox is toggled
// -----------------------------------------------------------------------------
void TextureEditorPanel::onTexWorldPanningChanged(wxCommandEvent& e)
{
	// Set texture world panning flag
	if (tex_current_)
		tex_current_->setWorldPanning(cb_tex_world_panning_->IsChecked());

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when a patch is selected on the patch list
// -----------------------------------------------------------------------------
void TextureEditorPanel::onPatchListSelect(wxListEvent& e)
{
	if (list_patches_->GetSelectedItemCount() > 0)
		tb_patches_->group("_Patch")->setAllButtonsEnabled(true);

	// Select the patch on the texture canvas
	tex_canvas_->selectPatch(e.GetIndex());

	// Update UI
	tex_canvas_->redraw(false);
	updatePatchControls();
}

// -----------------------------------------------------------------------------
// Called when a patch is deselected on the patch list
// -----------------------------------------------------------------------------
void TextureEditorPanel::onPatchListDeSelect(wxListEvent& e)
{
	if (list_patches_->GetSelectedItemCount() == 0)
	{
		tb_patches_->group("_Patch")->setAllButtonsEnabled(false);
		tb_patches_->findActionButton("txed_patch_add")->Enable();
	}

	// Deselect the patch on the texture canvas
	tex_canvas_->deSelectPatch(e.GetIndex());

	// Update UI
	tex_canvas_->redraw(false);
	updatePatchControls();
}

// -----------------------------------------------------------------------------
// Called when the patch x position spin control is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onPatchPositionXChanged(wxCommandEvent& e)
{
	// If anything other than 1 patch is selected, do nothing (shouldn't happen anyway)
	if (list_patches_->GetSelectedItemCount() != 1)
		return;

	// Get selected patch
	auto patch = tex_current_->patch(list_patches_->selectedItems()[0]);
	if (!patch)
		return;

	// Set patch x offset
	patch->setOffsetX(spin_patch_left_->GetValue());

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the patch y position spin control is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onPatchPositionYChanged(wxCommandEvent& e)
{
	// If anything other than 1 patch is selected, do nothing (shouldn't happen anyway)
	if (list_patches_->GetSelectedItemCount() != 1)
		return;

	// Get selected patch
	auto patch = tex_current_->patch(list_patches_->selectedItems()[0]);
	if (!patch)
		return;

	// Set patch y offset
	patch->setOffsetY(spin_patch_top_->GetValue());

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Apply Scale' checkbox is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onApplyScaleChanged(wxCommandEvent& e)
{
	tx_apply_scale = cb_tex_scale_->GetValue();
	tex_canvas_->applyTexScale(cb_tex_scale_->GetValue());
	tex_canvas_->redraw();
}

// -----------------------------------------------------------------------------
// Called when the 'Aspect Ratio Correction' checkbox is changed
// -----------------------------------------------------------------------------
void TextureEditorPanel::onARCChanged(wxCommandEvent& e)
{
	tx_arc = cb_tex_arc_->IsChecked();
	tex_canvas_->redraw();
}
