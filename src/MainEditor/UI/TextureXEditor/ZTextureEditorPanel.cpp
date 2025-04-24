
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ZTextureEditorPanel.cpp
// Description: The UI for viewing/editing an enhanced TEXTURES-format
//              composite texture
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
#include "ZTextureEditorPanel.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "MainEditor/MainEditor.h"
#include "TextureXEditor.h"
#include "UI/Canvas/GL/CTextureGLCanvas.h"
#include "UI/Controls/ColourBox.h"
#include "UI/Dialogs/TranslationEditorDialog.h"
#include "UI/Layout.h"
#include "UI/Lists/ListView.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/WxUtils.h"
#include "Utility/Tokenizer.h"


using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
CTextureGLCanvas::View view_types[] = { CTextureGLCanvas::View::Normal,
										CTextureGLCanvas::View::Sprite,
										CTextureGLCanvas::View::HUD };
}
CVAR(Bool, tx_truecolour, true, CVar::Flag::Save)
CVAR(Int, tx_offset_type, 0, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// ZTextureEditorPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Creates/sets up a panel with controls to edit texture properties
// -----------------------------------------------------------------------------
wxPanel* ZTextureEditorPanel::createTextureControls(wxWindow* parent)
{
	auto lh = ui::LayoutHelper(parent);

	auto panel = new wxScrolledWindow(parent, -1);
	panel->SetScrollRate(4, 0);

	// Setup tex controls panel sizer
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

	// "Texture Properties" frame
	auto frame      = new wxStaticBox(panel, -1, wxS("Texture Properties"));
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxSizerFlags().Expand());

	auto gb_sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	framesizer->Add(gb_sizer, lh.sfWithBorder(1).Expand());

	// Name
	text_tex_name_ = new wxTextCtrl(panel, -1);
	text_tex_name_->SetMaxLength(0);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Name:")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_tex_name_, { 0, 1 }, { 1, 2 }, wxEXPAND);

	// Size
	constexpr auto spinflags = wxSP_ARROW_KEYS | wxALIGN_RIGHT | wxTE_PROCESS_ENTER;
	spin_tex_width_          = new wxSpinCtrl(
        panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, 0, SHRT_MAX);
	spin_tex_height_ = new wxSpinCtrl(
		panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, 0, SHRT_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Size:")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_width_, { 1, 1 }, { 1, 1 });
	gb_sizer->Add(spin_tex_height_, { 1, 2 }, { 1, 1 });

	// Scale
	spin_tex_scalex_ = new wxSpinCtrlDouble(
		panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, 0.1, 100, 1, 0.1);
	spin_tex_scaley_ = new wxSpinCtrlDouble(
		panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, 0.1, 100, 1, 0.1);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Scale:")), { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_scalex_, { 2, 1 }, { 1, 1 });
	gb_sizer->Add(spin_tex_scaley_, { 2, 2 }, { 1, 1 });

	// Scaled size
	label_scaled_size_ = new wxStaticText(panel, -1, wxS("Scaled Size: N/A"));
	gb_sizer->Add(label_scaled_size_, { 3, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL);

	// Type
	vector<string> types = { "Texture", "Sprite", "Graphic", "WallTexture", "Flat" };
	choice_type_         = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(types));
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Type:")), { 0, 3 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_type_, { 0, 4 }, { 1, 2 }, wxEXPAND);

	// Offsets
	spin_tex_offsetx_ = new wxSpinCtrl(
		panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, INT_MIN, INT_MAX);
	spin_tex_offsety_ = new wxSpinCtrl(
		panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, INT_MIN, INT_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Offsets:")), { 1, 3 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_offsetx_, { 1, 4 }, { 1, 1 });
	gb_sizer->Add(spin_tex_offsety_, { 1, 5 }, { 1, 1 });

	// Flags
	cb_optional_     = new wxCheckBox(panel, -1, wxS("Optional"));
	cb_worldpanning_ = new wxCheckBox(panel, -1, wxS("World Panning"));
	cb_nodecals_     = new wxCheckBox(panel, -1, wxS("No Decals"));
	cb_nulltexture_  = new wxCheckBox(panel, -1, wxS("Null Texture"));
	gb_sizer->Add(cb_optional_, { 2, 4 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_worldpanning_, { 2, 5 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_nodecals_, { 3, 4 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_nulltexture_, { 3, 5 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);


	// Bind events
	spin_tex_scalex_->Bind(wxEVT_SPINCTRLDOUBLE, &ZTextureEditorPanel::onTexScaleXChanged, this);
	spin_tex_scaley_->Bind(wxEVT_SPINCTRLDOUBLE, &ZTextureEditorPanel::onTexScaleYChanged, this);
	spin_tex_scalex_->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTexScaleXChanged, this);
	spin_tex_scaley_->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTexScaleYChanged, this);
	spin_tex_offsetx_->Bind(wxEVT_SPINCTRL, &ZTextureEditorPanel::onTexOffsetXChanged, this);
	spin_tex_offsety_->Bind(wxEVT_SPINCTRL, &ZTextureEditorPanel::onTexOffsetYChanged, this);
	spin_tex_offsetx_->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTexOffsetXChanged, this);
	spin_tex_offsety_->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTexOffsetYChanged, this);
	choice_type_->Bind(wxEVT_CHOICE, &ZTextureEditorPanel::onTexTypeChanged, this);
	cb_optional_->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onTexOptionalChanged, this);
	cb_worldpanning_->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onTexWorldPanningChanged, this);
	cb_nodecals_->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onTexNoDecalsChanged, this);
	cb_nulltexture_->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onTexNullTextureChanged, this);
	cb_blend_rgba_->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onCBBlendRGBAChanged, this);
	choice_viewtype_->Bind(wxEVT_CHOICE, &ZTextureEditorPanel::onChoiceViewTypeSelected, this);

	// Show extra view options
	cb_blend_rgba_->SetValue(tx_truecolour);
	cb_blend_rgba_->Show(true);
	choice_viewtype_->SetSelection(tx_offset_type);
	tex_canvas_->setViewType(view_types[tx_offset_type]);
	choice_viewtype_->Show(true);
	label_viewtype_->Show(true);

	return panel;
}

// -----------------------------------------------------------------------------
// Updates all texture editing controls with values from the texture
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::updateTextureControls()
{
	// Check texture is open
	if (!tex_current_)
		return;

	text_tex_name_->SetValue(wxString::FromUTF8(tex_current_->name()));
	spin_tex_width_->SetValue(tex_current_->width());
	spin_tex_height_->SetValue(tex_current_->height());
	spin_tex_scalex_->SetValue(tex_current_->scaleX());
	spin_tex_scaley_->SetValue(tex_current_->scaleY());
	spin_tex_offsetx_->SetValue(tex_current_->offsetX());
	spin_tex_offsety_->SetValue(tex_current_->offsetY());
	updateTextureScaleLabel();

	// Update type
	if (tex_current_->type() == "Texture")
		choice_type_->SetSelection(0);
	else if (tex_current_->type() == "Sprite")
		choice_type_->SetSelection(1);
	else if (tex_current_->type() == "Graphic")
		choice_type_->SetSelection(2);
	else if (tex_current_->type() == "WallTexture")
		choice_type_->SetSelection(3);
	else if (tex_current_->type() == "Flat")
		choice_type_->SetSelection(4);
	else
		choice_type_->SetSelection(0);

	// Update flags
	cb_optional_->SetValue(tex_current_->isOptional());
	cb_worldpanning_->SetValue(tex_current_->worldPanning());
	cb_nodecals_->SetValue(tex_current_->noDecals());
	cb_nulltexture_->SetValue(tex_current_->nullTexture());
}

// -----------------------------------------------------------------------------
// Creates/sets up a panel with controls to edit a texture's patches
// -----------------------------------------------------------------------------
wxPanel* ZTextureEditorPanel::createPatchControls(wxWindow* parent)
{
	auto lh = ui::LayoutHelper(parent);

	auto panel = new wxScrolledWindow(parent, -1);
	panel->SetScrollRate(0, 4);

	// Setup panel sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// -- Texture Patches frame --
	auto frame      = new wxStaticBox(panel, -1, wxS("Patches"));
	auto framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	sizer->Add(framesizer, wxSizerFlags().Expand());

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
	list_patches_->SetInitialSize({ FromDIP(100), tb_patches_->group("_Patch")->GetBestSize().y });
	framesizer->Add(list_patches_, lh.sfWithBorder(1, wxLEFT | wxTOP | wxBOTTOM).Expand());
	framesizer->Add(tb_patches_, lh.sfWithSmallBorder(0, wxLEFT | wxTOP | wxBOTTOM).Expand());


	// -- Patch Properties frame --
	frame      = new wxStaticBox(panel, -1, wxS("Patch Properties"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, lh.sfWithBorder(0, wxTOP).Expand());

	auto* gb_sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	framesizer->Add(gb_sizer, lh.sfWithBorder(1).Expand());

	// X Position
	constexpr auto spinflags = wxSP_ARROW_KEYS | wxALIGN_RIGHT | wxTE_PROCESS_ENTER;
	spin_patch_left_         = new wxSpinCtrl(
        panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, SHRT_MIN, SHRT_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("X Position:")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_patch_left_, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// Y Position
	spin_patch_top_ = new wxSpinCtrl(
		panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, SHRT_MIN, SHRT_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Y Position:")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_patch_top_, { 1, 1 }, { 1, 1 }, wxEXPAND);

	// Use Offsets
	cb_useofs_ = new wxCheckBox(panel, -1, wxS("Use Source Gfx Offsets"));
	gb_sizer->Add(cb_useofs_, { 2, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL);

	// Flip X
	cb_flipx_ = new wxCheckBox(panel, -1, wxS("Flip X"));
	gb_sizer->Add(cb_flipx_, { 3, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	// Flip Y
	cb_flipy_ = new wxCheckBox(panel, -1, wxS("Flip Y"));
	gb_sizer->Add(cb_flipy_, { 3, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	// Rotation
	vector<string> rotval = { "0", "90", "180", "270" };
	choice_rotation_      = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(rotval));
	choice_rotation_->SetSelection(0);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Rotation:")), { 4, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_rotation_, { 4, 1 }, { 1, 1 }, wxEXPAND);

	// Alpha
	spin_alpha_ = new wxSpinCtrlDouble(
		panel, -1, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, 0, 1, 1, 0.1);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Alpha:")), { 5, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_alpha_, { 5, 1 }, { 1, 1 }, wxEXPAND);

	// Alpha Style
	vector<string> styles = { "Copy",     "Translucent", "Add",          "Subtract", "ReverseSubtract",
							  "Modulate", "CopyAlpha",   "CopyNewAlpha", "Overlay" };
	choice_style_         = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(styles));
	choice_style_->SetSelection(0);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Alpha Style:")), { 6, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_style_, { 6, 1 }, { 1, 2 }, wxEXPAND);

	frame      = new wxStaticBox(panel, -1, wxS("Patch Colour"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, lh.sfWithBorder(0, wxTOP).Expand());

	gb_sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	framesizer->Add(gb_sizer, lh.sfWithBorder(1).Expand());

	// 'Normal' colour
	rb_pc_normal_ = new wxRadioButton(panel, -1, wxS("Normal"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	gb_sizer->Add(rb_pc_normal_, { 0, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL);

	gb_sizer->Add(new wxStaticLine(panel, -1), { 1, 0 }, { 1, 2 }, wxEXPAND);

	// Blend
	rb_pc_blend_ = new wxRadioButton(panel, -1, wxS("Blend"));
	rb_pc_tint_  = new wxRadioButton(panel, -1, wxS("Tint"));
	gb_sizer->Add(rb_pc_blend_, { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(rb_pc_tint_, { 2, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	// Blend/Tint colour
	cb_blend_col_ = new ColourBox(panel, -1, false, true);
	cb_blend_col_->setPalette(palette());
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Colour:")), { 3, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_blend_col_, { 3, 1 }, { 1, 1 }, wxALIGN_RIGHT);

	// Tint amount
	spin_tint_amount_ = new wxSpinCtrlDouble(
		panel, 01, wxEmptyString, wxDefaultPosition, lh.spinSize(), spinflags, 0, 1, 0, 0.1);
	gb_sizer->Add(new wxStaticText(panel, -1, wxS("Amount:")), { 4, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tint_amount_, { 4, 1 }, { 1, 1 }, wxEXPAND);

	gb_sizer->Add(new wxStaticLine(panel, -1), { 5, 0 }, { 1, 2 }, wxEXPAND);

	// Translation
	rb_pc_translation_ = new wxRadioButton(panel, -1, wxS("Translation"));
	gb_sizer->Add(rb_pc_translation_, { 6, 0 }, { 1, 2 }, wxALIGN_CENTER_VERTICAL);

	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	gb_sizer->Add(hbox, { 7, 0 }, { 1, 2 }, wxEXPAND);

	// Translation text entry
	text_translation_ = new wxTextCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	hbox->Add(text_translation_, lh.sfWithBorder(1, wxRIGHT).Expand());

	// Translation edit button
	btn_edit_translation_ = new wxButton(panel, -1, wxS("Edit"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	hbox->Add(btn_edit_translation_);


	// Bind events
	cb_flipx_->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onPatchFlipXChanged, this);
	cb_flipy_->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onPatchFlipYChanged, this);
	cb_useofs_->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onPatchUseOfsChanged, this);
	choice_rotation_->Bind(wxEVT_CHOICE, &ZTextureEditorPanel::onPatchRotationChanged, this);
	spin_alpha_->Bind(wxEVT_SPINCTRLDOUBLE, &ZTextureEditorPanel::onPatchAlphaChanged, this);
	spin_alpha_->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onPatchAlphaChanged, this);
	choice_style_->Bind(wxEVT_CHOICE, &ZTextureEditorPanel::onPatchAlphaStyleChanged, this);
	rb_pc_normal_->Bind(wxEVT_RADIOBUTTON, &ZTextureEditorPanel::onPCNormalSelected, this);
	rb_pc_blend_->Bind(wxEVT_RADIOBUTTON, &ZTextureEditorPanel::onPCBlendSelected, this);
	rb_pc_tint_->Bind(wxEVT_RADIOBUTTON, &ZTextureEditorPanel::onPCTintSelected, this);
	rb_pc_translation_->Bind(wxEVT_RADIOBUTTON, &ZTextureEditorPanel::onPCTranslationSelected, this);
	cb_blend_col_->Bind(wxEVT_COLOURBOX_CHANGED, &ZTextureEditorPanel::onPatchColourChanged, this);
	spin_tint_amount_->Bind(wxEVT_SPINCTRLDOUBLE, &ZTextureEditorPanel::onPatchTintAmountChanged, this);
	spin_tint_amount_->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onPatchTintAmountChanged, this);
	btn_edit_translation_->Bind(wxEVT_BUTTON, &ZTextureEditorPanel::onBtnEditTranslation, this);
	text_translation_->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTextTranslationEnter, this);

	return panel;
}

// -----------------------------------------------------------------------------
// Updates all patch editing controls with values from the currently selected
// patch. Behaves differently depending on the number of patches selected
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::updatePatchControls()
{
	// Get selected patches
	auto selection = list_patches_->selectedItems();

	// If nothing is selected, disable patch controls
	if (selection.empty())
	{
		spin_patch_left_->Enable(false);
		spin_patch_top_->Enable(false);
		cb_flipx_->Enable(false);
		cb_flipy_->Enable(false);
		cb_useofs_->Enable(false);
		choice_rotation_->Enable(false);
		spin_alpha_->Enable(false);
		choice_style_->Enable(false);
		rb_pc_normal_->Enable(false);
		rb_pc_blend_->Enable(false);
		rb_pc_tint_->Enable(false);
		rb_pc_translation_->Enable(false);
		cb_blend_col_->Enable(false);
		spin_tint_amount_->Enable(false);
		text_translation_->Enable(false);
		btn_edit_translation_->Enable(false);
	}
	else
	{
		// Something is selected, enable the controls
		spin_patch_left_->Enable(true);
		spin_patch_top_->Enable(true);
		cb_flipx_->Enable(true);
		cb_flipy_->Enable(true);
		cb_useofs_->Enable(true);
		choice_rotation_->Enable(true);
		spin_alpha_->Enable(true);
		choice_style_->Enable(true);
		rb_pc_normal_->Enable(true);
		rb_pc_blend_->Enable(true);
		rb_pc_tint_->Enable(true);
		rb_pc_translation_->Enable(true);
		cb_blend_col_->Enable(true);
		spin_tint_amount_->Enable(true);
		text_translation_->Enable(true);
		btn_edit_translation_->Enable(true);

		// If only 1 patch is selected, just set the controls to this patch
		if (selection.size() == 1)
		{
			auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(selection[0]));
			if (!patch)
			{
				log::error("Selected patch does not exist in texture");
				return;
			}

			spin_patch_left_->SetValue(patch->xOffset());
			spin_patch_top_->SetValue(patch->yOffset());
			cb_flipx_->SetValue(patch->flipX());
			cb_flipy_->SetValue(patch->flipY());
			cb_useofs_->SetValue(patch->useOffsets());
			spin_alpha_->SetValue(patch->alpha());
			choice_style_->SetStringSelection(wxString::FromUTF8(patch->style()));
			cb_blend_col_->setColour(patch->colour());
			spin_tint_amount_->SetValue(static_cast<double>(patch->colour().a) / 255.0);
			text_translation_->SetValue(
				wxString::FromUTF8(patch->hasTranslation() ? patch->translation()->asText() : ""));

			switch (patch->rotation())
			{
			case 0:   choice_rotation_->SetSelection(0); break;
			case 90:  choice_rotation_->SetSelection(1); break;
			case 180: choice_rotation_->SetSelection(2); break;
			case -90: choice_rotation_->SetSelection(3); break;
			default:  choice_rotation_->SetSelection(-1); break;
			}

			// Update patch colour controls
			switch (patch->blendType())
			{
			case CTPatchEx::BlendType::Translation:
				rb_pc_translation_->SetValue(true);
				enableTranslationControls(true);
				enableBlendControls(false);
				break;
			case CTPatchEx::BlendType::Blend:
				rb_pc_blend_->SetValue(true);
				enableBlendControls(true);
				enableTranslationControls(false);
				break;
			case CTPatchEx::BlendType::Tint:
				rb_pc_tint_->SetValue(true);
				enableBlendControls(true, true);
				enableTranslationControls(false);
				break;
			default:
				rb_pc_normal_->SetValue(true);
				enableTranslationControls(false);
				enableBlendControls(false);
				break;
			}
		}
		else
		{
			// Multiple selection, only enable some controls
			spin_patch_left_->Enable(false);
			spin_patch_top_->Enable(false);
		}
	}
}

// -----------------------------------------------------------------------------
// Prompts the user to select a patch from any open resources to be added to
// the current texture
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::addPatch()
{
	// Do nothing if no texture is open
	if (!tex_current_)
		return;

	// Browse for patch
	tx_editor_->setFullPath(true);
	auto patch = tx_editor_->browsePatchEntry();
	if (!patch.empty())
	{
		// Add new patch
		tex_current_->addPatch(patch, 0, 0);

		// Update UI
		populatePatchList();
		updatePatchControls();
	}

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Prompts the user to select a patch any open resource to replace selected
// patch(es) with
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::replacePatch()
{
	// Get selection
	auto selection = list_patches_->selectedItems();

	// Do nothing if no patches are selected
	if (selection.empty())
		return;

	// Browse for patch
	tx_editor_->setFullPath(true);
	auto patch = tx_editor_->browsePatchEntry();
	if (!patch.empty())
	{
		// Go through selection and replace each patch
		for (int index : selection)
			tex_current_->replacePatch(index, patch);
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
// Enables or disables patch translation controls depending on [enable]
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::enableTranslationControls(bool enable) const
{
	text_translation_->Enable(enable);
	btn_edit_translation_->Enable(enable);
}

// -----------------------------------------------------------------------------
// Enables or disables patch colour blend controls depending on [enable]
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::enableBlendControls(bool enable, bool tint) const
{
	cb_blend_col_->Enable(enable);
	spin_tint_amount_->Enable(enable && tint);
}


// -----------------------------------------------------------------------------
//
// ZTextureEditorPanel Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the 'Truecolour Preview' checkbox is (un)checked
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onCBBlendRGBAChanged(wxCommandEvent& e)
{
	// Set rgba blending
	tx_truecolour = cb_blend_rgba_->GetValue();
	tex_canvas_->setBlendRGBA(cb_blend_rgba_->GetValue());

	// Update UI
	tex_canvas_->redraw(true);
}

// -----------------------------------------------------------------------------
// Called when the 'Offset Type' dropdown menu selection is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onChoiceViewTypeSelected(wxCommandEvent& e)
{
	// Set offset type
	tx_offset_type = choice_viewtype_->GetSelection();
	tex_canvas_->setViewType(view_types[choice_viewtype_->GetSelection()]);

	// Update UI
	tex_canvas_->redraw(false);
}

// -----------------------------------------------------------------------------
// Called when the texture x scale spin control is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexScaleXChanged(wxCommandEvent& e)
{
	// Set texture's x scale
	if (tex_current_)
		tex_current_->setScaleX(spin_tex_scalex_->GetValue());

	// Update UI
	updateTextureScaleLabel();
	tex_canvas_->redraw();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture y scale spin control is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexScaleYChanged(wxCommandEvent& e)
{
	// Set texture's y scale
	if (tex_current_)
		tex_current_->setScaleY(spin_tex_scaley_->GetValue());

	// Update UI
	updateTextureScaleLabel();
	tex_canvas_->redraw();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture x offset spin control is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexOffsetXChanged(wxCommandEvent& e)
{
	// Set texture's x offset
	if (tex_current_)
		tex_current_->setOffsetX(spin_tex_offsetx_->GetValue());

	// Update UI
	tex_canvas_->redraw();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture y offset spin control is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexOffsetYChanged(wxCommandEvent& e)
{
	// Set texture's y offset
	if (tex_current_)
		tex_current_->setOffsetY(spin_tex_offsety_->GetValue());

	// Update UI
	tex_canvas_->redraw();

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the texture type dropdown menu selection is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexTypeChanged(wxCommandEvent& e)
{
	// Set texture's type
	if (tex_current_)
		tex_current_->setType(choice_type_->GetStringSelection().utf8_string());

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Optional' checkbox is (un)checked
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexOptionalChanged(wxCommandEvent& e)
{
	if (tex_current_)
		tex_current_->setOptional(cb_optional_->GetValue());

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'World Panning' checkbox is (un)checked
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexWorldPanningChanged(wxCommandEvent& e)
{
	if (tex_current_)
		tex_current_->setWorldPanning(cb_worldpanning_->GetValue());

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'No Decals' checkbox is (un)checked
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexNoDecalsChanged(wxCommandEvent& e)
{
	if (tex_current_)
		tex_current_->setNoDecals(cb_nodecals_->GetValue());

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Null Texture' checkbox is (un)checked
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTexNullTextureChanged(wxCommandEvent& e)
{
	if (tex_current_)
		tex_current_->setNullTexture(cb_nulltexture_->GetValue());

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Flip X' checkbox is (un)checked
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPatchFlipXChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto index = list_patches_->selectedItems()[a];
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(index));
		if (patch)
		{
			patch->setFlipX(cb_flipx_->GetValue());
			tex_canvas_->refreshPatch(index);
		}
	}

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Flip Y' checkbox is (un)checked
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPatchFlipYChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto index = list_patches_->selectedItems()[a];
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(index));
		if (patch)
		{
			patch->setFlipY(cb_flipy_->GetValue());
			tex_canvas_->refreshPatch(index);
		}
	}

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'FUse Offsets' checkbox is (un)checked
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPatchUseOfsChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
			patch->setUseOffsets(cb_useofs_->GetValue());
	}

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the patch rotation dropdown menu selection is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPatchRotationChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Get rotation value to apply
	int rot = 0;
	switch (choice_rotation_->GetSelection())
	{
	case 1:  rot = 90; break;
	case 2:  rot = 180; break;
	case 3:  rot = -90; break;
	default: rot = 0; break;
	}

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto index = list_patches_->selectedItems()[a];
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(index));
		if (patch)
		{
			patch->setRotation(rot);
			tex_canvas_->refreshPatch(index);
		}
	}

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the patch alpha spin control is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPatchAlphaChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
			patch->setAlpha(spin_alpha_->GetValue());
	}

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the patch transparency style dropdown menu selection is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPatchAlphaStyleChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
			patch->setStyle(choice_style_->GetStringSelection().utf8_string());
	}

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Normal' patch colour radio button is selected
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPCNormalSelected(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
			patch->setBlendType(CTPatchEx::BlendType::None);
	}

	// Update UI
	enableTranslationControls(false);
	enableBlendControls(false);
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Blend' patch colour radio button is selected
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPCBlendSelected(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
			patch->setBlendType(CTPatchEx::BlendType::Blend);
	}

	// Update UI
	enableTranslationControls(false);
	enableBlendControls(true);
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Tint' patch colour radio button is selected
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPCTintSelected(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
			patch->setBlendType(CTPatchEx::BlendType::Tint);
	}

	// Update UI
	enableTranslationControls(false);
	enableBlendControls(true, true);
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Translation' patch colour radio button is selected
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPCTranslationSelected(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
			patch->setBlendType(CTPatchEx::BlendType::Translation);
	}

	// Update UI
	enableTranslationControls(true);
	enableBlendControls(false);
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the patch colour picker is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPatchColourChanged(wxEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
		{
			ColRGBA col = cb_blend_col_->colour();
			patch->setColour(col.r, col.g, col.b, spin_tint_amount_->GetValue() * 255);
		}
	}

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the patch tint amount spin control is changed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onPatchTintAmountChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current_)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches_->selectedItems().size(); a++)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(list_patches_->selectedItems()[a]));
		if (patch)
		{
			ColRGBA col = cb_blend_col_->colour();
			patch->setColour(col.r, col.g, col.b, spin_tint_amount_->GetValue() * 255);
		}
	}

	// Update UI
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}

// -----------------------------------------------------------------------------
// Called when the 'Edit' translation button is pressed
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onBtnEditTranslation(wxCommandEvent& e)
{
	// Get selected patches
	auto selection = list_patches_->selectedItems();

	// Do nothing if no patches selected
	if (selection.empty())
		return;

	// Copy first translation found from selected patches
	Translation trans;
	for (auto i : selection)
	{
		auto patch = dynamic_cast<CTPatchEx*>(tex_current_->patch(selection[i]));
		if (patch->hasTranslation())
		{
			trans.copy(*patch->translation());
			break;
		}
	}

	// Add palette range if no translation ranges exist
	if (trans.nRanges() == 0)
		trans.addRange(TransRange::Type::Palette, 0);

	// Create patch image
	SImage image(SImage::Type::PalMask);
	tex_canvas_->texture()->loadPatchImage(selection[0], image, tx_editor_->archive(), tex_canvas_->palette());

	// Open translation editor dialog
	TranslationEditorDialog ted(maineditor::windowWx(), *tex_canvas_->palette(), "Edit Translation", &image);
	ted.openTranslation(trans);
	if (ted.ShowModal() == wxID_OK)
	{
		// Copy updated translation to all selected patches
		for (int index : selection)
		{
			auto patchx = dynamic_cast<CTPatchEx*>(tex_current_->patch(index));
			patchx->setTranslation(ted.getTranslation());
		}

		// Update UI
		updatePatchControls();
		tex_canvas_->redraw(true);

		tex_modified_ = true;
	}
}

// -----------------------------------------------------------------------------
// Called when the enter key is pressed in the translation text box
// -----------------------------------------------------------------------------
void ZTextureEditorPanel::onTextTranslationEnter(wxCommandEvent& e)
{
	// Parse translation text line
	Tokenizer tz;
	tz.openString(text_translation_->GetValue().utf8_string());
	Translation trans;

	auto token = tz.getToken();
	while (!token.empty())
	{
		// Parse the translation component
		trans.parse(token);

		// Check for desaturate translation
		if (trans.builtInName() == "Desaturate")
		{
			tz.skipToken(); // Skip ,
			trans.setDesaturationAmount(tz.getInteger());
			token = tz.getToken();
			continue;
		}

		// Skip ,
		if (tz.peekToken() == ",")
			tz.getToken();

		// Next component
		token = tz.getToken();
	}

	// Copy updated translation to all selected patches
	auto selection = list_patches_->selectedItems();
	for (int index : selection)
	{
		auto patchx = dynamic_cast<CTPatchEx*>(tex_current_->patch(index));
		patchx->setTranslation(trans);
	}

	// Update UI
	updatePatchControls();
	tex_canvas_->redraw(true);

	tex_modified_ = true;
}
