
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ZTextureEditorPanel.cpp
 * Description: The UI for viewing/editing an enhanced TEXTURES-
 *              format composite texture
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


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "ZTextureEditorPanel.h"
#include "Dialogs/TranslationEditorDialog.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "TextureXEditor.h"
#include "UI/Canvas/CTextureCanvas.h"


/*******************************************************************
 * ZTEXTUREEDITORPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ZTextureEditorPanel::ZTextureEditorPanel
 * ZTextureEditorPanel class constructor
 *******************************************************************/
ZTextureEditorPanel::ZTextureEditorPanel(wxWindow* parent, TextureXEditor* tx_editor)
	: TextureEditorPanel(parent, tx_editor)
{
}

/* ZTextureEditorPanel::~ZTextureEditorPanel
 * ZTextureEditorPanel class destructor
 *******************************************************************/
ZTextureEditorPanel::~ZTextureEditorPanel()
{
}

/* ZTextureEditorPanel::createTextureControls
 * Creates/sets up a panel with controls to edit texture properties
 *******************************************************************/
wxPanel* ZTextureEditorPanel::createTextureControls(wxWindow* parent)
{
	wxScrolledWindow* panel = new wxScrolledWindow(parent, -1);
	panel->SetScrollRate(4, 0);

	// Setup tex controls panel sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	panel->SetSizer(sizer);

	// "Texture Properties" frame
	wxStaticBox* frame = new wxStaticBox(panel, -1, "Texture Properties");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);

	// Name
	text_tex_name = new wxTextCtrl(panel, -1);
	text_tex_name->SetMaxLength(8);
	gb_sizer->Add(new wxStaticText(panel, -1, "Name:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_tex_name, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND);

	// Size
	spin_tex_width = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, 0, SHRT_MAX);
	spin_tex_height = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, 0, SHRT_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, "Size:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_width, wxGBPosition(1, 1), wxDefaultSpan);
	gb_sizer->Add(spin_tex_height, wxGBPosition(1, 2), wxDefaultSpan);

	// Scale
	spin_tex_scalex = new wxSpinCtrlDouble(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, 0.1, 100, 1, 0.1);
	spin_tex_scaley = new wxSpinCtrlDouble(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, 0.1, 100, 1, 0.1);
	gb_sizer->Add(new wxStaticText(panel, -1, "Scale:"), wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_scalex, wxGBPosition(2, 1), wxDefaultSpan);
	gb_sizer->Add(spin_tex_scaley, wxGBPosition(2, 2), wxDefaultSpan);

	// Scaled size
	label_scaled_size = new wxStaticText(panel, -1, "Scaled Size: N/A");
	gb_sizer->Add(label_scaled_size, wxGBPosition(3, 0), wxGBSpan(1, 2), wxALIGN_CENTER_VERTICAL);

	// Type
	string types[] = { "Texture", "Sprite", "Graphic", "WallTexture", "Flat" };
	choice_type = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 5, types);
	gb_sizer->Add(new wxStaticText(panel, -1, "Type:"), wxGBPosition(0, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_type, wxGBPosition(0, 4), wxGBSpan(1, 2), wxEXPAND);

	// Offsets
	spin_tex_offsetx = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, INT_MIN, INT_MAX);
	spin_tex_offsety = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, INT_MIN, INT_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, "Offsets:"), wxGBPosition(1, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_offsetx, wxGBPosition(1, 4), wxDefaultSpan);
	gb_sizer->Add(spin_tex_offsety, wxGBPosition(1, 5), wxDefaultSpan);

	// Flags
	cb_optional = new wxCheckBox(panel, -1, "Optional");
	cb_worldpanning = new wxCheckBox(panel, -1, "World Panning");
	cb_nodecals = new wxCheckBox(panel, -1, "No Decals");
	cb_nulltexture = new wxCheckBox(panel, -1, "Null Texture");
	gb_sizer->Add(cb_optional, wxGBPosition(2, 4), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_worldpanning, wxGBPosition(2, 5), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_nodecals, wxGBPosition(3, 4), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_nulltexture, wxGBPosition(3, 5), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);


	// Bind events
	spin_tex_scalex->Bind(wxEVT_SPINCTRLDOUBLE, &ZTextureEditorPanel::onTexScaleXChanged, this);
	spin_tex_scaley->Bind(wxEVT_SPINCTRLDOUBLE, &ZTextureEditorPanel::onTexScaleYChanged, this);
	spin_tex_scalex->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTexScaleXChanged, this);
	spin_tex_scaley->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTexScaleYChanged, this);
	spin_tex_offsetx->Bind(wxEVT_SPINCTRL, &ZTextureEditorPanel::onTexOffsetXChanged, this);
	spin_tex_offsety->Bind(wxEVT_SPINCTRL, &ZTextureEditorPanel::onTexOffsetYChanged, this);
	spin_tex_offsetx->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTexOffsetXChanged, this);
	spin_tex_offsety->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTexOffsetYChanged, this);
	choice_type->Bind(wxEVT_CHOICE, &ZTextureEditorPanel::onTexTypeChanged, this);
	cb_optional->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onTexOptionalChanged, this);
	cb_worldpanning->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onTexWorldPanningChanged, this);
	cb_nodecals->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onTexNoDecalsChanged, this);
	cb_nulltexture->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onTexNullTextureChanged, this);



	// This stuff should be in the constructor really, but can't (it crashes)

	// Show extra view options
	cb_blend_rgba->Show(true);
	choice_viewtype->Show(true);
	label_viewtype->Show(true);
	// Bind events
	cb_blend_rgba->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onCBBlendRGBAChanged, this);
	choice_viewtype->Bind(wxEVT_CHOICE, &ZTextureEditorPanel::onChoiceViewTypeSelected, this);


	return panel;
}

/* ZTextureEditorPanel::updateTextureControls
 * Updates all texture editing controls with values from the texture
 *******************************************************************/
void ZTextureEditorPanel::updateTextureControls()
{
	// Check texture is open
	if (!tex_current)
		return;

	text_tex_name->SetValue(tex_current->getName());
	spin_tex_width->SetValue(tex_current->getWidth());
	spin_tex_height->SetValue(tex_current->getHeight());
	spin_tex_scalex->SetValue(tex_current->getScaleX());
	spin_tex_scaley->SetValue(tex_current->getScaleY());
	spin_tex_offsetx->SetValue(tex_current->getOffsetX());
	spin_tex_offsety->SetValue(tex_current->getOffsetY());
	updateTextureScaleLabel();

	// Update type
	if (tex_current->getType() == "Texture")
		choice_type->SetSelection(0);
	else if (tex_current->getType() == "Sprite")
		choice_type->SetSelection(1);
	else if (tex_current->getType() == "Graphic")
		choice_type->SetSelection(2);
	else if (tex_current->getType() == "WallTexture")
		choice_type->SetSelection(3);
	else if (tex_current->getType() == "Flat")
		choice_type->SetSelection(4);
	else
		choice_type->SetSelection(0);

	// Update flags
	cb_optional->SetValue(tex_current->isOptional());
	cb_worldpanning->SetValue(tex_current->worldPanning());
	cb_nodecals->SetValue(tex_current->noDecals());
	cb_nulltexture->SetValue(tex_current->nullTexture());
}

/* ZTextureEditorPanel::createPatchControls
 * Creates/sets up a panel with controls to edit a texture's patches
 *******************************************************************/
wxPanel* ZTextureEditorPanel::createPatchControls(wxWindow* parent)
{
	wxScrolledWindow* panel = new wxScrolledWindow(parent, -1);
	panel->SetScrollRate(0, 4);

	// Setup panel sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// -- Texture Patches frame --
	wxStaticBox* frame = new wxStaticBox(panel, -1, "Patches");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	// Add patches list
	list_patches = new ListView(panel, -1);
	list_patches->enableSizeUpdate(false);
	list_patches->SetInitialSize(wxSize(100, -1));
	framesizer->Add(list_patches, 1, wxEXPAND|wxALL, 4);

	// Add patch buttons
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxBOTTOM, 4);

	// 'Add' button
	btn_patch_add = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_add"));
	btn_patch_add->SetToolTip("Add new patch to texture");
	gb_sizer->Add(btn_patch_add, wxGBPosition(0, 0));

	// 'Remove' button
	btn_patch_remove = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_remove"));
	btn_patch_remove->SetToolTip("Remove selected patch(es) from texture");
	gb_sizer->Add(btn_patch_remove, wxGBPosition(0, 1));

	// 'Back' button
	btn_patch_back = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_back"));
	btn_patch_back->SetToolTip("Send selected patch(es) back");
	gb_sizer->Add(btn_patch_back, wxGBPosition(1, 0));

	// 'Forward' button
	btn_patch_forward = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_forward"));
	btn_patch_forward->SetToolTip("Bring selected patch(es) forward");
	gb_sizer->Add(btn_patch_forward, wxGBPosition(1, 1));

	// 'Replace' button
	btn_patch_replace = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_replace"));
	btn_patch_replace->SetToolTip("Replace selected patch(es)");
	gb_sizer->Add(btn_patch_replace, wxGBPosition(2, 0));

	// 'Duplicate' button
	btn_patch_duplicate = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_duplicate"));
	btn_patch_duplicate->SetToolTip("Duplicate selected patch(es)");
	gb_sizer->Add(btn_patch_duplicate, wxGBPosition(2, 1));



	// -- Patch Properties frame --
	frame = new wxStaticBox(panel, -1, "Patch Properties");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);

	// X Position
	spin_patch_left = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, SHRT_MIN, SHRT_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, "X Position:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_patch_left, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Y Position
	spin_patch_top = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, SHRT_MIN, SHRT_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, "Y Position:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_patch_top, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Use Offsets
	cb_useofs = new wxCheckBox(panel, -1, "Use Source Gfx Offsets");
	gb_sizer->Add(cb_useofs, wxGBPosition(2, 0), wxGBSpan(1, 2), wxALIGN_CENTER_VERTICAL);

	// Flip X
	cb_flipx = new wxCheckBox(panel, -1, "Flip X");
	gb_sizer->Add(cb_flipx, wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

	// Flip Y
	cb_flipy = new wxCheckBox(panel, -1, "Flip Y");
	gb_sizer->Add(cb_flipy, wxGBPosition(3, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

	// Rotation
	string rotval[] = { "0", "90", "180", "270" };
	choice_rotation = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 4, rotval);
	choice_rotation->SetSelection(0);
	gb_sizer->Add(new wxStaticText(panel, -1, "Rotation:"), wxGBPosition(4, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_rotation, wxGBPosition(4, 1), wxDefaultSpan, wxEXPAND);

	// Alpha
	spin_alpha = new wxSpinCtrlDouble(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, 0, 1, 1, 0.1);
	gb_sizer->Add(new wxStaticText(panel, -1, "Alpha:"), wxGBPosition(5, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_alpha, wxGBPosition(5, 1), wxDefaultSpan, wxEXPAND);

	// Alpha Style
	string styles[] = { "Copy", "Translucent", "Add", "Subtract", "ReverseSubtract", "Modulate", "CopyAlpha", "CopyNewAlpha", "Overlay" };
	choice_style = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 9, styles);
	choice_style->SetSelection(0);
	gb_sizer->Add(new wxStaticText(panel, -1, "Alpha Style:"), wxGBPosition(6, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_style, wxGBPosition(6, 1), wxGBSpan(1, 2), wxEXPAND);

	frame = new wxStaticBox(panel, -1, "Patch Colour");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	gb_sizer = new wxGridBagSizer(4, 4);
	framesizer->Add(gb_sizer, 1, wxEXPAND|wxALL, 4);
	gb_sizer->AddGrowableCol(0, 1);
	gb_sizer->AddGrowableCol(1, 1);

	// 'Normal' colour
	rb_pc_normal = new wxRadioButton(panel, -1, "Normal", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	gb_sizer->Add(rb_pc_normal, wxGBPosition(0, 0), wxGBSpan(1, 2), wxALIGN_CENTER_VERTICAL);

	gb_sizer->Add(new wxStaticLine(panel, -1), wxGBPosition(1, 0), wxGBSpan(1, 2), wxEXPAND);

	// Blend
	rb_pc_blend = new wxRadioButton(panel, -1, "Blend");
	rb_pc_tint = new wxRadioButton(panel, -1, "Tint");
	gb_sizer->Add(rb_pc_blend, wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(rb_pc_tint, wxGBPosition(2, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

	// Blend/Tint colour
	cb_blend_col = new ColourBox(panel, -1, false, true);
	cb_blend_col->setPalette(getPalette());
	gb_sizer->Add(new wxStaticText(panel, -1, "Colour:"), wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_blend_col, wxGBPosition(3, 1), wxDefaultSpan, wxALIGN_RIGHT);

	// Tint amount
	spin_tint_amount = new wxSpinCtrlDouble(panel, 01, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, 0, 1, 0, 0.1);
	gb_sizer->Add(new wxStaticText(panel, -1, "Amount:"), wxGBPosition(4, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tint_amount, wxGBPosition(4, 1), wxDefaultSpan, wxEXPAND);

	gb_sizer->Add(new wxStaticLine(panel, -1), wxGBPosition(5, 0), wxGBSpan(1, 2), wxEXPAND);

	// Translation
	rb_pc_translation = new wxRadioButton(panel, -1, "Translation");
	gb_sizer->Add(rb_pc_translation, wxGBPosition(6, 0), wxGBSpan(1, 2), wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	gb_sizer->Add(hbox, wxGBPosition(7, 0), wxGBSpan(1, 2), wxEXPAND);

	// Translation text entry
	text_translation = new wxTextCtrl(panel, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	hbox->Add(text_translation, 1, wxEXPAND|wxRIGHT, 4);

	// Translation edit button
	btn_edit_translation = new wxButton(panel, -1, "Edit", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	hbox->Add(btn_edit_translation);


	// Bind events
	cb_flipx->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onPatchFlipXChanged, this);
	cb_flipy->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onPatchFlipYChanged, this);
	cb_useofs->Bind(wxEVT_CHECKBOX, &ZTextureEditorPanel::onPatchUseOfsChanged, this);
	choice_rotation->Bind(wxEVT_CHOICE, &ZTextureEditorPanel::onPatchRotationChanged, this);
	spin_alpha->Bind(wxEVT_SPINCTRLDOUBLE, &ZTextureEditorPanel::onPatchAlphaChanged, this);
	spin_alpha->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onPatchAlphaChanged, this);
	choice_style->Bind(wxEVT_CHOICE, &ZTextureEditorPanel::onPatchAlphaStyleChanged, this);
	rb_pc_normal->Bind(wxEVT_RADIOBUTTON, &ZTextureEditorPanel::onPCNormalSelected, this);
	rb_pc_blend->Bind(wxEVT_RADIOBUTTON, &ZTextureEditorPanel::onPCBlendSelected, this);
	rb_pc_tint->Bind(wxEVT_RADIOBUTTON, &ZTextureEditorPanel::onPCTintSelected, this);
	rb_pc_translation->Bind(wxEVT_RADIOBUTTON, &ZTextureEditorPanel::onPCTranslationSelected, this);
	cb_blend_col->Bind(wxEVT_COLOURBOX_CHANGED, &ZTextureEditorPanel::onPatchColourChanged, this);
	spin_tint_amount->Bind(wxEVT_SPINCTRLDOUBLE, &ZTextureEditorPanel::onPatchTintAmountChanged, this);
	spin_tint_amount->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onPatchTintAmountChanged, this);
	btn_edit_translation->Bind(wxEVT_BUTTON, &ZTextureEditorPanel::onBtnEditTranslation, this);
	text_translation->Bind(wxEVT_TEXT_ENTER, &ZTextureEditorPanel::onTextTranslationEnter, this);

	return panel;
}

/* ZTextureEditorPanel::updatePatchControls
 * Updates all patch editing controls with values from the currently
 * selected patch. Behaves differently depending on the number of
 * patches selected
 *******************************************************************/
void ZTextureEditorPanel::updatePatchControls()
{
	// Get selected patches
	wxArrayInt selection = list_patches->selectedItems();

	// If nothing is selected, disable patch controls
	if (selection.size() == 0)
	{
		spin_patch_left->Enable(false);
		spin_patch_top->Enable(false);
		cb_flipx->Enable(false);
		cb_flipy->Enable(false);
		cb_useofs->Enable(false);
		choice_rotation->Enable(false);
		spin_alpha->Enable(false);
		choice_style->Enable(false);
		rb_pc_normal->Enable(false);
		rb_pc_blend->Enable(false);
		rb_pc_tint->Enable(false);
		rb_pc_translation->Enable(false);
		cb_blend_col->Enable(false);
		spin_tint_amount->Enable(false);
		text_translation->Enable(false);
		btn_edit_translation->Enable(false);
	}
	else
	{
		// Something is selected, enable the controls
		spin_patch_left->Enable(true);
		spin_patch_top->Enable(true);
		cb_flipx->Enable(true);
		cb_flipy->Enable(true);
		cb_useofs->Enable(true);
		choice_rotation->Enable(true);
		spin_alpha->Enable(true);
		choice_style->Enable(true);
		rb_pc_normal->Enable(true);
		rb_pc_blend->Enable(true);
		rb_pc_tint->Enable(true);
		rb_pc_translation->Enable(true);
		cb_blend_col->Enable(true);
		spin_tint_amount->Enable(true);
		text_translation->Enable(true);
		btn_edit_translation->Enable(true);

		// If only 1 patch is selected, just set the controls to this patch
		if (selection.size() == 1)
		{
			CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(selection[0]);
			if (!patch)
			{
				LOG_MESSAGE(1, "Error: Selected patch does not exist in texture");
				return;
			}

			spin_patch_left->SetValue(patch->xOffset());
			spin_patch_top->SetValue(patch->yOffset());
			cb_flipx->SetValue(patch->flipX());
			cb_flipy->SetValue(patch->flipY());
			cb_useofs->SetValue(patch->useOffsets());
			spin_alpha->SetValue(patch->getAlpha());
			choice_style->SetStringSelection(patch->getStyle());
			cb_blend_col->setColour(patch->getColour());
			spin_tint_amount->SetValue((double)patch->getColour().a / 255.0);
			text_translation->SetValue(patch->getTranslation().asText());

			switch (patch->getRotation())
			{
			case 0: choice_rotation->SetSelection(0); break;
			case 90: choice_rotation->SetSelection(1); break;
			case 180: choice_rotation->SetSelection(2); break;
			case -90: choice_rotation->SetSelection(3); break;
			default: choice_rotation->SetSelection(-1); break;
			};

			// Update patch colour controls
			switch (patch->getBlendType())
			{
			case 1:
				rb_pc_translation->SetValue(true);
				enableTranslationControls(true);
				enableBlendControls(false);
				break;
			case 2:
				rb_pc_blend->SetValue(true);
				enableBlendControls(true);
				enableTranslationControls(false);
				break;
			case 3:
				rb_pc_tint->SetValue(true);
				enableBlendControls(true, true);
				enableTranslationControls(false);
				break;
			default:
				rb_pc_normal->SetValue(true);
				enableTranslationControls(false);
				enableBlendControls(false);
				break;
			}
		}
		else
		{
			// Multiple selection, only enable some controls
			spin_patch_left->Enable(false);
			spin_patch_top->Enable(false);
		}
	}
}

/* ZTextureEditorPanel::addPatch
 * Prompts the user to select a patch from any open resources to be
 * added to the current texture
 *******************************************************************/
void ZTextureEditorPanel::addPatch()
{
	// Do nothing if no texture is open
	if (!tex_current)
		return;

	// Browse for patch
	string patch = tx_editor->browsePatchEntry();
	if (!patch.IsEmpty())
	{
		// Add new patch
		tex_current->addPatch(patch, 0, 0);

		// Update UI
		populatePatchList();
		updatePatchControls();
	}

	tex_modified = true;
}

/* ZTextureEditorPanel::replacePatch
 * Prompts the user to select a patch any open resource to replace
 * selectes patch(es) with
 *******************************************************************/
void ZTextureEditorPanel::replacePatch()
{
	// Get selection
	wxArrayInt selection = list_patches->selectedItems();

	// Do nothing if no patches are selected
	if (selection.size() == 0)
		return;

	// Browse for patch
	string patch = tx_editor->browsePatchEntry();
	if (!patch.IsEmpty())
	{
		// Go through selection and replace each patch
		for (size_t a = 0; a < selection.size(); a++)
			tex_current->replacePatch(selection[a], patch);
	}

	// Repopulate patch list
	populatePatchList();

	// Restore selection
	for (size_t a = 0; a < selection.size(); a++)
		list_patches->selectItem(selection[a]);

	// Update UI
	updatePatchControls();

	tex_modified = true;
}

void ZTextureEditorPanel::enableTranslationControls(bool enable)
{
	text_translation->Enable(enable); btn_edit_translation->Enable(enable);
}

void ZTextureEditorPanel::enableBlendControls(bool enable, bool tint)
{
	cb_blend_col->Enable(enable); spin_tint_amount->Enable(enable && tint);
}


/*******************************************************************
 * ZTEXTUREEDITORPANEL CLASS EVENTS
 *******************************************************************/

/* ZTextureEditorPanel::onCBBlendRGBAChanged
 * Called when the 'Truecolour Preview' checkbox is (un)checked
 *******************************************************************/
void ZTextureEditorPanel::onCBBlendRGBAChanged(wxCommandEvent& e)
{
	// Set rgba blending
	tex_canvas->blendRGBA(cb_blend_rgba->GetValue());

	// Update UI
	tex_canvas->redraw(true);
}

/* ZTextureEditorPanel::onChoiceViewTypeSelected
 * Called when the 'Offset Type' dropdown menu selection is changed
 *******************************************************************/
void ZTextureEditorPanel::onChoiceViewTypeSelected(wxCommandEvent& e)
{
	// Set offset type
	tex_canvas->setViewType(choice_viewtype->GetSelection());

	// Update UI
	tex_canvas->redraw(false);
}

/* ZTextureEditorPanel::onTexScaleXChanged
 * Called when the texture x scale spin control is changed
 *******************************************************************/
void ZTextureEditorPanel::onTexScaleXChanged(wxCommandEvent& e)
{
	// Set texture's x scale
	if (tex_current)
		tex_current->setScaleX(spin_tex_scalex->GetValue());

	// Update UI
	updateTextureScaleLabel();
	tex_canvas->redraw();

	tex_modified = true;
}

/* ZTextureEditorPanel::onTexScaleYChanged
 * Called when the texture y scale spin control is changed
 *******************************************************************/
void ZTextureEditorPanel::onTexScaleYChanged(wxCommandEvent& e)
{
	// Set texture's y scale
	if (tex_current)
		tex_current->setScaleY(spin_tex_scaley->GetValue());

	// Update UI
	updateTextureScaleLabel();
	tex_canvas->redraw();

	tex_modified = true;
}

/* ZTextureEditorPanel::onTexOffsetXChanged
 * Called when the texture x offset spin control is changed
 *******************************************************************/
void ZTextureEditorPanel::onTexOffsetXChanged(wxCommandEvent& e)
{
	// Set texture's x offset
	if (tex_current)
		tex_current->setOffsetX(spin_tex_offsetx->GetValue());

	// Update UI
	tex_canvas->redraw();

	tex_modified = true;
}

/* ZTextureEditorPanel::onTexOffsetYChanged
 * Called when the texture y offset spin control is changed
 *******************************************************************/
void ZTextureEditorPanel::onTexOffsetYChanged(wxCommandEvent& e)
{
	// Set texture's y offset
	if (tex_current)
		tex_current->setOffsetY(spin_tex_offsety->GetValue());

	// Update UI
	tex_canvas->redraw();

	tex_modified = true;
}

/* ZTextureEditorPanel::onTexTypeChanged
 * Called when the texture type dropdown menu selection is changed
 *******************************************************************/
void ZTextureEditorPanel::onTexTypeChanged(wxCommandEvent& e)
{
	// Set texture's type
	if (tex_current)
		tex_current->setType(choice_type->GetStringSelection());

	tex_modified = true;
}

/* ZTextureEditorPanel::onTexOptionalChanged
 * Called when the 'Optional' checkbox is (un)checked
 *******************************************************************/
void ZTextureEditorPanel::onTexOptionalChanged(wxCommandEvent& e)
{
	if (tex_current)
		tex_current->setOptional(cb_optional->GetValue());

	tex_modified =  true;
}

/* ZTextureEditorPanel::onTexWorldPanningChanged
 * Called when the 'World Panning' checkbox is (un)checked
 *******************************************************************/
void ZTextureEditorPanel::onTexWorldPanningChanged(wxCommandEvent& e)
{
	if (tex_current)
		tex_current->setWorldPanning(cb_worldpanning->GetValue());

	tex_modified =  true;
}

/* ZTextureEditorPanel::onTexNoDecalsChanged
 * Called when the 'No Decals' checkbox is (un)checked
 *******************************************************************/
void ZTextureEditorPanel::onTexNoDecalsChanged(wxCommandEvent& e)
{
	if (tex_current)
		tex_current->setNoDecals(cb_nodecals->GetValue());

	tex_modified =  true;
}

/* ZTextureEditorPanel::onTexNullTextureChanged
 * Called when the 'Null Texture' checkbox is (un)checked
 *******************************************************************/
void ZTextureEditorPanel::onTexNullTextureChanged(wxCommandEvent& e)
{
	if (tex_current)
		tex_current->setNullTexture(cb_nulltexture->GetValue());

	tex_modified =  true;
}

/* ZTextureEditorPanel::onPatchFlipXChanged
 * Called when the 'Flip X' checkbox is (un)checked
 *******************************************************************/
void ZTextureEditorPanel::onPatchFlipXChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->flipX(cb_flipx->GetValue());
	}

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPatchFlipYChanged
 * Called when the 'Flip Y' checkbox is (un)checked
 *******************************************************************/
void ZTextureEditorPanel::onPatchFlipYChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->flipY(cb_flipy->GetValue());
	}

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPatchUseOfsChanged
 * Called when the 'FUse Offsets' checkbox is (un)checked
 *******************************************************************/
void ZTextureEditorPanel::onPatchUseOfsChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->useOffsets(cb_useofs->GetValue());
	}

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPatchRotationChanged
 * Called when the patch rotation dropdown menu selection is changed
 *******************************************************************/
void ZTextureEditorPanel::onPatchRotationChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Get rotation value to apply
	int rot = 0;
	switch (choice_rotation->GetSelection())
	{
	case 1: rot = 90; break;
	case 2: rot = 180; break;
	case 3: rot = -90; break;
	default: rot = 0; break;
	};

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->setRotation(rot);
	}

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPatchAlphaChanged
 * Called when the patch alpha spin control is changed
 *******************************************************************/
void ZTextureEditorPanel::onPatchAlphaChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->setAlpha(spin_alpha->GetValue());
	}

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPatchAlphaStyleChanged
 * Called when the patch transparency style dropdown menu selection
 * is changed
 *******************************************************************/
void ZTextureEditorPanel::onPatchAlphaStyleChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->setStyle(choice_style->GetStringSelection());
	}

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPCNormalSelected
 * Called when the 'Normal' patch colour radio button is selected
 *******************************************************************/
void ZTextureEditorPanel::onPCNormalSelected(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->setBlendType(0);
	}

	// Update UI
	enableTranslationControls(false);
	enableBlendControls(false);
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPCBlendSelected
 * Called when the 'Blend' patch colour radio button is selected
 *******************************************************************/
void ZTextureEditorPanel::onPCBlendSelected(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->setBlendType(2);
	}

	// Update UI
	enableTranslationControls(false);
	enableBlendControls(true);
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPCTintSelected
 * Called when the 'Tint' patch colour radio button is selected
 *******************************************************************/
void ZTextureEditorPanel::onPCTintSelected(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->setBlendType(3);
	}

	// Update UI
	enableTranslationControls(false);
	enableBlendControls(true, true);
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPCTranslationSelected
 * Called when the 'Translation' patch colour radio button is
 * selected
 *******************************************************************/
void ZTextureEditorPanel::onPCTranslationSelected(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
			patch->setBlendType(1);
	}

	// Update UI
	enableTranslationControls(true);
	enableBlendControls(false);
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPatchColourChanged
 * Called when the patch colour picker is changed
 *******************************************************************/
void ZTextureEditorPanel::onPatchColourChanged(wxEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
		{
			rgba_t col = cb_blend_col->getColour();
			patch->setColour(col.r, col.g, col.b, spin_tint_amount->GetValue()*255);
		}
	}

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onPatchTintAmountChanged
 * Called when the patch tint amount spin control is changed
 *******************************************************************/
void ZTextureEditorPanel::onPatchTintAmountChanged(wxCommandEvent& e)
{
	// Check texture is open
	if (!tex_current)
		return;

	// Go through selected patches
	for (unsigned a = 0; a < list_patches->selectedItems().size(); a++)
	{
		CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(list_patches->selectedItems()[a]);
		if (patch)
		{
			rgba_t col = cb_blend_col->getColour();
			patch->setColour(col.r, col.g, col.b, spin_tint_amount->GetValue()*255);
		}
	}

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* ZTextureEditorPanel::onBtnEditTranslation
 * Called when the 'Edit' translation button is pressed
 *******************************************************************/
void ZTextureEditorPanel::onBtnEditTranslation(wxCommandEvent& e)
{
	// Get selected patches
	wxArrayInt selection = list_patches->selectedItems();

	// Do nothing if no patches selected
	if (selection.size() == 0)
		return;

	// Get translation from first selected patch
	Translation trans;
	CTPatchEx* patch = (CTPatchEx*)tex_current->getPatch(selection[0]);
	trans.copy(patch->getTranslation());

	// Add palette range if no translation ranges exist
	if (trans.nRanges() == 0)
		trans.addRange(TRANS_PALETTE, 0);

	// Create patch image
	SImage image(PALMASK);
	tex_canvas->getTexture()->loadPatchImage(selection[0], image, tx_editor->getArchive(), tex_canvas->getPalette());

	// Open translation editor dialog
	TranslationEditorDialog ted(MainEditor::windowWx(), tex_canvas->getPalette(), "Edit Translation", &image);
	ted.openTranslation(trans);
	if (ted.ShowModal() == wxID_OK)
	{
		// Copy updated translation to all selected patches
		for (unsigned a = 0; a < selection.size(); a++)
		{
			CTPatchEx* patchx = (CTPatchEx*)tex_current->getPatch(selection[a]);
			patchx->getTranslation().copy(ted.getTranslation());
		}

		// Update UI
		updatePatchControls();
		tex_canvas->redraw(true);

		tex_modified = true;
	}
}

/* ZTextureEditorPanel::onTextTranslationEnter
 * Called when the enter key is pressed in the translation text box
 *******************************************************************/
void ZTextureEditorPanel::onTextTranslationEnter(wxCommandEvent& e)
{
	// Parse translation text line
	Tokenizer tz;
	tz.openString(text_translation->GetValue());
	Translation trans;

	string token = tz.getToken();
	while (!token.IsEmpty())
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
	wxArrayInt selection = list_patches->selectedItems();
	for (unsigned a = 0; a < selection.size(); a++)
	{
		CTPatchEx* patchx = (CTPatchEx*)tex_current->getPatch(selection[a]);
		patchx->getTranslation().copy(trans);
	}

	// Update UI
	updatePatchControls();
	tex_canvas->redraw(true);

	tex_modified = true;
}
