
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TextureEditorPanel.cpp
 * Description: The UI for viewing/editing a composite texture
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
#include "TextureEditorPanel.h"
#include "General/KeyBind.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/Icons.h"
#include "App.h"
#include "TextureXEditor.h"
#include "UI/Canvas/CTextureCanvas.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
bool hack_nodrag = false;	// Hack to stop the drag event being erroneously triggered when
// double-clicking a patch in the patch browser to select it
EXTERN_CVAR(Bool, tx_arc)


/*******************************************************************
 * TEXTUREEDITORPANEL CLASS FUNCTIONS
 *******************************************************************/

/* TextureEditorPanel::TextureEditorPanel
 * TextureEditorPanel class constructor
 *******************************************************************/
TextureEditorPanel::TextureEditorPanel(wxWindow* parent, TextureXEditor* tx_editor) : wxPanel(parent, -1)
{
	// Init variables
	this->tx_editor = tx_editor;
	tex_current = NULL;
	alt_press = false;
}

/* TextureEditorPanel::~TextureEditorPanel
 * TextureEditorPanel class destructor
 *******************************************************************/
TextureEditorPanel::~TextureEditorPanel()
{
}

/* TextureEditorPanel::setupLayout
 * Initialises the panel layout (must be called after the constructor
 * to work correctly for ZTextureEditorPanel)
 *******************************************************************/
void TextureEditorPanel::setupLayout()
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Setup left section (view controls + texture canvas + texture controls)
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND);

	// Add view controls
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 4);

	// Zoom
	slider_zoom = new wxSlider(this, -1, 100, 20, 800);
	slider_zoom->SetLineSize(10);
	slider_zoom->SetPageSize(100);
	label_current_zoom = new wxStaticText(this, -1, "100%");
	hbox->Add(new wxStaticText(this, -1, "Zoom:"), 0, wxALIGN_CENTER_VERTICAL, 0);
	hbox->Add(slider_zoom, 1, wxEXPAND, 0);
	hbox->Add(label_current_zoom, 0, wxALIGN_CENTER_VERTICAL, 0);

	hbox->AddStretchSpacer();

	// 'Apply Scale' checkbox
	cb_tex_scale = new wxCheckBox(this, -1, "Apply Scale");
	cb_tex_scale->SetValue(false);
	hbox->Add(cb_tex_scale, 0, wxEXPAND|wxRIGHT, 4);

	// 'Aspect Ratio Correction' checkbox
	cb_tex_arc = new wxCheckBox(this, -1, "Aspect Ratio Correction");
	cb_tex_arc->SetValue(tx_arc);
	hbox->Add(cb_tex_arc, 0, wxEXPAND|wxRIGHT, 4);

	// 'Show Outside' checkbox
	cb_draw_outside = new wxCheckBox(this, -1, "Show Outside");
	cb_draw_outside->SetValue(true);
	hbox->Add(cb_draw_outside, 0, wxEXPAND);

	// Add texture canvas
	tex_canvas = new CTextureCanvas(this, -1);
	tex_canvas->setViewType(0);
	vbox->Add(tex_canvas, 1, wxEXPAND|wxALL, 4);

	// Add extra view controls
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);

	// Offset view type menu
	string otypes[] = { "None", "Sprite", "HUD" };
	choice_viewtype = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, otypes);
	label_viewtype = new wxStaticText(this, -1, "Offset Type:");
	hbox->Add(label_viewtype, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(choice_viewtype, 0, wxEXPAND | wxRIGHT, 8);
	choice_viewtype->SetSelection(0);
	choice_viewtype->Show(false); // Only show this on ZTextureEditorPanel
	label_viewtype->Show(false);

	hbox->AddStretchSpacer();

	// 'Truecolour Preview' checkbox
	cb_blend_rgba = new wxCheckBox(this, -1, "Truecolour Preview");
	cb_blend_rgba->SetValue(false);
	hbox->Add(cb_blend_rgba, 0, wxEXPAND);
	cb_blend_rgba->Show(false);	// Only show this on ZTextureEditorPanel

	// Add texture controls
	vbox->Add(createTextureControls(this), 0, wxEXPAND);


	// Setup right section (patch controls)
	vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 0, wxEXPAND);

	// Add patch controls
	vbox->Add(createPatchControls(this), 1, wxEXPAND);


	// Bind events
	slider_zoom->Bind(wxEVT_SLIDER, &TextureEditorPanel::onZoomChanged, this);
	cb_draw_outside->Bind(wxEVT_CHECKBOX, &TextureEditorPanel::onDrawOutsideChanged, this);
	tex_canvas->Bind(wxEVT_LEFT_DOWN, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas->Bind(wxEVT_LEFT_DCLICK, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas->Bind(wxEVT_LEFT_UP, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas->Bind(wxEVT_RIGHT_UP, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas->Bind(wxEVT_MOTION, &TextureEditorPanel::onTexCanvasMouseEvent, this);
	tex_canvas->Bind(EVT_DRAG_END, &TextureEditorPanel::onTexCanvasDragEnd, this);
	tex_canvas->Bind(wxEVT_KEY_DOWN, &TextureEditorPanel::onTexCanvasKeyDown, this);
	text_tex_name->Bind(wxEVT_TEXT, &TextureEditorPanel::onTexNameChanged, this);
	spin_tex_width->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexWidthChanged, this);
	spin_tex_height->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexHeightChanged, this);
	spin_tex_width->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onTexWidthChanged, this);
	spin_tex_height->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onTexHeightChanged, this);
	list_patches->Bind(wxEVT_LIST_ITEM_SELECTED, &TextureEditorPanel::onPatchListSelect, this);
	list_patches->Bind(wxEVT_LIST_ITEM_DESELECTED, &TextureEditorPanel::onPatchListDeSelect, this);
	btn_patch_add->Bind(wxEVT_BUTTON, &TextureEditorPanel::onBtnPatchAdd, this);
	btn_patch_remove->Bind(wxEVT_BUTTON, &TextureEditorPanel::onBtnPatchRemove, this);
	btn_patch_back->Bind(wxEVT_BUTTON, &TextureEditorPanel::onBtnPatchBack, this);
	btn_patch_forward->Bind(wxEVT_BUTTON, &TextureEditorPanel::onBtnPatchForward, this);
	btn_patch_replace->Bind(wxEVT_BUTTON, &TextureEditorPanel::onBtnPatchReplace, this);
	btn_patch_duplicate->Bind(wxEVT_BUTTON, &TextureEditorPanel::onBtnPatchDuplicate, this);
	spin_patch_left->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onPatchPositionXChanged, this);
	spin_patch_top->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onPatchPositionYChanged, this);
	spin_patch_left->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onPatchPositionXChanged, this);
	spin_patch_top->Bind(wxEVT_TEXT_ENTER, &TextureEditorPanel::onPatchPositionYChanged, this);
	cb_tex_scale->Bind(wxEVT_CHECKBOX, &TextureEditorPanel::onApplyScaleChanged, this);
	cb_tex_arc->Bind(wxEVT_CHECKBOX, &TextureEditorPanel::onARCChanged, this);

	// Init layout
	Layout();
}

/* TextureEditorPanel::createTextureControls
 * Creates/sets up a panel with controls to edit texture properties
 *******************************************************************/
wxPanel* TextureEditorPanel::createTextureControls(wxWindow* parent)
{
	wxPanel* panel = new wxPanel(parent, -1);

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
	spin_tex_scalex = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, 0, UCHAR_MAX);
	spin_tex_scaley = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, 0, UCHAR_MAX);
	gb_sizer->Add(new wxStaticText(panel, -1, "Scale:"), wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tex_scalex, wxGBPosition(2, 1), wxDefaultSpan);
	gb_sizer->Add(spin_tex_scaley, wxGBPosition(2, 2), wxDefaultSpan);

	// Scaled size
	label_scaled_size = new wxStaticText(panel, -1, "Scaled Size: N/A");
	gb_sizer->Add(label_scaled_size, wxGBPosition(3, 0), wxGBSpan(1, 2), wxALIGN_CENTER_VERTICAL);

	// World panning
	cb_tex_world_panning = new wxCheckBox(panel, -1, "World Panning");
	gb_sizer->Add(cb_tex_world_panning, wxGBPosition(3, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);


	// Bind events
	spin_tex_scalex->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexScaleXChanged, this);
	spin_tex_scaley->Bind(wxEVT_SPINCTRL, &TextureEditorPanel::onTexScaleYChanged, this);
	cb_tex_world_panning->Bind(wxEVT_CHECKBOX, &TextureEditorPanel::onTexWorldPanningChanged, this);

	return panel;
}

/* TextureEditorPanel::updateTextureControls
 * Updates all texture editing controls with values from the texture
 *******************************************************************/
void TextureEditorPanel::updateTextureControls()
{
	// Check texture is open
	if (!tex_current)
		return;

	bool modified = tex_modified;
	text_tex_name->SetValue(tex_current->getName());
	spin_tex_width->SetValue(tex_current->getWidth());
	spin_tex_height->SetValue(tex_current->getHeight());
	spin_tex_scalex->SetValue(tex_current->getScaleX()*8);
	spin_tex_scaley->SetValue(tex_current->getScaleY()*8);
	cb_tex_world_panning->SetValue(tex_current->worldPanning());
	updateTextureScaleLabel();
	tex_modified = modified;
}

/* TextureEditorPanel::updateTextureScaleLabel
 * Updates the 'Scaled Size' label according to the current texture's
 * dimensions and scale
 *******************************************************************/
void TextureEditorPanel::updateTextureScaleLabel()
{
	// Check texture is open
	if (!tex_current)
		return;

	// Determine scaled X value
	uint32_t scaled_x = tex_current->getWidth();
	if (tex_current->getScaleX() != 0)
		scaled_x /= tex_current->getScaleX();

	// Determine scaled Y value
	uint32_t scaled_y = tex_current->getHeight();
	if (tex_current->getScaleY() != 0)
		scaled_y /= tex_current->getScaleY();

	// Update the label
	label_scaled_size->SetLabel(S_FMT("Scaled Size: %dx%d", scaled_x, scaled_y));
}

/* TextureEditorPanel::createPatchControls
 * Creates/sets up a panel with controls to edit a texture's patches
 *******************************************************************/
wxPanel* TextureEditorPanel::createPatchControls(wxWindow* parent)
{
	wxPanel* panel = new wxPanel(parent, -1);

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
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	framesizer->Add(vbox, 0, wxEXPAND|wxTOP|wxRIGHT|wxBOTTOM, 4);

	// 'Add' button
	btn_patch_add = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_add"));
	btn_patch_add->SetToolTip("Add new patch to texture");
	vbox->Add(btn_patch_add, 0, wxBOTTOM, 4);

	// 'Remove' button
	btn_patch_remove = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_remove"));
	btn_patch_remove->SetToolTip("Remove selected patch(es) from texture");
	vbox->Add(btn_patch_remove, 0, wxBOTTOM, 4);

	// 'Back' button
	btn_patch_back = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_back"));
	btn_patch_back->SetToolTip("Send selected patch(es) back");
	vbox->Add(btn_patch_back, 0, wxBOTTOM, 4);

	// 'Forward' button
	btn_patch_forward = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_forward"));
	btn_patch_forward->SetToolTip("Bring selected patch(es) forward");
	vbox->Add(btn_patch_forward, 0, wxBOTTOM, 4);

	// 'Replace' button
	btn_patch_replace = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_replace"));
	btn_patch_replace->SetToolTip("Replace selected patch(es)");
	vbox->Add(btn_patch_replace, 0, wxBOTTOM, 4);

	// 'Duplicate' button
	btn_patch_duplicate = new wxBitmapButton(panel, -1, Icons::getIcon(Icons::GENERAL, "patch_duplicate"));
	btn_patch_duplicate->SetToolTip("Duplicate selected patch(es)");
	vbox->Add(btn_patch_duplicate, 0, wxBOTTOM, 4);



	// -- Patch Properties frame --
	frame = new wxStaticBox(panel, -1, "Patch Properties");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	// X Position
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	spin_patch_left = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, SHRT_MIN, SHRT_MAX);
	hbox->Add(new wxStaticText(panel, -1, "X Position:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(spin_patch_left, 1);

	// Y Position
	hbox = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	spin_patch_top = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxTE_PROCESS_ENTER, SHRT_MIN, SHRT_MAX);
	hbox->Add(new wxStaticText(panel, -1, "Y Position:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(spin_patch_top, 1);

	return panel;
}

/* TextureEditorPanel::populatePatchList
 * Populates the patch list with all patches present in the texture
 *******************************************************************/
void TextureEditorPanel::populatePatchList()
{
	// Clear current list
	list_patches->ClearAll();
	list_patches->Show(false);

	// Add columns
	list_patches->InsertColumn(0, "Name");

	// Check a texture is currently opened for editing
	if (!tex_current)
		return;

	// Add each patch to the list
	for (size_t a = 0; a < tex_current->nPatches(); a++)
		list_patches->addItem(a, tex_current->getPatch(a)->getName());

	// Update list width
	list_patches->Show(true);
	list_patches->GetParent()->Layout();
}

/* TextureEditorPanel::updatePatchControls
 * Updates all patch editing controls with values from the currently
 * selected patch. Behaves differently depending on the number of
 * patches selected
 *******************************************************************/
void TextureEditorPanel::updatePatchControls()
{
	// Get selected patches
	wxArrayInt selection = list_patches->selectedItems();

	// If nothing is selected, disable patch controls
	if (selection.size() == 0)
	{
		spin_patch_left->Enable(false);
		spin_patch_top->Enable(false);
	}
	else
	{
		// Something is selected, enable the controls
		spin_patch_left->Enable(true);
		spin_patch_top->Enable(true);

		// If only 1 patch is selected, just set the controls to this patch
		if (selection.size() == 1)
		{
			CTPatch* patch = tex_current->getPatch(selection[0]);
			if (!patch)
			{
				LOG_MESSAGE(1, "Error: Selected patch does not exist in texture");
				return;
			}

			spin_patch_left->SetValue(patch->xOffset());
			spin_patch_top->SetValue(patch->yOffset());
		}
		else
		{
			// Multiple selection, only enable some controls
			spin_patch_left->Enable(false);
			spin_patch_top->Enable(false);
		}
	}
}

/* TextureEditorPanel::openTexture
 * Loads a TEXTUREX format texture into the editor
 *******************************************************************/
bool TextureEditorPanel::openTexture(CTexture* tex, TextureXList* list)
{
	// Check texture was given
	if (!tex)
	{
		clearTexture();
		return false;
	}

	// Set as current texture
	if (!tex_current)
		tex_current = new CTexture();
	tex_current->copyTexture(tex);
	tex_current->setList(list);

	// Open texture in canvas
	tex_canvas->openTexture(tex_current, tx_editor->getArchive());

	// Set control values
	updateTextureControls();
	populatePatchList();
	updatePatchControls();

	tex_modified = false;

	return true;
}

/* TextureEditorPanel::clearTexture
 * Clears the current texture
 *******************************************************************/
void TextureEditorPanel::clearTexture()
{
	// Clear texture
	if (tex_current)
		delete tex_current;
	tex_canvas->clearTexture();

	// Update variables
	tex_current = NULL;
	tex_modified = false;

	// Set control values
	updateTextureControls();
	populatePatchList();
	updatePatchControls();
}

/* TextureEditorPanel::setPalette
 * Sets the texture canvas' palette and refreshes it
 *******************************************************************/
void TextureEditorPanel::setPalette(Palette8bit* pal)
{
	tex_canvas->setPalette(pal);
	tex_canvas->updatePatchTextures();
	tex_canvas->Refresh();
}

/* TextureEditorPanel::getPalette
 * Returns the texture canvas' palette
 *******************************************************************/
Palette8bit* TextureEditorPanel::getPalette()
{
	return tex_canvas->getPalette();
}

/* TextureEditorPanel::getPalette
 * Returns true if the texture uses RGBA blending
 *******************************************************************/
bool TextureEditorPanel::getBlendRGBA()
{
	return tex_canvas->getBlendRGBA();
}

/* TextureEditorPanel::addPatch
 * Prompts the user to select a patch from the patch table to be
 * added to the current texture
 *******************************************************************/
void TextureEditorPanel::addPatch()
{
	// Do nothing if patch list is empty
	if (tx_editor->patchTable().nPatches() == 0 || !tex_current)
		return;

	// Browse for patch
	int patch = tx_editor->browsePatchTable();
	if (patch >= 0)
	{
		// Add new patch
		tex_current->addPatch(tx_editor->patchTable().patchName(patch), 0, 0);

		// Update UI
		populatePatchList();
		updatePatchControls();
	}

	tex_modified = true;
}

/* TextureEditorPanel::removePatch
 * Removes selected patch(es) from the current texture
 *******************************************************************/
void TextureEditorPanel::removePatch()
{
	// Get selection
	wxArrayInt selection = list_patches->selectedItems();

	// Do nothing if no patches are selected
	if (selection.size() == 0 )
		return;

	// Remove each selected patch
	for (int a = selection.size()-1; a >= 0; a--)
	{
		int index = selection[a];
		// Remove patch from texture
		tex_current->removePatch(index);

		// Remove patch from list
		list_patches->DeleteItem(index);
	}

	// Update UI
	updatePatchControls();
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* TextureEditorPanel::patchBack
 * Moves selected patch(es) 'back' in the current texture
 *******************************************************************/
void TextureEditorPanel::patchBack()
{
	// Get selected patch(es)
	wxArrayInt selection = list_patches->selectedItems();

	// Do nothing if nothing is selected
	if (selection.size() == 0)
		return;

	// Do nothing if first patch is selected
	if (selection[0] == 0)
		return;

	// Go through selection
	for (size_t a = 0; a < selection.size(); a++)
	{
		// Swap in list
		list_patches->swapItems(selection[a], selection[a] - 1);

		// Swap in texture
		tex_canvas->swapPatches(selection[a], selection[a] - 1);
	}

	// Update UI
	updatePatchControls();
	tex_canvas->redraw(true);

	// Restore selection in texture canvas
	selection = list_patches->selectedItems();
	for (unsigned a = 0; a < selection.size(); a++)
		tex_canvas->selectPatch(selection[a]);

	tex_modified = true;
}

/* TextureEditorPanel::patchForward
 * Moves selected patch(es) 'forward' in the current texture
 *******************************************************************/
void TextureEditorPanel::patchForward()
{
	// Get selected patch(es)
	wxArrayInt selection = list_patches->selectedItems();

	// Do nothing if nothing is selected
	if (selection.size() == 0)
		return;

	// Do nothing if last patch is selected
	if (selection.back() == list_patches->GetItemCount()-1)
		return;

	// Go through selection from bottom up
	for (int a = selection.size() - 1; a >= 0; a--)
	{
		// Swap in list
		list_patches->swapItems(selection[a], selection[a] + 1);

		// Swap in texture
		tex_canvas->swapPatches(selection[a], selection[a] + 1);
	}

	// Update UI
	updatePatchControls();
	tex_canvas->redraw(true);

	// Restore selection in texture canvas
	selection = list_patches->selectedItems();
	for (unsigned a = 0; a < selection.size(); a++)
		tex_canvas->selectPatch(selection[a]);

	tex_modified = true;
}

/* TextureEditorPanel::replacePatch
 * Prompts the user to select a patch from the patch table to replace
 * selectes patch(es) with
 *******************************************************************/
void TextureEditorPanel::replacePatch()
{
	// Get selection
	wxArrayInt selection = list_patches->selectedItems();

	// Do nothing if no patches are selected
	if (selection.size() == 0)
		return;

	// Get first selected patch name (for browser)
	string pname = tex_canvas->getTexture()->getPatch(selection[0])->getName();

	// Browse for patch
	int patch = tx_editor->browsePatchTable(pname);
	if (patch >= 0)
	{
		// Go through selection and replace each patch
		for (size_t a = 0; a < selection.size(); a++)
			tex_current->replacePatch(selection[a], tx_editor->patchTable().patchName(patch));
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

/* TextureEditorPanel::duplicatePatch
 * Duplicates selected patch(es) in the current texture (each
 * duplication is placed 8 units right+down from its original patch)
 *******************************************************************/
void TextureEditorPanel::duplicatePatch(int xoff, int yoff)
{
	// Get selection
	wxArrayInt selection = list_patches->selectedItems();

	// Do nothing if no patches are selected
	if (selection.size() == 0)
		return;

	// Go through selection backwards
	for (int a = selection.size()-1; a >= 0; a--)
	{
		// Duplicate selected patch
		tex_current->duplicatePatch(selection[a], xoff, yoff);
	}

	// Repopulate patch list
	populatePatchList();

	// Update selection
	int offset = 1;
	for (size_t a = 0; a < selection.size(); a++)
	{
		list_patches->selectItem(selection[a] + offset);
		offset++;
	}

	// Update UI
	updatePatchControls();

	tex_modified = true;
}

/* TextureEditorPanel::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool TextureEditorPanel::handleAction(string id)
{
	// Don't handle actions if hidden
	if (!IsShown())
		return false;

	// Only interested in actions beginning with txed_
	if (!id.StartsWith("txed_"))
		return false;

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



/*******************************************************************
 * TEXTUREEDITORPANEL EVENTS
 *******************************************************************/

/* TextureEditorPanel::onZoomChanged
 * Called when the zoom slider is changed
 *******************************************************************/
void TextureEditorPanel::onZoomChanged(wxCommandEvent& e)
{
	// Get zoom value
	int zoom_percent = slider_zoom->GetValue();

	// Lock to 10% increments
	int remainder = zoom_percent % 10;
	zoom_percent -= remainder;

	// Update zoom label
	label_current_zoom->SetLabel(S_FMT("%d%%", zoom_percent));

	// Zoom gfx canvas and update
	tex_canvas->setScale((double)zoom_percent * 0.01);
	tex_canvas->redraw(false);
}

/* TextureEditorPanel::onDrawOutsideChanged
 * Called when the 'show outside' checkbox is changed
 *******************************************************************/
void TextureEditorPanel::onDrawOutsideChanged(wxCommandEvent& e)
{
	// Set texture canvas 'show outside' option depending on checkbox value
	tex_canvas->drawOutside(cb_draw_outside->GetValue());

	// Update UI
	tex_canvas->redraw(false);
}

/* TextureEditorPanel::onTexCanvasMouseEvent
 * Called on any mouse event within the texture canvas
 *******************************************************************/
void TextureEditorPanel::onTexCanvasMouseEvent(wxMouseEvent& e)
{
	// Get mouse position relative to texture
	point2_t pos = tex_canvas->screenToTexPosition(e.GetX(), e.GetY());

	// Get patch that the mouse is over (if any)
	int patch = tex_canvas->patchAt(pos.x, pos.y);

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
				list_patches->selectItem(patch);
		}
		else if (e.ControlDown())
		{
			// Control is down, remove from selection
			if (patch >= 0)
				list_patches->deSelectItem(patch);
		}
		else
		{
			// Clear selection only if patch clicked was not already selected
			if (!tex_canvas->patchSelected(patch))
				list_patches->clearSelection();

			// Select patch
			if (patch >= 0)
				list_patches->selectItem(patch);
		}
	}

	// LEFT MOUSE UP
	else if (e.LeftUp())
	{
		// Hide texture grid
		tex_canvas->showGrid(false);

		// If mouse up over an already-selected patch, and shift/ctrl aren't down,
		// select only that patch (this mimics 'normal' drag-and-drop/selection behaviour)
		if (!e.ShiftDown() && !e.ControlDown() && tex_canvas->patchSelected(patch) && !tex_canvas->isDragging())
		{
			list_patches->clearSelection();
			list_patches->selectItem(patch);
		}

		// Redraw texture canvas
		tex_canvas->redraw(false);
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
			if (list_patches->GetSelectedItemCount() > 0)
			{
				// Get drag amount according to texture
				point2_t tex_cur = tex_canvas->screenToTexPosition(e.GetX(), e.GetY());
				point2_t tex_prev = tex_canvas->screenToTexPosition(tex_canvas->getMousePrevPos().x, tex_canvas->getMousePrevPos().y);
				point2_t diff = tex_cur - tex_prev;

				// Move any selected patches
				wxArrayInt selected_patches = list_patches->selectedItems();
				for (size_t a = 0; a < selected_patches.size(); a++)
				{
					CTPatch* patch = tex_current->getPatch(selected_patches[a]);
					if (!patch) continue;
					int16_t cx = patch->xOffset();
					int16_t cy = patch->yOffset();
					patch->setOffsetX(cx + diff.x);
					patch->setOffsetY(cy + diff.y);
					tex_modified = true;
				}

				// Refresh texture canvas
				tex_canvas->showGrid(true);
				tex_canvas->redraw(false);
			}
			else if (tex_current && tex_current->isExtended() && tex_canvas->getViewType() > 0)
			{
				// Get drag amount according to texture
				point2_t tex_cur = tex_canvas->screenToTexPosition(e.GetX(), e.GetY());
				point2_t tex_prev = tex_canvas->screenToTexPosition(tex_canvas->getMousePrevPos().x, tex_canvas->getMousePrevPos().y);
				point2_t diff = tex_cur - tex_prev;

				// Modify offsets
				tex_current->setOffsetX(tex_current->getOffsetX() - diff.x);
				tex_current->setOffsetY(tex_current->getOffsetY() - diff.y);
				tex_modified = true;

				// Refresh texture canvas
				tex_canvas->redraw(false);
			}
		}
	}

	e.Skip();
}

/* TextureEditorPanel::onTexCanvasDragEnd
 * Called when a mouse drag action ends on the texture canvas
 *******************************************************************/
void TextureEditorPanel::onTexCanvasDragEnd(wxCommandEvent& e)
{
	// If patch dragging ended (left button)
	if (e.GetInt() == wxMOUSE_BTN_LEFT)
	{
		// Update patch controls
		updatePatchControls();
	}
}

/* TextureEditorPanel::onTexCanvasKeyDown
 * Called when a key is pressed within the texture canvas
 *******************************************************************/
void TextureEditorPanel::onTexCanvasKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	wxArrayString binds = KeyBind::getBinds(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Check for alt key
	if (e.GetKeyCode() == WXK_ALT)
		alt_press = true;

	// Go through matching binds
	int x_movement = 0;
	int y_movement = 0;
	bool handled = false;
	for (unsigned a = 0; a < binds.size(); a++)
	{
		string name = binds[a];

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
		if (e.GetModifiers() == wxMOD_ALT && alt_press)
		{
			duplicatePatch(0, 0);
			alt_press = false;
		}

		wxArrayInt selected_patches = list_patches->selectedItems();
		for (size_t a = 0; a < selected_patches.size(); a++)
		{
			CTPatch* patch = tex_current->getPatch(selected_patches[a]);
			if (!patch) continue;
			int16_t cx = patch->xOffset();
			int16_t cy = patch->yOffset();
			patch->setOffsetX(cx + x_movement);
			patch->setOffsetY(cy + y_movement);
			tex_modified = true;
		}

		tex_canvas->redraw(true);
		handled = true;
	}

	if (!e.AltDown())
		alt_press = false;

	if (!handled)
		e.Skip();
}

/* TextureEditorPanel::onTexNameChanged
 * Called when the texture name entry box is changed
 *******************************************************************/
void TextureEditorPanel::onTexNameChanged(wxCommandEvent& e)
{
	// Change texture name
	if (tex_current)
		tex_current->setName(text_tex_name->GetValue());

	tex_modified = true;
}

/* TextureEditorPanel::onTexWidthChanged
 * Called when the texture width spin control is changed
 *******************************************************************/
void TextureEditorPanel::onTexWidthChanged(wxCommandEvent& e)
{
	// Set texture's width
	if (tex_current)
		tex_current->setWidth(spin_tex_width->GetValue());

	// Update UI
	tex_canvas->redraw(true);
	updateTextureScaleLabel();

	tex_modified = true;
}

/* TextureEditorPanel::onTexHeightChanged
 * Called when the texture height spin control is changed
 *******************************************************************/
void TextureEditorPanel::onTexHeightChanged(wxCommandEvent& e)
{
	// Set texture's height
	if (tex_current)
		tex_current->setHeight(spin_tex_height->GetValue());

	// Update UI
	tex_canvas->redraw(true);
	updateTextureScaleLabel();

	tex_modified = true;
}

/* TextureEditorPanel::onTexScaleXChanged
 * Called when the texture x scale spin control is changed
 *******************************************************************/
void TextureEditorPanel::onTexScaleXChanged(wxCommandEvent& e)
{
	// Set texture's x scale
	if (tex_current)
		tex_current->setScaleX((double)spin_tex_scalex->GetValue() / 8.0);

	// Update UI
	updateTextureScaleLabel();

	tex_modified = true;
}

/* TextureEditorPanel::onTexScaleYChanged
 * Called when the texture y scale spin control is changed
 *******************************************************************/
void TextureEditorPanel::onTexScaleYChanged(wxCommandEvent& e)
{
	// Set texture's y scale
	if (tex_current)
		tex_current->setScaleY((double)spin_tex_scaley->GetValue() / 8.0);

	// Update UI
	updateTextureScaleLabel();

	tex_modified = true;
}

/* TextureEditorPanel::onTexWorldPanningChanged
 * Called when the texture world panning checkbox is toggled
 *******************************************************************/
void TextureEditorPanel::onTexWorldPanningChanged(wxCommandEvent& e)
{
	// Set texture world panning flag
	if (tex_current)
		tex_current->setWorldPanning(cb_tex_world_panning->IsChecked());

	tex_modified = true;
}

/* TextureEditorPanel::onPatchListSelect
 * Called when a patch is selected on the patch list
 *******************************************************************/
void TextureEditorPanel::onPatchListSelect(wxListEvent& e)
{
	// Select the patch on the texture canvas
	tex_canvas->selectPatch(e.GetIndex());

	// Update UI
	tex_canvas->redraw(false);
	updatePatchControls();
}

/* TextureEditorPanel::onPatchListDeSelect
 * Called when a patch is deselected on the patch list
 *******************************************************************/
void TextureEditorPanel::onPatchListDeSelect(wxListEvent& e)
{
	// Deselect the patch on the texture canvas
	tex_canvas->deSelectPatch(e.GetIndex());

	// Update UI
	tex_canvas->redraw(false);
	updatePatchControls();
}

/* TextureEditorPanel::onBtnPatchAdd
 * Called when the 'add patch' button is pressed
 *******************************************************************/
void TextureEditorPanel::onBtnPatchAdd(wxCommandEvent& e)
{
	addPatch();
}

/* TextureEditorPanel::onBtnPatchRemove
 * Called when the 'remove patch' button is pressed
 *******************************************************************/
void TextureEditorPanel::onBtnPatchRemove(wxCommandEvent& e)
{
	removePatch();
}

/* TextureEditorPanel::onBtnPatchBack
 * Called when the 'send patch back' button is pressed
 *******************************************************************/
void TextureEditorPanel::onBtnPatchBack(wxCommandEvent& e)
{
	patchBack();
}

/* TextureEditorPanel::onBtnPatchForward
 * Called when the 'bring patch forward' button is pressed
 *******************************************************************/
void TextureEditorPanel::onBtnPatchForward(wxCommandEvent& e)
{
	patchForward();
}

/* TextureEditorPanel::onBtnPatchReplace
 * Called when the 'replace patch' button is pressed
 *******************************************************************/
void TextureEditorPanel::onBtnPatchReplace(wxCommandEvent& e)
{
	replacePatch();
}

/* TextureEditorPanel::onBtnPatchDuplicate
 * Called when the 'duplicate patch' button is pressed
 *******************************************************************/
void TextureEditorPanel::onBtnPatchDuplicate(wxCommandEvent& e)
{
	duplicatePatch();
}

/* TextureEditorPanel::onPatchPositionXChanged
 * Called when the patch x position spin control is changed
 *******************************************************************/
void TextureEditorPanel::onPatchPositionXChanged(wxCommandEvent& e)
{
	// If anything other than 1 patch is selected, do nothing (shouldn't happen anyway)
	if (list_patches->GetSelectedItemCount() != 1)
		return;

	// Get selected patch
	CTPatch* patch = tex_current->getPatch(list_patches->selectedItems()[0]);
	if (!patch) return;

	// Set patch x offset
	patch->setOffsetX(spin_patch_left->GetValue());

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* TextureEditorPanel::onPatchPositionYChanged
 * Called when the patch y position spin control is changed
 *******************************************************************/
void TextureEditorPanel::onPatchPositionYChanged(wxCommandEvent& e)
{
	// If anything other than 1 patch is selected, do nothing (shouldn't happen anyway)
	if (list_patches->GetSelectedItemCount() != 1)
		return;

	// Get selected patch
	CTPatch* patch = tex_current->getPatch(list_patches->selectedItems()[0]);
	if (!patch) return;

	// Set patch y offset
	patch->setOffsetY(spin_patch_top->GetValue());

	// Update UI
	tex_canvas->redraw(true);

	tex_modified = true;
}

/* TextureEditorPanel::onApplyScaleChanged
 * Called when the 'Apply Scale' checkbox is changed
 *******************************************************************/
void TextureEditorPanel::onApplyScaleChanged(wxCommandEvent& e)
{
	tex_canvas->applyTexScale(cb_tex_scale->GetValue());
	tex_canvas->redraw();
}

/* TextureEditorPanel::onARCChanged
 * Called when the 'Aspect Ratio Correction' checkbox is changed
 *******************************************************************/
void TextureEditorPanel::onARCChanged(wxCommandEvent& e)
{
	tx_arc = cb_tex_arc->IsChecked();
	tex_canvas->redraw();
}
