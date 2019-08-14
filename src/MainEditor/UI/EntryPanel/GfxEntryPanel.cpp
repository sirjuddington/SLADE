
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxEntryPanel.cpp
// Description: GfxEntryPanel class. The UI for editing gfx entries.
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
#include "GfxEntryPanel.h"
#include "Archive/Archive.h"
#include "Dialogs/GfxColouriseDialog.h"
#include "Dialogs/GfxConvDialog.h"
#include "Dialogs/GfxCropDialog.h"
#include "Dialogs/GfxTintDialog.h"
#include "Dialogs/ModifyOffsetsDialog.h"
#include "Dialogs/TranslationEditorDialog.h"
#include "General/Console/ConsoleHelpers.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/SZoomSlider.h"
#include "UI/SBrush.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, gfx_arc)
EXTERN_CVAR(String, last_colour)
EXTERN_CVAR(String, last_tint_colour)
EXTERN_CVAR(Int, last_tint_amount)


// -----------------------------------------------------------------------------
//
// GfxEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxEntryPanel class constructor
// -----------------------------------------------------------------------------
GfxEntryPanel::GfxEntryPanel(wxWindow* parent) : EntryPanel(parent, "gfx")
{
	// Init variables
	prev_translation_.addRange(TransRange::Type::Palette, 0);
	edit_translation_.addRange(TransRange::Type::Palette, 0);

	// Add gfx canvas
	gfx_canvas_ = new GfxCanvas(this, -1);
	sizer_main_->Add(gfx_canvas_->toPanel(this), 1, wxEXPAND, 0);
	gfx_canvas_->setViewType(GfxCanvas::View::Default);
	gfx_canvas_->allowDrag(true);
	gfx_canvas_->allowScroll(true);
	gfx_canvas_->setPalette(MainEditor::currentPalette());
	gfx_canvas_->setTranslation(&edit_translation_);

	// Offsets
	wxSize spinsize = { UI::px(UI::Size::SpinCtrlWidth), -1 };
	spin_xoffset_   = new wxSpinCtrl(
        this,
        -1,
        wxEmptyString,
        wxDefaultPosition,
        wxDefaultSize,
        wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
        SHRT_MIN,
        SHRT_MAX,
        0);
	spin_yoffset_ = new wxSpinCtrl(
		this,
		-1,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize,
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		SHRT_MIN,
		SHRT_MAX,
		0);
	spin_xoffset_->SetMinSize(spinsize);
	spin_yoffset_->SetMinSize(spinsize);
	sizer_bottom_->Add(new wxStaticText(this, -1, "Offsets:"), 0, wxALIGN_CENTER_VERTICAL, 0);
	sizer_bottom_->Add(spin_xoffset_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, UI::pad());
	sizer_bottom_->Add(spin_yoffset_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());

	// Gfx (offset) type
	wxString offset_types[] = { "Auto", "Graphic", "Sprite", "HUD" };
	choice_offset_type_     = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 4, offset_types);
	choice_offset_type_->SetSelection(0);
	sizer_bottom_->Add(choice_offset_type_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());

	// Auto offset
	btn_auto_offset_ = new SIconButton(this, "offset", "Modify Offsets...");
	sizer_bottom_->Add(btn_auto_offset_, 0, wxALIGN_CENTER_VERTICAL);

	sizer_bottom_->AddStretchSpacer();

	// Aspect ratio correction checkbox
	cb_arc_ = new wxCheckBox(this, -1, "Aspect Ratio Correction");
	cb_arc_->SetValue(gfx_arc);
	sizer_bottom_->Add(cb_arc_, 0, wxEXPAND, 0);
	sizer_bottom_->AddSpacer(UI::padLarge());

	// Tile checkbox
	cb_tile_ = new wxCheckBox(this, -1, "Tile");
	sizer_bottom_->Add(cb_tile_, 0, wxEXPAND, 0);
	sizer_bottom_->AddSpacer(UI::padLarge());

	// Image selection buttons
	btn_nextimg_ = new SIconButton(this, "right");
	btn_previmg_ = new SIconButton(this, "left");
	text_curimg_ = new wxStaticText(this, -1, "Image XX/XX");
	sizer_bottom_->Add(btn_previmg_, 0, wxEXPAND | wxRIGHT, 4);
	sizer_bottom_->Add(btn_nextimg_, 0, wxEXPAND | wxRIGHT, 4);
	sizer_bottom_->Add(text_curimg_, 0, wxALIGN_CENTER, 0);
	btn_nextimg_->Show(false);
	btn_previmg_->Show(false);
	text_curimg_->Show(false);

	// Palette chooser
	listenTo(theMainWindow->paletteChooser());

	// Custom menu
	menu_custom_ = new wxMenu();
	GfxEntryPanel::fillCustomMenu(menu_custom_);
	custom_menu_name_ = "Graphic";

	// Brushes menu
	menu_brushes_ = new wxMenu();
	fillBrushMenu(menu_brushes_);

	// Custom toolbar
	setupToolbar();

	// Bind Events
	cb_colour_->Bind(wxEVT_COLOURBOX_CHANGED, &GfxEntryPanel::onPaintColourChanged, this);
	spin_xoffset_->Bind(wxEVT_SPINCTRL, &GfxEntryPanel::onXOffsetChanged, this);
	spin_yoffset_->Bind(wxEVT_SPINCTRL, &GfxEntryPanel::onYOffsetChanged, this);
	spin_xoffset_->Bind(wxEVT_TEXT_ENTER, &GfxEntryPanel::onXOffsetChanged, this);
	spin_yoffset_->Bind(wxEVT_TEXT_ENTER, &GfxEntryPanel::onYOffsetChanged, this);
	choice_offset_type_->Bind(wxEVT_CHOICE, &GfxEntryPanel::onOffsetTypeChanged, this);
	cb_tile_->Bind(wxEVT_CHECKBOX, &GfxEntryPanel::onTileChanged, this);
	cb_arc_->Bind(wxEVT_CHECKBOX, &GfxEntryPanel::onARCChanged, this);
	Bind(wxEVT_GFXCANVAS_OFFSET_CHANGED, &GfxEntryPanel::onGfxOffsetChanged, this, gfx_canvas_->GetId());
	Bind(wxEVT_GFXCANVAS_PIXELS_CHANGED, &GfxEntryPanel::onGfxPixelsChanged, this, gfx_canvas_->GetId());
	Bind(wxEVT_GFXCANVAS_COLOUR_PICKED, &GfxEntryPanel::onColourPicked, this, gfx_canvas_->GetId());
	btn_nextimg_->Bind(wxEVT_BUTTON, &GfxEntryPanel::onBtnNextImg, this);
	btn_previmg_->Bind(wxEVT_BUTTON, &GfxEntryPanel::onBtnPrevImg, this);
	btn_auto_offset_->Bind(wxEVT_BUTTON, &GfxEntryPanel::onBtnAutoOffset, this);

	// Apply layout
	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Loads an entry into the entry panel if it is a valid image format
// -----------------------------------------------------------------------------
bool GfxEntryPanel::loadEntry(ArchiveEntry* entry)
{
	return loadEntry(entry, 0);
}
bool GfxEntryPanel::loadEntry(ArchiveEntry* entry, int index)
{
	// Check entry was given
	if (!entry)
	{
		Global::error = "no entry to load";
		return false;
	}

	// Update variables
	setModified(false);

	// Attempt to load the image
	if (!Misc::loadImageFromEntry(image(), entry, index))
		return false;

	// Only show next/prev image buttons if the entry contains multiple images
	if (image()->size() > 1)
	{
		btn_nextimg_->Show();
		btn_previmg_->Show();
		text_curimg_->Show();
	}
	else
	{
		btn_nextimg_->Show(false);
		btn_previmg_->Show(false);
		text_curimg_->Show(false);
	}

	// Hack for colormaps to be 256-wide
	if (StrUtil::equalCI(entry->type()->name(), "colormap"))
		image()->setWidth(256);

	// Refresh everything
	refresh(entry);

	return true;
}

// -----------------------------------------------------------------------------
// Saves any changes to the entry
// -----------------------------------------------------------------------------
bool GfxEntryPanel::saveEntry()
{
	// Check entry is open
	auto entry = entry_.lock();
	if (!entry)
		return false;

	// Set offsets
	auto image = this->image();
	image->setXOffset(spin_xoffset_->GetValue());
	image->setYOffset(spin_yoffset_->GetValue());

	// Write new image data if modified
	bool ok = true;
	if (image_data_modified_)
	{
		auto format = image->format();

		wxString error = "";
		ok             = false;
		auto writable  = format ? format->canWrite(*image) : SIFormat::Writable::No;
		if (!format || format == SIFormat::unknownFormat())
			error = "Image is of unknown format";
		else if (writable == SIFormat::Writable::No)
			error = wxString::Format("Writing unsupported for format \"%s\"", format->name());
		else
		{
			// Convert image if necessary (using default options)
			if (writable == SIFormat::Writable::Convert)
			{
				format->convertWritable(*image, SIFormat::ConvertOptions());
				Log::info("Image converted for writing");
			}

			if (format->saveImage(*image, entry->data(), &gfx_canvas_->palette()))
				ok = true;
			else
				error = "Error writing image";
		}

		if (ok)
		{
			// Set modified
			entry->setState(ArchiveEntry::State::Modified);

			// Re-detect type
			auto oldtype = entry->type();
			EntryType::detectEntryType(*entry);

			// Update extension if type changed
			if (oldtype != entry->type())
				entry->setExtensionByType();
		}
		else
			wxMessageBox(wxString("Cannot save changes to image: ") + error, "Error", wxICON_ERROR);
	}
	// Otherwise just set offsets
	else
		EntryOperations::setGfxOffsets(entry.get(), spin_xoffset_->GetValue(), spin_yoffset_->GetValue());

	// Apply alPh/tRNS options
	if (entry->type()->formatId() == "img_png")
	{
		bool alph = EntryOperations::getalPhChunk(entry.get());
		bool trns = EntryOperations::gettRNSChunk(entry.get());

		if (alph != menu_custom_->IsChecked(SAction::fromId("pgfx_alph")->wxId()))
			EntryOperations::modifyalPhChunk(entry.get(), !alph);
		if (trns != menu_custom_->IsChecked(SAction::fromId("pgfx_trns")->wxId()))
			EntryOperations::modifytRNSChunk(entry.get(), !trns);
	}

	if (ok)
		setModified(false);

	return ok;
}

// -----------------------------------------------------------------------------
// Adds controls to the entry panel toolbar
// -----------------------------------------------------------------------------
void GfxEntryPanel::setupToolbar()
{
	// Zoom
	auto g_zoom  = new SToolBarGroup(toolbar_, "Zoom");
	slider_zoom_ = new SZoomSlider(g_zoom, gfx_canvas_);
	g_zoom->addCustomControl(slider_zoom_);
	toolbar_->addGroup(g_zoom);

	// Editing operations
	auto g_edit = new SToolBarGroup(toolbar_, "Editing");
	g_edit->addActionButton("pgfx_settrans", "");
	cb_colour_ = new ColourBox(g_edit, -1, ColRGBA::BLACK, false, true);
	cb_colour_->setPalette(&gfx_canvas_->palette());
	button_brush_ = g_edit->addActionButton("pgfx_setbrush", "");
	g_edit->addCustomControl(cb_colour_);
	g_edit->addActionButton("pgfx_drag", "");
	g_edit->addActionButton("pgfx_draw", "");
	g_edit->addActionButton("pgfx_erase", "");
	g_edit->addActionButton("pgfx_magic", "");
	SAction::fromId("pgfx_drag")->setChecked(); // Drag offsets by default
	toolbar_->addGroup(g_edit);

	// Image operations
	auto g_image = new SToolBarGroup(toolbar_, "Image");
	g_image->addActionButton("pgfx_mirror", "");
	g_image->addActionButton("pgfx_flip", "");
	g_image->addActionButton("pgfx_rotate", "");
	g_image->addActionButton("pgfx_crop", "");
	g_image->addActionButton("pgfx_convert", "");
	toolbar_->addGroup(g_image);

	// Colour operations
	auto g_colour = new SToolBarGroup(toolbar_, "Colour");
	g_colour->addActionButton("pgfx_remap", "");
	g_colour->addActionButton("pgfx_colourise", "");
	g_colour->addActionButton("pgfx_tint", "");
	toolbar_->addGroup(g_colour);

	// Misc operations
	auto g_png = new SToolBarGroup(toolbar_, "PNG");
	g_png->addActionButton("pgfx_pngopt", "");
	toolbar_->addGroup(g_png);
	toolbar_->enableGroup("PNG", false);
}

// -----------------------------------------------------------------------------
// Fills the brush menu with available brushes
// -----------------------------------------------------------------------------
void GfxEntryPanel::fillBrushMenu(wxMenu* bm) const
{
	SAction::fromId("pgfx_brush_sq_1")->addToMenu(bm);
	SAction::fromId("pgfx_brush_sq_3")->addToMenu(bm);
	SAction::fromId("pgfx_brush_sq_5")->addToMenu(bm);
	SAction::fromId("pgfx_brush_sq_7")->addToMenu(bm);
	SAction::fromId("pgfx_brush_sq_9")->addToMenu(bm);
	SAction::fromId("pgfx_brush_ci_5")->addToMenu(bm);
	SAction::fromId("pgfx_brush_ci_7")->addToMenu(bm);
	SAction::fromId("pgfx_brush_ci_9")->addToMenu(bm);
	SAction::fromId("pgfx_brush_di_3")->addToMenu(bm);
	SAction::fromId("pgfx_brush_di_5")->addToMenu(bm);
	SAction::fromId("pgfx_brush_di_7")->addToMenu(bm);
	SAction::fromId("pgfx_brush_di_9")->addToMenu(bm);
	wxMenu* pa = new wxMenu;
	SAction::fromId("pgfx_brush_pa_a")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_b")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_c")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_d")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_e")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_f")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_g")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_h")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_i")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_j")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_k")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_l")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_m")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_n")->addToMenu(pa);
	SAction::fromId("pgfx_brush_pa_o")->addToMenu(pa);
	bm->AppendSubMenu(pa, "Dither Patterns");
}

// -----------------------------------------------------------------------------
// Extract all sub-images as individual PNGs
// -----------------------------------------------------------------------------
bool GfxEntryPanel::extractAll() const
{
	auto entry = entry_.lock();
	if (!entry)
		return false;

	if (image()->size() < 2)
		return false;

	// Remember where we are
	int imgindex = image()->index();

	auto parent = entry->parent();
	if (parent == nullptr)
		return false;

	int      index = parent->entryIndex(entry.get(), entry->parentDir());
	wxString name  = wxFileName(entry->name()).GetName();

	// Loop through subimages and get things done
	int pos = 0;
	for (int i = 0; i < image()->size(); ++i)
	{
		wxString newname = wxString::Format("%s_%i.png", name, i);
		Misc::loadImageFromEntry(image(), entry.get(), i);

		// Only process images that actually contain some pixels
		if (image()->width() && image()->height())
		{
			auto newimg = parent->addNewEntry(newname.ToStdString(), index + pos + 1, entry->parentDir());
			if (newimg == nullptr)
				return false;
			SIFormat::getFormat("png")->saveImage(*image(), newimg->data(), &gfx_canvas_->palette());
			EntryType::detectEntryType(*newimg);
			pos++;
		}
	}

	// Reload image of where we were
	Misc::loadImageFromEntry(image(), entry.get(), imgindex);

	return true;
}

// -----------------------------------------------------------------------------
// Reloads image data and force refresh
// -----------------------------------------------------------------------------
void GfxEntryPanel::refresh(ArchiveEntry* entry)
{
	// Get entry to use
	if (!entry)
		entry = entry_.lock().get();
	if (!entry)
		return;

	// Setup palette
	theMainWindow->paletteChooser()->setGlobalFromArchive(entry->parent(), Misc::detectPaletteHack(entry));
	updateImagePalette();

	// Set offset text boxes
	spin_xoffset_->SetValue(image()->offset().x);
	spin_yoffset_->SetValue(image()->offset().y);

	// Get some needed menu ids
	int menu_gfxep_pngopt      = SAction::fromId("pgfx_pngopt")->wxId();
	int menu_gfxep_alph        = SAction::fromId("pgfx_alph")->wxId();
	int menu_gfxep_trns        = SAction::fromId("pgfx_trns")->wxId();
	int menu_gfxep_extract     = SAction::fromId("pgfx_extract")->wxId();
	int menu_gfxep_translate   = SAction::fromId("pgfx_remap")->wxId();
	int menu_archgfx_exportpng = SAction::fromId("arch_gfx_exportpng")->wxId();

	// Set PNG check menus
	if (entry->type() != nullptr && entry->type()->formatId() == "img_png")
	{
		// Check for alph
		alph_ = EntryOperations::getalPhChunk(entry);
		menu_custom_->Enable(menu_gfxep_alph, true);
		menu_custom_->Check(menu_gfxep_alph, alph_);

		// Check for trns
		trns_ = EntryOperations::gettRNSChunk(entry);
		menu_custom_->Enable(menu_gfxep_trns, true);
		menu_custom_->Check(menu_gfxep_trns, trns_);

		// Disable 'Export as PNG' (it already is :P)
		menu_custom_->Enable(menu_archgfx_exportpng, false);

		// Add 'Optimize PNG' option
		menu_custom_->Enable(menu_gfxep_pngopt, true);
		toolbar_->enableGroup("PNG", true);
	}
	else
	{
		menu_custom_->Enable(menu_gfxep_alph, false);
		menu_custom_->Enable(menu_gfxep_trns, false);
		menu_custom_->Check(menu_gfxep_alph, false);
		menu_custom_->Check(menu_gfxep_trns, false);
		menu_custom_->Enable(menu_gfxep_pngopt, false);
		menu_custom_->Enable(menu_archgfx_exportpng, true);
		toolbar_->enableGroup("PNG", false);
	}

	// Set multi-image format stuff thingies
	cur_index_ = image()->index();
	if (image()->size() > 1)
		menu_custom_->Enable(menu_gfxep_extract, true);
	else
		menu_custom_->Enable(menu_gfxep_extract, false);
	text_curimg_->SetLabel(wxString::Format("Image %d/%d", cur_index_ + 1, image()->size()));

	// Update status bar in case image dimensions changed
	updateStatus();

	// Apply offset view type
	applyViewType();

	// Reset display offsets in graphics mode
	if (gfx_canvas_->viewType() != GfxCanvas::View::Sprite)
		gfx_canvas_->resetOffsets();

	// Setup custom menu
	if (image()->type() == SImage::Type::RGBA)
		menu_custom_->Enable(menu_gfxep_translate, false);
	else
		menu_custom_->Enable(menu_gfxep_translate, true);

	// Refresh the canvas
	gfx_canvas_->Refresh();
}

// -----------------------------------------------------------------------------
// Returns a string with extended editing/entry info for the status bar
// -----------------------------------------------------------------------------
wxString GfxEntryPanel::statusString()
{
	// Setup status string
	auto     image  = this->image();
	wxString status = wxString::Format("%dx%d", image->width(), image->height());

	// Colour format
	if (image->type() == SImage::Type::RGBA)
		status += ", 32bpp";
	else
		status += ", 8bpp";

	// PNG stuff
	if (auto entry = entry_.lock(); entry->type()->formatId() == "img_png")
	{
		// alPh
		if (EntryOperations::getalPhChunk(entry.get()))
			status += ", alPh";

		// tRNS
		if (EntryOperations::gettRNSChunk(entry.get()))
			status += ", tRNS";
	}

	return status;
}

// -----------------------------------------------------------------------------
// Redraws the panel
// -----------------------------------------------------------------------------
void GfxEntryPanel::refreshPanel()
{
	Update();
	Refresh();
}

// -----------------------------------------------------------------------------
// Sets the gfx canvas' palette to what is selected in the palette chooser, and
// refreshes the gfx canvas
// -----------------------------------------------------------------------------
void GfxEntryPanel::updateImagePalette() const
{
	gfx_canvas_->setPalette(MainEditor::currentPalette());
	gfx_canvas_->updateImageTexture();
}

// -----------------------------------------------------------------------------
// Detects the offset view type of the current entry
// -----------------------------------------------------------------------------
GfxCanvas::View GfxEntryPanel::detectOffsetType() const
{
	auto entry = entry_.lock();

	if (!entry)
		return GfxCanvas::View::Default;

	if (!entry->parent())
		return GfxCanvas::View::Default;

	// Check what section of the archive the entry is in -- only PNGs or images
	// in the sprites section can be HUD or sprite
	bool is_sprite = ("sprites" == entry->parent()->detectNamespace(entry.get()));
	bool is_png    = ("img_png" == entry->type()->formatId());
	if (!is_sprite && !is_png)
		return GfxCanvas::View::Default;

	auto img = image();
	if (is_png && img->offset().x == 0 && img->offset().y == 0)
		return GfxCanvas::View::Default;

	int width        = img->width();
	int height       = img->height();
	int left         = -img->offset().x;
	int right        = left + width;
	int top          = -img->offset().y;
	int bottom       = top + height;
	int horiz_center = (left + right) / 2;

	// Determine sprite vs. HUD with a rough heuristic: give each one a
	// penalty, measuring how far (in pixels) the offsets are from the "ideal"
	// offsets for that type.  Lowest penalty wins.
	int sprite_penalty = 0;
	int hud_penalty    = 0;
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
		return GfxCanvas::View::HUD;
	else
		return GfxCanvas::View::Sprite;
}

// -----------------------------------------------------------------------------
// Sets the view type of the gfx canvas depending on what is selected in the
// offset type combo box
// -----------------------------------------------------------------------------
void GfxEntryPanel::applyViewType() const
{
	// Tile checkbox overrides offset type selection
	if (cb_tile_->IsChecked())
		gfx_canvas_->setViewType(GfxCanvas::View::Tiled);
	else
	{
		// Set gfx canvas view type depending on the offset combobox selection
		int sel = choice_offset_type_->GetSelection();
		switch (sel)
		{
		case 0: gfx_canvas_->setViewType(detectOffsetType()); break;
		case 1: gfx_canvas_->setViewType(GfxCanvas::View::Default); break;
		case 2: gfx_canvas_->setViewType(GfxCanvas::View::Sprite); break;
		case 3: gfx_canvas_->setViewType(GfxCanvas::View::HUD); break;
		default: break;
		}
	}

	// Refresh
	gfx_canvas_->Refresh();
}

// ----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// ----------------------------------------------------------------------------
bool GfxEntryPanel::handleEntryPanelAction(string_view id)
{
	// We're only interested in "pgfx_" actions
	if (!StrUtil::startsWith(id, "pgfx_"))
		return false;

	auto entry = entry_.lock();

	// For pgfx_brush actions, the string after pgfx is a brush name
	if (StrUtil::startsWith(id, "pgfx_brush"))
	{
		gfx_canvas_->setBrush(SBrush::get(string{ id }));
		button_brush_->setIcon(StrUtil::afterFirst(id, '_'));
	}

	// Editing - drag mode
	else if (id == "pgfx_drag")
	{
		editing_ = false;
		gfx_canvas_->setEditingMode(GfxCanvas::EditMode::None);
	}

	// Editing - draw mode
	else if (id == "pgfx_draw")
	{
		editing_ = true;
		gfx_canvas_->setEditingMode(GfxCanvas::EditMode::Paint);
		gfx_canvas_->setPaintColour(cb_colour_->colour());
	}

	// Editing - erase mode
	else if (id == "pgfx_erase")
	{
		editing_ = true;
		gfx_canvas_->setEditingMode(GfxCanvas::EditMode::Erase);
	}

	// Editing - translate mode
	else if (id == "pgfx_magic")
	{
		editing_ = true;
		gfx_canvas_->setEditingMode(GfxCanvas::EditMode::Translate);
	}

	// Editing - set translation
	else if (id == "pgfx_settrans")
	{
		// Create translation editor dialog
		TranslationEditorDialog ted(
			theMainWindow, *theMainWindow->paletteChooser()->selectedPalette(), " Colour Remap", image());

		// Create translation to edit
		ted.openTranslation(edit_translation_);

		// Show the dialog
		if (ted.ShowModal() == wxID_OK)
		{
			// Set the translation
			edit_translation_.copy(ted.getTranslation());
			gfx_canvas_->setTranslation(&edit_translation_);
		}
	}

	// Editing - set brush
	else if (id == "pgfx_setbrush")
	{
		auto p = button_brush_->GetScreenPosition() -= GetScreenPosition();
		p.y += button_brush_->GetMaxHeight();
		PopupMenu(menu_brushes_, p);
	}

	// Mirror
	else if (id == "pgfx_mirror")
	{
		// Mirror X
		image()->mirror(false);

		// Update UI
		gfx_canvas_->updateImageTexture();
		gfx_canvas_->Refresh();

		// Update variables
		image_data_modified_ = true;
		setModified();
	}

	// Flip
	else if (id == "pgfx_flip")
	{
		// Mirror Y
		image()->mirror(true);

		// Update UI
		gfx_canvas_->updateImageTexture();
		gfx_canvas_->Refresh();

		// Update variables
		image_data_modified_ = true;
		setModified();
	}

	// Rotate
	else if (id == "pgfx_rotate")
	{
		// Prompt for rotation angle
		wxString angles[] = { "90", "180", "270" };
		int      choice   = wxGetSingleChoiceIndex("Select rotation angle", "Rotate", 3, angles, 0);

		// Rotate image
		switch (choice)
		{
		case 0: image()->rotate(90); break;
		case 1: image()->rotate(180); break;
		case 2: image()->rotate(270); break;
		default: break;
		}

		// Update UI
		gfx_canvas_->updateImageTexture();
		gfx_canvas_->Refresh();

		// Update variables
		image_data_modified_ = true;
		setModified();
	}

	// Translate
	else if (id == "pgfx_remap")
	{
		// Create translation editor dialog
		auto                    pal = MainEditor::currentPalette();
		TranslationEditorDialog ted(theMainWindow, *pal, " Colour Remap", &gfx_canvas_->image());

		// Create translation to edit
		ted.openTranslation(prev_translation_);

		// Show the dialog
		if (ted.ShowModal() == wxID_OK)
		{
			// Apply translation to image
			image()->applyTranslation(&ted.getTranslation(), pal);

			// Update UI
			gfx_canvas_->updateImageTexture();
			gfx_canvas_->Refresh();

			// Update variables
			image_data_modified_ = true;
			gfx_canvas_->updateImageTexture();
			setModified();
			prev_translation_.copy(ted.getTranslation());
		}
	}

	// Colourise
	else if (id == "pgfx_colourise" && entry)
	{
		auto               pal = MainEditor::currentPalette();
		GfxColouriseDialog gcd(theMainWindow, entry.get(), *pal);
		gcd.setColour(last_colour);

		// Show colourise dialog
		if (gcd.ShowModal() == wxID_OK)
		{
			// Colourise image
			image()->colourise(gcd.colour(), pal);

			// Update UI
			gfx_canvas_->updateImageTexture();
			gfx_canvas_->Refresh();

			// Update variables
			image_data_modified_ = true;
			Refresh();
			setModified();
		}
		last_colour = gcd.colour().toString(ColRGBA::StringFormat::RGB);
	}

	// Tint
	else if (id == "pgfx_tint" && entry)
	{
		auto          pal = MainEditor::currentPalette();
		GfxTintDialog gtd(theMainWindow, entry.get(), *pal);
		gtd.setValues(last_tint_colour, last_tint_amount);

		// Show tint dialog
		if (gtd.ShowModal() == wxID_OK)
		{
			// Tint image
			image()->tint(gtd.colour(), gtd.amount(), pal);

			// Update UI
			gfx_canvas_->updateImageTexture();
			gfx_canvas_->Refresh();

			// Update variables
			image_data_modified_ = true;
			Refresh();
			setModified();
		}
		last_tint_colour = gtd.colour().toString(ColRGBA::StringFormat::RGB);
		last_tint_amount = (int)(gtd.amount() * 100.0);
	}

	// Crop
	else if (id == "pgfx_crop")
	{
		auto          image = this->image();
		auto          pal   = MainEditor::currentPalette();
		GfxCropDialog gcd(theMainWindow, image, pal);

		// Show crop dialog
		if (gcd.ShowModal() == wxID_OK)
		{
			// Prompt to adjust offsets
			auto crop = gcd.cropRect();
			if (crop.tl.x > 0 || crop.tl.y > 0)
			{
				if (wxMessageBox(
						"Do you want to adjust the offsets? This will keep the graphic in the same relative "
						"position it was before cropping.",
						"Adjust Offsets?",
						wxYES_NO)
					== wxYES)
				{
					image->setXOffset(image->offset().x - crop.tl.x);
					image->setYOffset(image->offset().y - crop.tl.y);
				}
			}

			// Crop image
			image->crop(crop.x1(), crop.y1(), crop.x2(), crop.y2());

			// Update UI
			gfx_canvas_->updateImageTexture();
			gfx_canvas_->Refresh();

			// Update variables
			image_data_modified_ = true;
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
	else if (id == "pgfx_pngopt" && entry)
	{
		// This is a special case. If we set the entry as modified, SLADE will prompt
		// to save it, rewriting the entry and cancelling the optimization done...
		if (EntryOperations::optimizePNG(entry.get()))
			setModified(false);
		else
			wxMessageBox(
				"Warning: Couldn't optimize this image, check console log for info",
				"Warning",
				wxOK | wxCENTRE | wxICON_WARNING);
		Refresh();
	}

	// Extract all
	else if (id == "pgfx_extract")
	{
		extractAll();
	}

	// Convert
	else if (id == "pgfx_convert" && entry)
	{
		GfxConvDialog gcd(theMainWindow);
		gcd.CenterOnParent();
		gcd.openEntry(entry.get());

		gcd.ShowModal();

		if (gcd.itemModified(0))
		{
			// Get image and conversion info
			auto image  = gcd.itemImage(0);
			auto format = gcd.itemFormat(0);

			// Write converted image back to entry
			format->saveImage(*image, entry_data_, gcd.itemPalette(0));
			// This makes the "save" button (and the setModified stuff) redundant and confusing!
			// The alternative is to save to entry effectively (uncomment the importMemChunk line)
			// but remove the setModified and image_data_modified lines, and add a call to refresh
			// to get the PNG tRNS status back in sync.
			// entry->importMemChunk(entry_data);
			image_data_modified_ = true;
			setModified();

			// Fix tRNS status if we converted to paletted PNG
			int MENU_GFXEP_PNGOPT      = SAction::fromId("pgfx_pngopt")->wxId();
			int MENU_GFXEP_ALPH        = SAction::fromId("pgfx_alph")->wxId();
			int MENU_GFXEP_TRNS        = SAction::fromId("pgfx_trns")->wxId();
			int MENU_ARCHGFX_EXPORTPNG = SAction::fromId("arch_gfx_exportpng")->wxId();
			if (format->name() == "PNG")
			{
				ArchiveEntry temp;
				temp.importMemChunk(entry_data_);
				temp.setType(EntryType::fromId("png"));
				menu_custom_->Enable(MENU_GFXEP_ALPH, true);
				menu_custom_->Enable(MENU_GFXEP_TRNS, true);
				menu_custom_->Check(MENU_GFXEP_TRNS, EntryOperations::gettRNSChunk(&temp));
				menu_custom_->Enable(MENU_ARCHGFX_EXPORTPNG, false);
				menu_custom_->Enable(MENU_GFXEP_PNGOPT, true);
				toolbar_->enableGroup("PNG", true);
			}
			else
			{
				menu_custom_->Enable(MENU_GFXEP_ALPH, false);
				menu_custom_->Enable(MENU_GFXEP_TRNS, false);
				menu_custom_->Enable(MENU_ARCHGFX_EXPORTPNG, true);
				menu_custom_->Enable(MENU_GFXEP_PNGOPT, false);
				toolbar_->enableGroup("PNG", false);
			}

			// Refresh
			this->image()->open(entry_data_, 0, format->id());
			gfx_canvas_->Refresh();
		}
	}

	// Unknown action
	else
		return false;

	// Action handled
	return true;
}

// -----------------------------------------------------------------------------
// Fills the given menu with the panel's custom actions. Used by both the
// constructor to create the main window's custom menu, and
// ArchivePanel::onEntryListRightClick to fill the context menu with
// context-appropriate stuff
// -----------------------------------------------------------------------------
bool GfxEntryPanel::fillCustomMenu(wxMenu* custom)
{
	SAction::fromId("pgfx_mirror")->addToMenu(custom);
	SAction::fromId("pgfx_flip")->addToMenu(custom);
	SAction::fromId("pgfx_rotate")->addToMenu(custom);
	SAction::fromId("pgfx_convert")->addToMenu(custom);
	custom->AppendSeparator();
	SAction::fromId("pgfx_remap")->addToMenu(custom);
	SAction::fromId("pgfx_colourise")->addToMenu(custom);
	SAction::fromId("pgfx_tint")->addToMenu(custom);
	SAction::fromId("pgfx_crop")->addToMenu(custom);
	custom->AppendSeparator();
	SAction::fromId("pgfx_alph")->addToMenu(custom);
	SAction::fromId("pgfx_trns")->addToMenu(custom);
	SAction::fromId("pgfx_pngopt")->addToMenu(custom);
	custom->AppendSeparator();
	SAction::fromId("arch_gfx_exportpng")->addToMenu(custom);
	SAction::fromId("pgfx_extract")->addToMenu(custom);
	custom->AppendSeparator();
	SAction::fromId("arch_gfx_addptable")->addToMenu(custom);
	SAction::fromId("arch_gfx_addtexturex")->addToMenu(custom);
	// TODO: Should change the way gfx conversion and offset modification work so I can put them in this menu

	return true;
}


// -----------------------------------------------------------------------------
//
// GfxEntryPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the colour box's value is changed
// -----------------------------------------------------------------------------
void GfxEntryPanel::onPaintColourChanged(wxEvent& e)
{
	gfx_canvas_->setPaintColour(cb_colour_->colour());
}

// -----------------------------------------------------------------------------
// Called when the X offset value is modified
// -----------------------------------------------------------------------------
void GfxEntryPanel::onXOffsetChanged(wxCommandEvent& e)
{
	// Ignore if the value wasn't changed
	int offset = spin_xoffset_->GetValue();
	if (offset == image()->offset().x)
		return;

	// Update offset & refresh
	image()->setXOffset(offset);
	setModified();
	gfx_canvas_->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the Y offset value is modified
// -----------------------------------------------------------------------------
void GfxEntryPanel::onYOffsetChanged(wxCommandEvent& e)
{
	// Ignore if the value wasn't changed
	int offset = spin_yoffset_->GetValue();
	if (offset == image()->offset().y)
		return;

	// Update offset & refresh
	image()->setYOffset(offset);
	setModified();
	gfx_canvas_->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the 'type' combo box selection is changed
// -----------------------------------------------------------------------------
void GfxEntryPanel::onOffsetTypeChanged(wxCommandEvent& e)
{
	applyViewType();
}

// -----------------------------------------------------------------------------
// Called when the 'Tile' checkbox is checked/unchecked
// -----------------------------------------------------------------------------
void GfxEntryPanel::onTileChanged(wxCommandEvent& e)
{
	choice_offset_type_->Enable(!cb_tile_->IsChecked());
	applyViewType();
}

// -----------------------------------------------------------------------------
// Called when the 'Aspect Ratio' checkbox is checked/unchecked
// -----------------------------------------------------------------------------
void GfxEntryPanel::onARCChanged(wxCommandEvent& e)
{
	gfx_arc = cb_arc_->IsChecked();
	gfx_canvas_->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the gfx canvas image offsets are changed
// -----------------------------------------------------------------------------
void GfxEntryPanel::onGfxOffsetChanged(wxEvent& e)
{
	// Update spin controls
	spin_xoffset_->SetValue(image()->offset().x);
	spin_yoffset_->SetValue(image()->offset().y);

	// Set changed
	setModified();
}

// -----------------------------------------------------------------------------
// Called when pixels are changed in the canvas
// -----------------------------------------------------------------------------
void GfxEntryPanel::onGfxPixelsChanged(wxEvent& e)
{
	// Set changed
	image_data_modified_ = true;
	setModified();
}

// -----------------------------------------------------------------------------
// Handles any announcements
// -----------------------------------------------------------------------------
void GfxEntryPanel::onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data)
{
	if (announcer != theMainWindow->paletteChooser())
		return;

	if (event_name == "main_palette_changed")
	{
		updateImagePalette();
		gfx_canvas_->Refresh();
	}
}

// -----------------------------------------------------------------------------
// Called when the 'next image' button is clicked
// -----------------------------------------------------------------------------
void GfxEntryPanel::onBtnNextImg(wxCommandEvent& e)
{
	int  num   = gfx_canvas_->image().size();
	auto entry = entry_.lock().get();
	if (num > 1 && entry)
	{
		if (cur_index_ < num - 1)
			loadEntry(entry, cur_index_ + 1);
		else
			loadEntry(entry, 0);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'previous image' button is clicked
// -----------------------------------------------------------------------------
void GfxEntryPanel::onBtnPrevImg(wxCommandEvent& e)
{
	int  num   = gfx_canvas_->image().size();
	auto entry = entry_.lock().get();
	if (num > 1 && entry)
	{
		if (cur_index_ > 0)
			loadEntry(entry, cur_index_ - 1);
		else
			loadEntry(entry, num - 1);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'modify offsets' button is clicked
// -----------------------------------------------------------------------------
void GfxEntryPanel::onBtnAutoOffset(wxCommandEvent& e)
{
	ModifyOffsetsDialog dlg;
	dlg.SetParent(theMainWindow);
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		// Calculate new offsets
		Vec2i offsets = dlg.calculateOffsets(
			spin_xoffset_->GetValue(),
			spin_yoffset_->GetValue(),
			gfx_canvas_->image().width(),
			gfx_canvas_->image().height());

		// Change offsets
		spin_xoffset_->SetValue(offsets.x);
		spin_yoffset_->SetValue(offsets.y);
		image()->setXOffset(offsets.x);
		image()->setYOffset(offsets.y);
		refreshPanel();

		// Set changed
		setModified();
	}
}

// -----------------------------------------------------------------------------
// Called when a pixel's colour has been picked on the canvas
// -----------------------------------------------------------------------------
void GfxEntryPanel::onColourPicked(wxEvent& e)
{
	cb_colour_->setColour(gfx_canvas_->paintColour());
}



// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

// I'd love to put them in their own file, but attempting to do so
// results in a circular include nightmare and nothing works anymore.
#include "General/Console/Console.h"
#include "MainEditor/UI/ArchivePanel.h"

GfxEntryPanel* CH::getCurrentGfxPanel()
{
	auto panel = MainEditor::currentEntryPanel();
	if (panel)
	{
		if (!(panel->name().CmpNoCase("gfx")))
		{
			return static_cast<GfxEntryPanel*>(panel);
		}
	}
	return nullptr;
}

CONSOLE_COMMAND(rotate, 1, true)
{
	double   val;
	wxString bluh = args[0];
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
			Log::error(wxString::Format("Invalid parameter: %s is not a number.", bluh.mb_str()));
			return;
		}
	}
	int angle = (int)val;
	if (angle % 90)
	{
		Log::error(wxString::Format("Invalid parameter: %i is not a multiple of 90.", angle));
		return;
	}

	auto foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		Log::info(1, "No active panel.");
		return;
	}
	auto bar = foo->currentEntry();
	if (!bar)
	{
		Log::info(1, "No active entry.");
		return;
	}
	auto meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		Log::info(1, "No image selected.");
		return;
	}

	// Get current entry
	auto entry = MainEditor::currentEntry();

	if (meep->image())
	{
		meep->image()->rotate(angle);
		meep->refresh();
		MemChunk mc;
		if (meep->image()->format()->saveImage(*meep->image(), mc))
			bar->importMemChunk(mc);
	}
}

CONSOLE_COMMAND(mirror, 1, true)
{
	bool     vertical;
	wxString bluh = args[0];
	if (!bluh.CmpNoCase("y") || !bluh.CmpNoCase("v") || !bluh.CmpNoCase("vert") || !bluh.CmpNoCase("vertical"))
		vertical = true;
	else if (!bluh.CmpNoCase("x") || !bluh.CmpNoCase("h") || !bluh.CmpNoCase("horz") || !bluh.CmpNoCase("horizontal"))
		vertical = false;
	else
	{
		Log::error(wxString::Format("Invalid parameter: %s is not a known value.", bluh.mb_str()));
		return;
	}
	auto foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		Log::info(1, "No active panel.");
		return;
	}
	auto bar = foo->currentEntry();
	if (!bar)
	{
		Log::info(1, "No active entry.");
		return;
	}
	auto meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		Log::info(1, "No image selected.");
		return;
	}
	if (meep->image())
	{
		meep->image()->mirror(vertical);
		meep->refresh();
		MemChunk mc;
		if (meep->image()->format()->saveImage(*meep->image(), mc))
			bar->importMemChunk(mc);
	}
}

CONSOLE_COMMAND(crop, 4, true)
{
	int x1, y1, x2, y2;
	if (StrUtil::toInt(args[0], x1) && StrUtil::toInt(args[1], y1) && StrUtil::toInt(args[2], x2)
		&& StrUtil::toInt(args[3], y2))
	{
		auto foo = CH::getCurrentArchivePanel();
		if (!foo)
		{
			Log::info(1, "No active panel.");
			return;
		}
		auto meep = CH::getCurrentGfxPanel();
		if (!meep)
		{
			Log::info(1, "No image selected.");
			return;
		}
		auto bar = foo->currentEntry();
		if (!bar)
		{
			Log::info(1, "No active entry.");
			return;
		}
		if (meep->image())
		{
			meep->image()->crop(x1, y1, x2, y2);
			meep->refresh();
			MemChunk mc;
			if (meep->image()->format()->saveImage(*meep->image(), mc))
				bar->importMemChunk(mc);
		}
	}
}

CONSOLE_COMMAND(adjust, 0, true)
{
	auto foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		Log::info(1, "No active panel.");
		return;
	}
	auto meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		Log::info(1, "No image selected.");
		return;
	}
	auto bar = foo->currentEntry();
	if (!bar)
	{
		Log::info(1, "No active entry.");
		return;
	}
	if (meep->image())
	{
		meep->image()->adjust();
		meep->refresh();
		MemChunk mc;
		if (meep->image()->format()->saveImage(*meep->image(), mc))
			bar->importMemChunk(mc);
	}
}

CONSOLE_COMMAND(mirrorpad, 0, true)
{
	auto foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		Log::info(1, "No active panel.");
		return;
	}
	auto meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		Log::info(1, "No image selected.");
		return;
	}
	auto bar = foo->currentEntry();
	if (!bar)
	{
		Log::info(1, "No active entry.");
		return;
	}
	if (meep->image())
	{
		meep->image()->mirrorpad();
		meep->refresh();
		MemChunk mc;
		if (meep->image()->format()->saveImage(*meep->image(), mc))
			bar->importMemChunk(mc);
	}
}

CONSOLE_COMMAND(imgconv, 0, true)
{
	auto foo = CH::getCurrentArchivePanel();
	if (!foo)
	{
		Log::info(1, "No active panel.");
		return;
	}
	auto bar = foo->currentEntry();
	if (!bar)
	{
		Log::info(1, "No active entry.");
		return;
	}
	auto meep = CH::getCurrentGfxPanel();
	if (!meep)
	{
		Log::info(1, "No image selected.");
		return;
	}
	if (meep->image())
	{
		meep->image()->imgconv();
		meep->refresh();
		MemChunk mc;
		if (meep->image()->format()->saveImage(*meep->image(), mc))
			bar->importMemChunk(mc);
	}
}
