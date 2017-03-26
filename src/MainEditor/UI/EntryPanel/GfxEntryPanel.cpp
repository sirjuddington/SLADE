
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    GfxEntryPanel.cpp
 * Description: GfxEntryPanel class. The UI for editing gfx entries.
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
#include "GfxEntryPanel.h"
#include "Dialogs/GfxConvDialog.h"
#include "Dialogs/GfxCropDialog.h"
#include "Dialogs/ModifyOffsetsDialog.h"
#include "Dialogs/TranslationEditorDialog.h"
#include "General/Console/ConsoleHelpers.h"
#include "General/Misc.h"
#include "Graphics/Icons.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/Palette/PaletteManager.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/MainWindow.h"
#include "UI/SToolBar/SToolBar.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, gfx_arc)
EXTERN_CVAR(String, last_colour)
EXTERN_CVAR(String, last_tint_colour)
EXTERN_CVAR(Int, last_tint_amount)

string brushlist[Brush::NUM_BRUSHES] =
{
	"brush_sq_1",
	"brush_sq_3",
	"brush_sq_5",
	"brush_sq_7",
	"brush_sq_9",
	"brush_ci_5",
	"brush_ci_7",
	"brush_ci_9",
	"brush_di_3",
	"brush_di_5",
	"brush_di_7",
	"brush_di_9",
	"brush_pa_a",
	"brush_pa_b",
	"brush_pa_c",
	"brush_pa_d",
	"brush_pa_e",
	"brush_pa_f",
	"brush_pa_g",
	"brush_pa_h",
	"brush_pa_i",
	"brush_pa_j",
	"brush_pa_k",
	"brush_pa_l",
	"brush_pa_m",
	"brush_pa_n",
	"brush_pa_o",
};

/*******************************************************************
 * GFXENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* GfxEntryPanel::GfxEntryPanel
 * GfxEntryPanel class constructor
 *******************************************************************/
GfxEntryPanel::GfxEntryPanel(wxWindow* parent)
	: EntryPanel(parent, "gfx")
{
	// Init variables
	prev_translation.addRange(TRANS_PALETTE, 0);
	edit_translation.addRange(TRANS_PALETTE, 0);
	offset_changing = false;

	// Add gfx canvas
	gfx_canvas = new GfxCanvas(this, -1);
	sizer_main->Add(gfx_canvas->toPanel(this), 1, wxEXPAND, 0);
	gfx_canvas->setViewType(GFXVIEW_DEFAULT);
	gfx_canvas->allowDrag(true);
	gfx_canvas->allowScroll(true);
	gfx_canvas->setPalette(thePaletteChooser->getSelectedPalette());
	gfx_canvas->setTranslation(&edit_translation);

	// Offsets
	spin_xoffset = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, SHRT_MIN, SHRT_MAX, 0);
	spin_yoffset = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, SHRT_MIN, SHRT_MAX, 0);
	spin_xoffset->SetMinSize(wxSize(64, -1));
	spin_yoffset->SetMinSize(wxSize(64, -1));
	sizer_bottom->Add(new wxStaticText(this, -1, "Offsets:"), 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_bottom->Add(spin_xoffset, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 4);
	sizer_bottom->Add(spin_yoffset, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);

	// Gfx (offset) type
	string offset_types[] ={ "Auto", "Graphic", "Sprite", "HUD" };
	choice_offset_type = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 4, offset_types);
	choice_offset_type->SetSelection(0);
	sizer_bottom->Add(choice_offset_type, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);

	// Auto offset
	btn_auto_offset = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "offset"));
	btn_auto_offset->SetToolTip("Modify Offsets...");
	sizer_bottom->Add(btn_auto_offset, 0, wxALIGN_CENTER_VERTICAL);

	sizer_bottom->AddStretchSpacer();

	// Aspect ratio correction checkbox
	cb_arc = new wxCheckBox(this, -1, "Aspect Ratio Correction");
	cb_arc->SetValue(gfx_arc);
	sizer_bottom->Add(cb_arc, 0, wxEXPAND, 0);
	sizer_bottom->AddSpacer(8);

	// Tile checkbox
	cb_tile = new wxCheckBox(this, -1, "Tile");
	sizer_bottom->Add(cb_tile, 0, wxEXPAND, 0);
	sizer_bottom->AddSpacer(8);

	// Image selection buttons
	btn_nextimg = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "right"));
	btn_previmg = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "left"));
	text_curimg = new wxStaticText(this, -1, "Image XX/XX");
	btn_nextimg->Show(false);
	btn_previmg->Show(false);
	text_curimg->Show(false);

	// Palette chooser
	listenTo(theMainWindow->getPaletteChooser());

	// Custom menu
	menu_custom = new wxMenu();
	fillCustomMenu(menu_custom);
	custom_menu_name = "Graphic";

	// Brushes menu
	menu_brushes = new wxMenu();
	fillBrushMenu(menu_brushes);

	// Custom toolbar
	setupToolbar();

	// Bind Events
	slider_zoom->Bind(wxEVT_SLIDER, &GfxEntryPanel::onZoomChanged, this);
	cb_colour->Bind(wxEVT_COLOURBOX_CHANGED, &GfxEntryPanel::onPaintColourChanged, this);
	spin_xoffset->Bind(wxEVT_SPINCTRL, &GfxEntryPanel::onXOffsetChanged, this);
	spin_yoffset->Bind(wxEVT_SPINCTRL, &GfxEntryPanel::onYOffsetChanged, this);
	spin_xoffset->Bind(wxEVT_TEXT_ENTER, &GfxEntryPanel::onXOffsetChanged, this);
	spin_yoffset->Bind(wxEVT_TEXT_ENTER, &GfxEntryPanel::onYOffsetChanged, this);
	choice_offset_type->Bind(wxEVT_CHOICE, &GfxEntryPanel::onOffsetTypeChanged, this);
	cb_tile->Bind(wxEVT_CHECKBOX, &GfxEntryPanel::onTileChanged, this);
	cb_arc->Bind(wxEVT_CHECKBOX, &GfxEntryPanel::onARCChanged, this);
	Bind(wxEVT_GFXCANVAS_OFFSET_CHANGED, &GfxEntryPanel::onGfxOffsetChanged, this, gfx_canvas->GetId());
	Bind(wxEVT_GFXCANVAS_PIXELS_CHANGED, &GfxEntryPanel::onGfxPixelsChanged, this, gfx_canvas->GetId());
	Bind(wxEVT_GFXCANVAS_COLOUR_PICKED, &GfxEntryPanel::onColourPicked, this, gfx_canvas->GetId());
	btn_nextimg->Bind(wxEVT_BUTTON, &GfxEntryPanel::onBtnNextImg, this);
	btn_previmg->Bind(wxEVT_BUTTON, &GfxEntryPanel::onBtnPrevImg, this);
	btn_auto_offset->Bind(wxEVT_BUTTON, &GfxEntryPanel::onBtnAutoOffset, this);

	// Apply layout
	Layout();
}

/* GfxEntryPanel::~GfxEntryPanel
 * GfxEntryPanel class destructor
 *******************************************************************/
GfxEntryPanel::~GfxEntryPanel()
{
}

/* GfxEntryPanel::loadEntry
 * Loads an entry into the entry panel if it is a valid image format
 *******************************************************************/
bool GfxEntryPanel::loadEntry(ArchiveEntry* entry)
{
	return loadEntry(entry, 0);
}
bool GfxEntryPanel::loadEntry(ArchiveEntry* entry, int index)
{
	// Check entry was given
	if (entry == NULL)
	{
		Global::error = "no entry to load";
		return false;
	}

	// Update variables
	this->entry = entry;
	setModified(false);

	// Attempt to load the image
	if (!Misc::loadImageFromEntry(getImage(), this->entry, index))
		return false;

	// Only show next/prev image buttons if the entry contains multiple images
	if (getImage()->getSize() > 1)
	{
		btn_nextimg->Show();
		btn_previmg->Show();
		text_curimg->Show();
		sizer_bottom->Add(btn_previmg, 0, wxEXPAND|wxRIGHT, 4);
		sizer_bottom->Add(btn_nextimg, 0, wxEXPAND|wxRIGHT, 4);
		sizer_bottom->Add(text_curimg, 0, wxALIGN_CENTER, 0);
	}
	else
	{
		btn_nextimg->Show(false);
		btn_previmg->Show(false);
		text_curimg->Show(false);
		sizer_bottom->Detach(btn_nextimg);
		sizer_bottom->Detach(btn_previmg);
		sizer_bottom->Detach(text_curimg);
	}

	// Hack for colormaps to be 256-wide
	if (S_CMPNOCASE(entry->getType()->getName(), "colormap"))
	{
		getImage()->setWidth(256);
	}

	// Refresh everything
	refresh();

	return true;
}

/* GfxEntryPanel::saveEntry
 * Saves any changes to the entry
 *******************************************************************/
bool GfxEntryPanel::saveEntry()
{
	// Set offsets
	getImage()->setXOffset(spin_xoffset->GetValue());
	getImage()->setYOffset(spin_yoffset->GetValue());

	// Write new image data if modified
	bool ok = true;
	if (image_data_modified)
	{
		SImage* image = getImage();
		SIFormat* format = image->getFormat();

		string error = "";
		ok = false;
		int writable = format->canWrite(*image);
		if (format == SIFormat::unknownFormat())
			error = "Image is of unknown format";
		else if (writable == SIFormat::NOTWRITABLE)
			error = S_FMT("Writing unsupported for format \"%s\"", format->getName());
		else
		{
			// Convert image if necessary (using default options)
			if (writable == SIFormat::CONVERTIBLE)
			{
				format->convertWritable(*image, SIFormat::convert_options_t());
				wxLogMessage("Image converted for writing");
			}

			if (format->saveImage(*image, entry->getMCData(), gfx_canvas->getPalette()))
				ok = true;
			else
				error = "Error writing image";
		}

		if (ok)
		{
			// Set modified
			entry->setState(1);

			// Re-detect type
			EntryType* oldtype = entry->getType();
			EntryType::detectEntryType(entry);

			// Update extension if type changed
			if (oldtype != entry->getType())
				entry->setExtensionByType();
		}
		else
			wxMessageBox(wxString("Cannot save changes to image: ") + error, "Error", wxICON_ERROR);
	}
	// Otherwise just set offsets
	else
		EntryOperations::setGfxOffsets(entry, spin_xoffset->GetValue(), spin_yoffset->GetValue());

	// Apply alPh/tRNS options
	if (entry->getType()->getFormat() == "img_png")
	{
		bool alph = EntryOperations::getalPhChunk(entry);
		bool trns = EntryOperations::gettRNSChunk(entry);

		if (alph != menu_custom->IsChecked(theApp->getAction("pgfx_alph")->getWxId()))
			EntryOperations::modifyalPhChunk(entry, !alph);
		if (trns != menu_custom->IsChecked(theApp->getAction("pgfx_trns")->getWxId()))
			EntryOperations::modifytRNSChunk(entry, !trns);
	}

	if (ok)
		setModified(false);

	return ok;
}

void GfxEntryPanel::setupToolbar()
{
	// Zoom
	SToolBarGroup* g_zoom = new SToolBarGroup(toolbar, "Zoom", true);
	slider_zoom = new wxSlider(g_zoom, -1, 100, 20, 800, wxDefaultPosition, wxSize(200, -1));
	slider_zoom->SetLineSize(10);
	slider_zoom->SetPageSize(100);
	label_current_zoom = new wxStaticText(g_zoom, -1, "100%");
	g_zoom->addCustomControl(slider_zoom);
	g_zoom->addCustomControl(label_current_zoom);
	toolbar->addGroup(g_zoom);

	// Editing operations
	SToolBarGroup* g_edit = new SToolBarGroup(toolbar, "Editing");
	g_edit->addActionButton("pgfx_settrans", "");
	cb_colour = new ColourBox(g_edit, -1, COL_BLACK, false, true);
	cb_colour->setPalette(gfx_canvas->getPalette());
	button_brush = g_edit->addActionButton("pgfx_setbrush", "");
	g_edit->addCustomControl(cb_colour);
	g_edit->addActionButton("pgfx_drag", "");
	g_edit->addActionButton("pgfx_draw", "");
	g_edit->addActionButton("pgfx_erase", "");
	g_edit->addActionButton("pgfx_magic", "");
	theApp->toggleAction("pgfx_drag"); // Drag offsets by default
	toolbar->addGroup(g_edit);

	// Image operations
	SToolBarGroup* g_image = new SToolBarGroup(toolbar, "Image");
	g_image->addActionButton("pgfx_mirror", "");
	g_image->addActionButton("pgfx_flip", "");
	g_image->addActionButton("pgfx_rotate", "");
	g_image->addActionButton("pgfx_crop", "");
	g_image->addActionButton("pgfx_convert", "");
	toolbar->addGroup(g_image);

	// Colour operations
	SToolBarGroup* g_colour = new SToolBarGroup(toolbar, "Colour");
	g_colour->addActionButton("pgfx_remap", "");
	g_colour->addActionButton("pgfx_colourise", "");
	g_colour->addActionButton("pgfx_tint", "");
	toolbar->addGroup(g_colour);

	// Misc operations
	SToolBarGroup* g_png = new SToolBarGroup(toolbar, "PNG");
	g_png->addActionButton("pgfx_pngopt", "");
	toolbar->addGroup(g_png);
	toolbar->enableGroup("PNG", false);
}

void GfxEntryPanel::fillBrushMenu(wxMenu* bm)
{
	theApp->getAction("pgfx_brush_sq_1")->addToMenu(bm);
	theApp->getAction("pgfx_brush_sq_3")->addToMenu(bm);
	theApp->getAction("pgfx_brush_sq_5")->addToMenu(bm);
	theApp->getAction("pgfx_brush_sq_7")->addToMenu(bm);
	theApp->getAction("pgfx_brush_sq_9")->addToMenu(bm);
	theApp->getAction("pgfx_brush_ci_5")->addToMenu(bm);
	theApp->getAction("pgfx_brush_ci_7")->addToMenu(bm);
	theApp->getAction("pgfx_brush_ci_9")->addToMenu(bm);
	theApp->getAction("pgfx_brush_di_3")->addToMenu(bm);
	theApp->getAction("pgfx_brush_di_5")->addToMenu(bm);
	theApp->getAction("pgfx_brush_di_7")->addToMenu(bm);
	theApp->getAction("pgfx_brush_di_9")->addToMenu(bm);
	wxMenu* pa = new wxMenu;
	theApp->getAction("pgfx_brush_pa_a")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_b")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_c")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_d")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_e")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_f")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_g")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_h")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_i")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_j")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_k")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_l")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_m")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_n")->addToMenu(pa);
	theApp->getAction("pgfx_brush_pa_o")->addToMenu(pa);
	bm->AppendSubMenu(pa, "Dither Patterns");
}

bool GfxEntryPanel::iconChanger(wxCommandEvent& e)
{
	return true;
}

/* GfxEntryPanel::extractAll
 * Extract all sub-images as individual PNGs
 *******************************************************************/
bool GfxEntryPanel::extractAll()
{
	if (getImage()->getSize() < 2)
		return false;

	// Remember where we are
	int imgindex = getImage()->getIndex();

	Archive* parent = entry->getParent();
	if (parent == NULL) return false;

	int index = parent->entryIndex(entry, entry->getParentDir());
	string name = wxFileName(entry->getName()).GetName();

	// Loop through subimages and get things done
	int pos = 0;
	for (int i = 0; i < getImage()->getSize(); ++i)
	{
		string newname = S_FMT("%s_%i.png", name, i);
		Misc::loadImageFromEntry(getImage(), entry, i);

		// Only process images that actually contain some pixels
		if (getImage()->getWidth() && getImage()->getHeight())
		{
			ArchiveEntry* newimg = parent->addNewEntry(newname, index+pos+1, entry->getParentDir());
			if (newimg == NULL) return false;
			SIFormat::getFormat("png")->saveImage(*getImage(), newimg->getMCData(), gfx_canvas->getPalette());
			EntryType::detectEntryType(newimg);
			pos++;
		}
	}

	// Reload image of where we were
	Misc::loadImageFromEntry(getImage(), entry, imgindex);

	return true;
}

/* GfxEntryPanel::refresh
 * Reloads image data and force refresh
 *******************************************************************/
void GfxEntryPanel::refresh()
{
	// Setup palette
	theMainWindow->getPaletteChooser()->setGlobalFromArchive(entry->getParent(), Misc::detectPaletteHack(entry));
	updateImagePalette();

	// Set offset text boxes
	spin_xoffset->SetValue(getImage()->offset().x);
	spin_yoffset->SetValue(getImage()->offset().y);

	// Get some needed menu ids
	int MENU_GFXEP_PNGOPT = theApp->getAction("pgfx_pngopt")->getWxId();
	int MENU_GFXEP_ALPH = theApp->getAction("pgfx_alph")->getWxId();
	int MENU_GFXEP_TRNS = theApp->getAction("pgfx_trns")->getWxId();
	int MENU_GFXEP_EXTRACT = theApp->getAction("pgfx_extract")->getWxId();
	int MENU_GFXEP_TRANSLATE = theApp->getAction("pgfx_remap")->getWxId();
	int MENU_ARCHGFX_EXPORTPNG = theApp->getAction("arch_gfx_exportpng")->getWxId();

	// Set PNG check menus
	if (this->entry->getType() != NULL && this->entry->getType()->getFormat() == "img_png")
	{
		// Check for alph
		alph = EntryOperations::getalPhChunk(this->entry);
		menu_custom->Enable(MENU_GFXEP_ALPH, true);
		menu_custom->Check(MENU_GFXEP_ALPH, alph);

		// Check for trns
		trns = EntryOperations::gettRNSChunk(this->entry);
		menu_custom->Enable(MENU_GFXEP_TRNS, true);
		menu_custom->Check(MENU_GFXEP_TRNS, trns);

		// Disable 'Export as PNG' (it already is :P)
		menu_custom->Enable(MENU_ARCHGFX_EXPORTPNG, false);

		// Add 'Optimize PNG' option
		menu_custom->Enable(MENU_GFXEP_PNGOPT, true);
		toolbar->enableGroup("PNG", true);
	}
	else
	{
		menu_custom->Enable(MENU_GFXEP_ALPH, false);
		menu_custom->Enable(MENU_GFXEP_TRNS, false);
		menu_custom->Check(MENU_GFXEP_ALPH, false);
		menu_custom->Check(MENU_GFXEP_TRNS, false);
		menu_custom->Enable(MENU_GFXEP_PNGOPT, false);
		menu_custom->Enable(MENU_ARCHGFX_EXPORTPNG, true);
		toolbar->enableGroup("PNG", false);
	}

	// Set multi-image format stuff thingies
	cur_index = getImage()->getIndex();
	if (getImage()->getSize() > 1)
		menu_custom->Enable(MENU_GFXEP_EXTRACT, true);
	else menu_custom->Enable(MENU_GFXEP_EXTRACT, false);
	text_curimg->SetLabel(S_FMT("Image %d/%d", cur_index+1, getImage()->getSize()));

	// Update status bar in case image dimensions changed
	updateStatus();

	// Apply offset view type
	applyViewType();

	// Reset display offsets in graphics mode
	if (gfx_canvas->getViewType() != GFXVIEW_SPRITE)
		gfx_canvas->resetOffsets();

	// Setup custom menu
	if (getImage()->getType() == RGBA)
		menu_custom->Enable(MENU_GFXEP_TRANSLATE, false);
	else
		menu_custom->Enable(MENU_GFXEP_TRANSLATE, true);

	// Refresh the canvas
	gfx_canvas->Refresh();
}

/* GfxEntryPanel::statusString
 * Returns a string with extended editing/entry info for the status
 * bar
 *******************************************************************/
string GfxEntryPanel::statusString()
{
	// Setup status string
	SImage* image = getImage();
	string status = S_FMT("%dx%d", image->getWidth(), image->getHeight());

	// Colour format
	if (image->getType() == RGBA)
		status += ", 32bpp";
	else
		status += ", 8bpp";

	// PNG stuff
	if (entry->getType()->getFormat() == "img_png")
	{
		// alPh
		if (EntryOperations::getalPhChunk(entry))
			status += ", alPh";

		// tRNS
		if (EntryOperations::gettRNSChunk(entry))
			status += ", tRNS";
	}

	return status;
}

/* GfxEntryPanel::refreshPanel
 * Redraws the panel
 *******************************************************************/
void GfxEntryPanel::refreshPanel()
{
	//if (entry && getImage() && !image_data_modified)
	//	loadEntry(entry, getImage()->getIndex());
	Update();
	Refresh();
}

/* GfxEntryPanel::updateImagePalette
 * Sets the gfx canvas' palette to what is selected in the palette
 * chooser, and refreshes the gfx canvas
 *******************************************************************/
void GfxEntryPanel::updateImagePalette()
{
	gfx_canvas->setPalette(theMainWindow->getPaletteChooser()->getSelectedPalette());
	gfx_canvas->updateImageTexture();
}

/* GfxEntryPanel::detectOffsetType
 * Detects the offset view type of the current entry
 *******************************************************************/
int GfxEntryPanel::detectOffsetType()
{
	if (!entry)
		return GFXVIEW_DEFAULT;

	if (!entry->getParent())
		return GFXVIEW_DEFAULT;

	// Check what section of the archive the entry is in -- only PNGs or images
	// in the sprites section can be HUD or sprite
	bool is_sprite = ("sprites" == entry->getParent()->detectNamespace(entry));
	bool is_png = ("img_png" == entry->getType()->getFormat());
	if (!is_sprite && !is_png)
		return GFXVIEW_DEFAULT;

	SImage *img = getImage();
	if (is_png && img->offset().x == 0 && img->offset().y == 0)
		return GFXVIEW_DEFAULT;

	int width = img->getWidth();
	int height = img->getHeight();
	int left = -img->offset().x;
	int right = left + width;
	int top = -img->offset().y;
	int bottom = top + height;
	int horiz_center = (left + right) / 2;

	// Determine sprite vs. HUD with a rough heuristic: give each one a
	// penalty, measuring how far (in pixels) the offsets are from the "ideal"
	// offsets for that type.  Lowest penalty wins.
	int sprite_penalty = 0;
	int hud_penalty = 0;
	// The HUD is drawn with the origin in the top left, so HUD offsets
	// generally put the center of the screen (160, 100) above or inside the
	// top center of the sprite.
	hud_penalty += abs(horiz_center - 160);
	hud_penalty += abs(top - 100);
	// It's extremely unusual for the bottom of the sprite to be above 168,
	// which is where the weapon is cut off in fullscreen.  Extra penalty.
	if (bottom < 168)
		hud_penalty += (168 - bottom);
	// Sprites are drawn relative to the center of an object at floor height,
	// so the offsets generally put the origin (0, 0) near the vertical center
	// line and the bottom edge.  Some sprites are vertically centered whereas
	// some use a small bottom margin for feet, so split the difference and use
	// 1/4 up from the bottom.
	int bottom_quartile = (bottom * 3 + top) / 4;
	sprite_penalty += abs(bottom_quartile - 0);
	sprite_penalty += abs(horiz_center - 0);
	// It's extremely unusual for the sprite to not contain the origin, which
	// would draw it not touching its actual position.  Extra panalty for that,
	// though allow for a sprite that floats up to its own height above the
	// floor.
	if (top > 0)
		sprite_penalty += (top - 0);
	else if (bottom < 0 - height)
		sprite_penalty += (0 - height - bottom);

	// Sprites are more common than HUD, so in case of a tie, sprite wins
	if (sprite_penalty > hud_penalty)
		return GFXVIEW_HUD;
	else
		return GFXVIEW_SPRITE;
}

/* GfxEntryPanel::applyViewType
 * Sets the view type of the gfx canvas depending on what is selected
 * in the offset type combo box
 *******************************************************************/
void GfxEntryPanel::applyViewType()
{
	// Tile checkbox overrides offset type selection
	if (cb_tile->IsChecked())
		gfx_canvas->setViewType(GFXVIEW_TILED);
	else
	{
		// Set gfx canvas view type depending on the offset combobox selection
		int sel = choice_offset_type->GetSelection();
		switch (sel)
		{
		case 0:
			gfx_canvas->setViewType(detectOffsetType());
			break;
		case 1:
			gfx_canvas->setViewType(GFXVIEW_DEFAULT);
			break;
		case 2:
			gfx_canvas->setViewType(GFXVIEW_SPRITE);
			break;
		case 3:
			gfx_canvas->setViewType(GFXVIEW_HUD);
			break;
		}
	}

	// Refresh
	gfx_canvas->Refresh();
}

/* GfxEntryPanel::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool GfxEntryPanel::handleAction(string id)
{
	// Don't handle actions if hidden
	if (!isActivePanel())
		return false;

	// We're only interested in "pgfx_" actions
	if (!id.StartsWith("pgfx_"))
		return false;

	// For pgfx_brush actions, the string after pgfx is a brush name
	if (id.StartsWith("pgfx_brush"))
	{
		string br = id.AfterFirst('_');

		uint8_t i;
		for (i = 0; i < Brush::NUM_BRUSHES; ++i)
			if (S_CMP(br, brushlist[i]))
				break;

		if (i == Brush::NUM_BRUSHES)
			return false;

		gfx_canvas->setBrush(i);
		button_brush->setIcon(br);
	}

	// Editing - drag mode
	else if (id == "pgfx_drag")
	{
		editing = false;
		gfx_canvas->setEditingMode(0);
	}

	// Editing - draw mode
	else if (id == "pgfx_draw")
	{
		editing = true;
		gfx_canvas->setEditingMode(1);
		gfx_canvas->setPaintColour(cb_colour->getColour());
	}

	// Editing - erase mode
	else if (id == "pgfx_erase")
	{
		editing = true;
		gfx_canvas->setEditingMode(2);
	}

	// Editing - translate mode
	else if (id == "pgfx_magic")
	{
		editing = true;
		gfx_canvas->setEditingMode(3);
	}

	// Editing - set translation
	else if (id == "pgfx_settrans")
	{
		// Create translation editor dialog
		Palette8bit* pal = theMainWindow->getPaletteChooser()->getSelectedPalette();
		TranslationEditorDialog ted(theMainWindow, pal, " Colour Remap", getImage());

		// Create translation to edit
		ted.openTranslation(edit_translation);

		// Show the dialog
		if (ted.ShowModal() == wxID_OK)
		{
			// Set the translation
			edit_translation.copy(ted.getTranslation());
			gfx_canvas->setTranslation(&edit_translation);
		}

	}

	// Editing - set brush
	else if (id == "pgfx_setbrush")
	{
		wxPoint p = button_brush->GetScreenPosition() -= GetScreenPosition();
		p.y += button_brush->GetMaxHeight();
		PopupMenu(menu_brushes, p);
	}

	// Mirror
	else if (id == "pgfx_mirror")
	{
		// Mirror X
		getImage()->mirror(false);

		// Update UI
		gfx_canvas->updateImageTexture();
		gfx_canvas->Refresh();

		// Update variables
		image_data_modified = true;
		setModified();
	}

	// Flip
	else if (id == "pgfx_flip")
	{
		// Mirror Y
		getImage()->mirror(true);

		// Update UI
		gfx_canvas->updateImageTexture();
		gfx_canvas->Refresh();

		// Update variables
		image_data_modified = true;
		setModified();
	}

	// Rotate
	else if (id == "pgfx_rotate")
	{
		// Prompt for rotation angle
		string angles[] = { "90", "180", "270" };
		int choice = wxGetSingleChoiceIndex("Select rotation angle", "Rotate", 3, angles, 0);

		// Rotate image
		switch (choice)
		{
		case 0:
			getImage()->rotate(90);
			break;
		case 1:
			getImage()->rotate(180);
			break;
		case 2:
			getImage()->rotate(270);
			break;
		default: break;
		}

		// Update UI
		gfx_canvas->updateImageTexture();
		gfx_canvas->Refresh();

		// Update variables
		image_data_modified = true;
		setModified();
	}

	// Translate
	else if (id == "pgfx_remap")
	{
		// Create translation editor dialog
		Palette8bit* pal = theMainWindow->getPaletteChooser()->getSelectedPalette();
		TranslationEditorDialog ted(theMainWindow, pal, " Colour Remap", getImage());

		// Create translation to edit
		ted.openTranslation(prev_translation);

		// Show the dialog
		if (ted.ShowModal() == wxID_OK)
		{
			// Apply translation to image
			getImage()->applyTranslation(&ted.getTranslation(), pal);

			// Update UI
			gfx_canvas->updateImageTexture();
			gfx_canvas->Refresh();

			// Update variables
			image_data_modified = true;
			gfx_canvas->updateImageTexture();
			setModified();
			prev_translation.copy(ted.getTranslation());
		}
	}

	// Colourise
	else if (id == "pgfx_colourise")
	{
		Palette8bit* pal = theMainWindow->getPaletteChooser()->getSelectedPalette();
		GfxColouriseDialog gcd(theMainWindow, entry, pal);
		gcd.setColour(last_colour);

		// Show colourise dialog
		if (gcd.ShowModal() == wxID_OK)
		{
			// Colourise image
			getImage()->colourise(gcd.getColour(), pal);

			// Update UI
			gfx_canvas->updateImageTexture();
			gfx_canvas->Refresh();

			// Update variables
			image_data_modified = true;
			Refresh();
			setModified();
		}
		rgba_t gcdcol = gcd.getColour();
		last_colour = S_FMT("RGB(%d, %d, %d)", gcdcol.r, gcdcol.g, gcdcol.b);
	}

	// Tint
	else if (id == "pgfx_tint")
	{
		Palette8bit* pal = theMainWindow->getPaletteChooser()->getSelectedPalette();
		GfxTintDialog gtd(theMainWindow, entry, pal);
		gtd.setValues(last_tint_colour, last_tint_amount);

		// Show tint dialog
		if (gtd.ShowModal() == wxID_OK)
		{
			// Tint image
			getImage()->tint(gtd.getColour(), gtd.getAmount(), pal);

			// Update UI
			gfx_canvas->updateImageTexture();
			gfx_canvas->Refresh();

			// Update variables
			image_data_modified = true;
			Refresh();
			setModified();
		}
		rgba_t gtdcol = gtd.getColour();
		last_tint_colour = S_FMT("RGB(%d, %d, %d)", gtdcol.r, gtdcol.g, gtdcol.b);
		last_tint_amount = (int)(gtd.getAmount() * 100.0);
	}

	// Crop
	else if (id == "pgfx_crop")
	{
		auto image = getImage();
		auto pal = theMainWindow->getPaletteChooser()->getSelectedPalette();
		GfxCropDialog gcd(theMainWindow, image, pal);

		// Show crop dialog
		if (gcd.ShowModal() == wxID_OK)
		{
			// Prompt to adjust offsets
			auto crop = gcd.getCropRect();
			if (crop.tl.x > 0 || crop.tl.y > 0)
			{
				if (wxMessageBox(
					"Do you want to adjust the offsets? This will keep the graphic in the same relative "
					"position it was before cropping.",
					"Adjust Offsets?",
					wxYES_NO) == wxYES)
				{
					image->setXOffset(image->offset().x - crop.tl.x);
					image->setYOffset(image->offset().y - crop.tl.y);
				}
			}

			// Crop image
			image->crop(crop.x1(), crop.y1(), crop.x2(), crop.y2());

			// Update UI
			gfx_canvas->updateImageTexture();
			gfx_canvas->Refresh();

			// Update variables
			image_data_modified = true;
			Refresh();
			setModified();
		}
	}

	// alPh/tRNS
	else if (id == "pgfx_alph" || id == "pgfx_trns")
	{
		setModified();
		Refresh();
	}

	// Optimize PNG
	else if (id == "pgfx_pngopt")
	{
		// This is a special case. If we set the entry as modified, SLADE will prompt
		// to save it, rewriting the entry and cancelling the optimization done...
		if (EntryOperations::optimizePNG(entry))
			setModified(false);
		else
			wxMessageBox("Warning: Couldn't optimize this image, check console log for info", "Warning", wxOK|wxCENTRE|wxICON_WARNING);
		Refresh();
	}

	// Extract all
	else if (id == "pgfx_extract")
	{
		extractAll();
	}

	// Convert
	else if (id == "pgfx_convert")
	{
		GfxConvDialog gcd(theMainWindow);
		gcd.CenterOnParent();
		gcd.openEntry(entry);

		gcd.ShowModal();

		if (gcd.itemModified(0))
		{
			// Get image and conversion info
			SImage* image = gcd.getItemImage(0);
			SIFormat* format = gcd.getItemFormat(0);

			// Write converted image back to entry
			format->saveImage(*image, entry_data, gcd.getItemPalette(0));
			// This makes the "save" button (and the setModified stuff) redundant and confusing!
			// The alternative is to save to entry effectively (uncomment the importMemChunk line)
			// but remove the setModified and image_data_modified lines, and add a call to refresh
			// to get the PNG tRNS status back in sync.
			//entry->importMemChunk(entry_data);
			image_data_modified = true;
			setModified();

			// Fix tRNS status if we converted to paletted PNG
			int MENU_GFXEP_PNGOPT = theApp->getAction("pgfx_pngopt")->getWxId();
			int MENU_GFXEP_ALPH = theApp->getAction("pgfx_alph")->getWxId();
			int MENU_GFXEP_TRNS = theApp->getAction("pgfx_trns")->getWxId();
			int MENU_ARCHGFX_EXPORTPNG = theApp->getAction("arch_gfx_exportpng")->getWxId();
			if (format->getName() == "PNG")
			{
				ArchiveEntry temp;
				temp.importMemChunk(entry_data);
				temp.setType(EntryType::getType("png"));
				menu_custom->Enable(MENU_GFXEP_ALPH, true);
				menu_custom->Enable(MENU_GFXEP_TRNS, true);
				menu_custom->Check(MENU_GFXEP_TRNS, EntryOperations::gettRNSChunk(&temp));
				menu_custom->Enable(MENU_ARCHGFX_EXPORTPNG, false);
				menu_custom->Enable(MENU_GFXEP_PNGOPT, true);
				toolbar->enableGroup("PNG", true);
			}
			else
			{
				menu_custom->Enable(MENU_GFXEP_ALPH, false);
				menu_custom->Enable(MENU_GFXEP_TRNS, false);
				menu_custom->Enable(MENU_ARCHGFX_EXPORTPNG, true);
				menu_custom->Enable(MENU_GFXEP_PNGOPT, false);
				toolbar->enableGroup("PNG", false);
			}

			// Refresh
			getImage()->open(entry_data, 0, format->getId());
			gfx_canvas->Refresh();
		}
	}

	// Unknown action
	else
		return false;

	// Action handled
	return true;
}

/* GfxEntryPanel::fillCustomMenu
 * Fills the given menu with the panel's custom actions. Used by
 * both the constructor to create the main window's custom menu,
 * and ArchivePanel::onEntryListRightClick to fill the context
 * menu with context-appropriate stuff
 *******************************************************************/
bool GfxEntryPanel::fillCustomMenu(wxMenu* custom)
{
	theApp->getAction("pgfx_mirror")->addToMenu(custom);
	theApp->getAction("pgfx_flip")->addToMenu(custom);
	theApp->getAction("pgfx_rotate")->addToMenu(custom);
	theApp->getAction("pgfx_convert")->addToMenu(custom);
	custom->AppendSeparator();
	theApp->getAction("pgfx_remap")->addToMenu(custom);
	theApp->getAction("pgfx_colourise")->addToMenu(custom);
	theApp->getAction("pgfx_tint")->addToMenu(custom);
	theApp->getAction("pgfx_crop")->addToMenu(custom);
	custom->AppendSeparator();
	theApp->getAction("pgfx_alph")->addToMenu(custom);
	theApp->getAction("pgfx_trns")->addToMenu(custom);
	theApp->getAction("pgfx_pngopt")->addToMenu(custom);
	custom->AppendSeparator();
	theApp->getAction("arch_gfx_exportpng")->addToMenu(custom);
	theApp->getAction("pgfx_extract")->addToMenu(custom);
	custom->AppendSeparator();
	theApp->getAction("arch_gfx_addptable")->addToMenu(custom);
	theApp->getAction("arch_gfx_addtexturex")->addToMenu(custom);
	// TODO: Should change the way gfx conversion and offset modification work so I can put them in this menu

	return true;
}

/*******************************************************************
 * GFXENTRYPANEL EVENTS
 *******************************************************************/

/* GfxEntryPanel::sliderZoomChanged
 * Called when the zoom slider is changed
 *******************************************************************/
void GfxEntryPanel::onZoomChanged(wxCommandEvent& e)
{
	// Get zoom value
	int zoom_percent = slider_zoom->GetValue();

	// Lock to 10% increments
	int remainder = zoom_percent % 10;
	zoom_percent -= remainder;

	// Update zoom label
	label_current_zoom->SetLabel(S_FMT("%d%%", zoom_percent));

	// Zoom gfx canvas and update
	gfx_canvas->setScale((double)zoom_percent * 0.01);
	gfx_canvas->Refresh();
}

/* GfxEntryPanel::onPaintColourChanged
 * Called when the colour box's value is changed
 *******************************************************************/
void GfxEntryPanel::onPaintColourChanged(wxEvent& e)
{
	gfx_canvas->setPaintColour(cb_colour->getColour());
}


/* GfxEntryPanel::onXOffsetChanged
 * Called when the X offset value is modified
 *******************************************************************/
void GfxEntryPanel::onXOffsetChanged(wxCommandEvent& e)
{
	// Change the image x-offset
	int offset = spin_xoffset->GetValue();
	getImage()->setXOffset(offset);

	// Update variables
	setModified();

	// Refresh canvas
	gfx_canvas->Refresh();
}

/* GfxEntryPanel::onYOffsetChanged
 * Called when the Y offset value is modified
 *******************************************************************/
void GfxEntryPanel::onYOffsetChanged(wxCommandEvent& e)
{
	// Change the image y-offset
	int offset = spin_yoffset->GetValue();
	getImage()->setYOffset(offset);

	// Update variables
	setModified();

	// Refresh canvas
	gfx_canvas->Refresh();
}

/* GfxEntryPanel::comboOffsetTypeChanged
 * Called when the 'type' combo box selection is changed
 *******************************************************************/
void GfxEntryPanel::onOffsetTypeChanged(wxCommandEvent& e)
{
	applyViewType();
}

/* GfxEntryPanel::cbTileChecked
 * Called when the 'Tile' checkbox is checked/unchecked
 *******************************************************************/
void GfxEntryPanel::onTileChanged(wxCommandEvent& e)
{
	choice_offset_type->Enable(!cb_tile->IsChecked());
	applyViewType();
}

/* GfxEntryPanel::onARCChanged
 * Called when the 'Aspect Ratio' checkbox is checked/unchecked
 *******************************************************************/
void GfxEntryPanel::onARCChanged(wxCommandEvent& e)
{
	gfx_arc = cb_arc->IsChecked();
	gfx_canvas->Refresh();
}

/* GfxEntryPanel::onGfxOffsetChanged
 * Called when the gfx canvas image offsets are changed
 *******************************************************************/
void GfxEntryPanel::onGfxOffsetChanged(wxEvent& e)
{
	// Update spin controls
	spin_xoffset->SetValue(getImage()->offset().x);
	spin_yoffset->SetValue(getImage()->offset().y);

	// Set changed
	setModified();
}

/* GfxEntryPanel::onGfxPixelsChanged
 * Called when pixels are changed in the canvas
 *******************************************************************/
void GfxEntryPanel::onGfxPixelsChanged(wxEvent& e)
{
	// Set changed
	image_data_modified = true;
	setModified();
}

/* GfxEntryPanel::onAnnouncement
 * Handles any announcements
 *******************************************************************/
void GfxEntryPanel::onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data)
{
	if (announcer != theMainWindow->getPaletteChooser())
		return;

	if (event_name == "main_palette_changed")
	{
		updateImagePalette();
		gfx_canvas->Refresh();
	}
}

/* GfxEntryPanel::onBtnNextImg
 * Called when the 'next image' button is clicked
 *******************************************************************/
void GfxEntryPanel::onBtnNextImg(wxCommandEvent& e)
{
	int num = gfx_canvas->getImage()->getSize();
	if (num > 1)
	{
		if (cur_index < num - 1)
			loadEntry(entry, cur_index + 1);
		else
			loadEntry(entry, 0);
	}
}

/* GfxEntryPanel::onBtnPrevImg
 * Called when the 'previous image' button is clicked
 *******************************************************************/
void GfxEntryPanel::onBtnPrevImg(wxCommandEvent& e)
{
	int num = gfx_canvas->getImage()->getSize();
	if (num > 1)
	{
		if (cur_index > 0)
			loadEntry(entry, cur_index - 1);
		else
			loadEntry(entry, num - 1);
	}
}

/* GfxEntryPanel::onBtnAutoOffset
 * Called when the 'modify offsets' button is clicked
 *******************************************************************/
void GfxEntryPanel::onBtnAutoOffset(wxCommandEvent& e)
{
	ModifyOffsetsDialog dlg;
	dlg.SetParent(theMainWindow);
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		// Calculate new offsets
		point2_t offsets = dlg.calculateOffsets(spin_xoffset->GetValue(), spin_yoffset->GetValue(),
			gfx_canvas->getImage()->getWidth(), gfx_canvas->getImage()->getHeight());

		// Change offsets
		spin_xoffset->SetValue(offsets.x);
		spin_yoffset->SetValue(offsets.y);
		getImage()->setXOffset(offsets.x);
		getImage()->setYOffset(offsets.y);
		refreshPanel();
		
		// Set changed
		setModified();
	}
}

/* GfxEntryPanel::onColourPicked
 * Called when a pixel's colour has been picked on the canvas
 *******************************************************************/
void GfxEntryPanel::onColourPicked(wxEvent & e)
{
	cb_colour->setColour(gfx_canvas->getPaintColour());
}



/*******************************************************************
 * EXTRA CONSOLE COMMANDS
 *******************************************************************/

// I'd love to put them in their own file, but attempting to do so
// results in a circular include nightmare and nothing works anymore.
#include "General/Console/Console.h"
#include "MainApp.h"
#include "MainEditor/UI/ArchivePanel.h"
#include "MainEditor/MainWindow.h"

GfxEntryPanel* CH::getCurrentGfxPanel()
{
	EntryPanel* panel = theActivePanel;
	if (panel)
	{
		if (!(panel->getName().CmpNoCase("gfx")))
		{
			return (GfxEntryPanel*)panel;
		}
	}
	return NULL;
}

CONSOLE_COMMAND(rotate, 1, true)
{
	double val;
	string bluh = args[0];
	if (!bluh.ToDouble(&val))
	{
		if (!bluh.CmpNoCase("l") || !bluh.CmpNoCase("left"))
			val = 90.;
		else if (!bluh.CmpNoCase("f") || !bluh.CmpNoCase("flip"))
			val = 180.;
		else if (!bluh.CmpNoCase("r") || !bluh.CmpNoCase("right"))
			val = 270.;
		else
		{
			wxLogMessage("Invalid parameter: %s is not a number.", bluh.mb_str());
			return;
		}
	}
	int angle = (int)val;
	if (angle % 90)
	{
		wxLogMessage("Invalid parameter: %i is not a multiple of 90.", angle);
		return;
	}

	ArchivePanel* foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		wxLogMessage("No active panel.");
		return;
	}
	ArchiveEntry* bar = foo->currentEntry();
	if (!bar)
	{
		wxLogMessage("No active entry.");
		return;
	}
	GfxEntryPanel* meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		wxLogMessage("No image selected.");
		return;
	}

	// Get current entry
	ArchiveEntry* entry = theMainWindow->getCurrentEntry();

	if (meep->getImage())
	{
		meep->getImage()->rotate(angle);
		meep->refresh();
		MemChunk mc;
		if (meep->getImage()->getFormat()->saveImage(*meep->getImage(), mc))
			bar->importMemChunk(mc);
	}
}

CONSOLE_COMMAND (mirror, 1, true)
{
	bool vertical;
	string bluh = args[0];
	if (!bluh.CmpNoCase("y") || !bluh.CmpNoCase("v") ||
	        !bluh.CmpNoCase("vert") || !bluh.CmpNoCase("vertical"))
		vertical = true;
	else if (!bluh.CmpNoCase("x") || !bluh.CmpNoCase("h") ||
	         !bluh.CmpNoCase("horz") || !bluh.CmpNoCase("horizontal"))
		vertical = false;
	else
	{
		wxLogMessage("Invalid parameter: %s is not a known value.", bluh.mb_str());
		return;
	}
	ArchivePanel* foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		wxLogMessage("No active panel.");
		return;
	}
	ArchiveEntry* bar = foo->currentEntry();
	if (!bar)
	{
		wxLogMessage("No active entry.");
		return;
	}
	GfxEntryPanel* meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		wxLogMessage("No image selected.");
		return;
	}
	if (meep->getImage())
	{
		meep->getImage()->mirror(vertical);
		meep->refresh();
		MemChunk mc;
		if (meep->getImage()->getFormat()->saveImage(*meep->getImage(), mc))
			bar->importMemChunk(mc);
	}
}

CONSOLE_COMMAND (crop, 4, true)
{
	long x1, y1, x2, y2;
	if (args[0].ToLong(&x1) && args[1].ToLong(&y1) && args[2].ToLong(&x2) && args[3].ToLong(&y2))
	{
		ArchivePanel* foo = CH::getCurrentArchivePanel();
		if (!foo)
		{
			wxLogMessage("No active panel.");
			return;
		}
		GfxEntryPanel* meep = CH::getCurrentGfxPanel();
		if (!meep)
		{
			wxLogMessage("No image selected.");
			return;
		}
		ArchiveEntry* bar = foo->currentEntry();
		if (!bar)
		{
			wxLogMessage("No active entry.");
			return;
		}
		if (meep->getImage())
		{
			meep->getImage()->crop(x1, y1, x2, y2);
			meep->refresh();
			MemChunk mc;
			if (meep->getImage()->getFormat()->saveImage(*meep->getImage(), mc))
				bar->importMemChunk(mc);
		}
	}
}

CONSOLE_COMMAND(adjust, 0, true)
{
	ArchivePanel* foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		wxLogMessage("No active panel.");
		return;
	}
	GfxEntryPanel* meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		wxLogMessage("No image selected.");
		return;
	}
	ArchiveEntry* bar = foo->currentEntry();
	if (!bar)
	{
		wxLogMessage("No active entry.");
		return;
	}
	if (meep->getImage())
	{
		meep->getImage()->adjust();
		meep->refresh();
		MemChunk mc;
		if (meep->getImage()->getFormat()->saveImage(*meep->getImage(), mc))
			bar->importMemChunk(mc);
	}
}

CONSOLE_COMMAND(mirrorpad, 0, true)
{
	ArchivePanel* foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		wxLogMessage("No active panel.");
		return;
	}
	GfxEntryPanel* meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		wxLogMessage("No image selected.");
		return;
	}
	ArchiveEntry* bar = foo->currentEntry();
	if (!bar)
	{
		wxLogMessage("No active entry.");
		return;
	}
	if (meep->getImage())
	{
		meep->getImage()->mirrorpad();
		meep->refresh();
		MemChunk mc;
		if (meep->getImage()->getFormat()->saveImage(*meep->getImage(), mc))
			bar->importMemChunk(mc);
	}
}

CONSOLE_COMMAND(imgconv, 0, true)
{
	ArchivePanel* foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		wxLogMessage("No active panel.");
		return;
	}
	ArchiveEntry* bar = foo->currentEntry();
	if (!bar)
	{
		wxLogMessage("No active entry.");
		return;
	}
	GfxEntryPanel* meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		wxLogMessage("No image selected.");
		return;
	}
	if (meep->getImage())
	{
		meep->getImage()->imgconv();
		meep->refresh();
		MemChunk mc;
		if (meep->getImage()->getFormat()->saveImage(*meep->getImage(), mc))
			bar->importMemChunk(mc);
	}
}
