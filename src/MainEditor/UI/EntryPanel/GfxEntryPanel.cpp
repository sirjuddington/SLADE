
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Misc.h"
#include "General/SAction.h"
#include "Graphics/Graphics.h"
#include "Graphics/Translation.h"
#include "MainEditor/EntryOperations.h"
#include "MainEditor/GfxOffsetsClipboardItem.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Canvas/GfxCanvasBase.h"
#include "UI/Controls/ColourBox.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Controls/ZoomControl.h"
#include "UI/Dialogs/GfxColouriseDialog.h"
#include "UI/Dialogs/GfxConvDialog.h"
#include "UI/Dialogs/GfxCropDialog.h"
#include "UI/Dialogs/GfxTintDialog.h"
#include "UI/Dialogs/ModifyOffsetsDialog.h"
#include "UI/Dialogs/TranslationEditorDialog.h"
#include "UI/Layout.h"
#include "UI/SBrush.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/State.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"
#include "Utility/Colour.h"
#include "Utility/StringUtils.h"


using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, gfx_arc)


// -----------------------------------------------------------------------------
//
// GfxEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxEntryPanel class constructor
// -----------------------------------------------------------------------------
GfxEntryPanel::GfxEntryPanel(wxWindow* parent) :
	EntryPanel(parent, "gfx", true),
	prev_translation_{ new Translation },
	edit_translation_{ new Translation }
{
	auto lh = ui::LayoutHelper(this);

	// Init variables
	prev_translation_->addRange(TransRange::Type::Palette, 0);
	edit_translation_->addRange(TransRange::Type::Palette, 0);

	// Add gfx canvas
	gfx_canvas_ = ui::createGfxCanvas(this);
	sizer_main_->Add(gfx_canvas_->window(), 1, wxEXPAND, 0);

	// Setup gfx canvas
	gfx_canvas_->setViewType(GfxView::Default);
	gfx_canvas_->allowDrag(true);
	gfx_canvas_->allowScroll(true);
	gfx_canvas_->showBorder(true);
	gfx_canvas_->showHilight(true);
	gfx_canvas_->setPalette(maineditor::currentPalette());
	gfx_canvas_->setTranslation(edit_translation_.get());

	// Offsets
	spin_xoffset_ = new wxSpinCtrl(
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
	spin_xoffset_->SetMinSize(lh.spinSize());
	spin_yoffset_->SetMinSize(lh.spinSize());
	sizer_bottom_->Add(new wxStaticText(this, -1, wxS("Offsets:")), wxSizerFlags().CenterVertical());
	sizer_bottom_->Add(spin_xoffset_, lh.sfWithBorder(0, wxLEFT | wxRIGHT).CenterVertical());
	sizer_bottom_->Add(spin_yoffset_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());

	// Gfx (offset) type
	vector<string> offset_types = { "Auto", "Graphic", "Sprite", "HUD" };
	choice_offset_type_         = new wxChoice(
        this, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(offset_types));
	choice_offset_type_->SetSelection(0);
	sizer_bottom_->Add(choice_offset_type_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());

	// Auto offset
	btn_auto_offset_ = new SIconButton(this, "offset", "Modify Offsets...");
	sizer_bottom_->Add(btn_auto_offset_, wxSizerFlags().CenterVertical());

	sizer_bottom_->AddStretchSpacer();

	// Image selection controls
	text_imgnum_   = new wxStaticText(this, -1, wxS("Image: "));
	text_imgoutof_ = new wxStaticText(this, -1, wxS(" out of XX"));
	spin_curimg_   = new wxSpinCtrl(
        this,
        -1,
        wxEmptyString,
        wxDefaultPosition,
        wxDefaultSize,
        wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER | wxSP_WRAP,
        1,
        1,
        0);
	spin_curimg_->SetMinSize(lh.spinSize());
	sizer_bottom_->Add(text_imgnum_, wxSizerFlags().Center());
	sizer_bottom_->Add(spin_curimg_, wxSizerFlags().Center()); // 0, wxSHRINK | wxALIGN_CENTER, ui::pad());
	sizer_bottom_->Add(text_imgoutof_, lh.sfWithLargeBorder(0, wxRIGHT).Center());
	text_imgnum_->Show(false);
	spin_curimg_->Show(false);
	text_imgoutof_->Show(false);

	// Zoom
	zc_zoom_ = new ui::ZoomControl(this, gfx_canvas_);
	sizer_bottom_->Add(zc_zoom_, 0, wxEXPAND);

	// Refresh when main palette changed
	sc_palette_changed_ = theMainWindow->paletteChooser()->signals().palette_changed.connect(
		[this]()
		{
			updateImagePalette();
			gfx_canvas_->window()->Refresh();
		});

	// Custom menu
	menu_custom_ = new wxMenu();
	GfxEntryPanel::fillCustomMenu(menu_custom_);
	custom_menu_name_ = "Graphic";

	// Brushes menu
	menu_brushes_ = new wxMenu();
	fillBrushMenu(menu_brushes_);

	// Custom toolbar
	setupToolbars();

	// Bind Events
	cb_colour_->Bind(wxEVT_COLOURBOX_CHANGED, &GfxEntryPanel::onPaintColourChanged, this);
	spin_xoffset_->Bind(wxEVT_SPINCTRL, &GfxEntryPanel::onXOffsetChanged, this);
	spin_yoffset_->Bind(wxEVT_SPINCTRL, &GfxEntryPanel::onYOffsetChanged, this);
	spin_xoffset_->Bind(wxEVT_TEXT_ENTER, &GfxEntryPanel::onXOffsetChanged, this);
	spin_yoffset_->Bind(wxEVT_TEXT_ENTER, &GfxEntryPanel::onYOffsetChanged, this);
	choice_offset_type_->Bind(wxEVT_CHOICE, &GfxEntryPanel::onOffsetTypeChanged, this);
	Bind(wxEVT_GFXCANVAS_OFFSET_CHANGED, &GfxEntryPanel::onGfxOffsetChanged, this, gfx_canvas_->window()->GetId());
	Bind(wxEVT_GFXCANVAS_PIXELS_CHANGED, &GfxEntryPanel::onGfxPixelsChanged, this, gfx_canvas_->window()->GetId());
	Bind(wxEVT_GFXCANVAS_COLOUR_PICKED, &GfxEntryPanel::onColourPicked, this, gfx_canvas_->window()->GetId());
	spin_curimg_->Bind(wxEVT_SPINCTRL, &GfxEntryPanel::onCurImgChanged, this);
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
		global::error = "no entry to load";
		return false;
	}

	// Update variables
	setModified(false);

	// Attempt to load the image
	if (!misc::loadImageFromEntry(image(), entry, index))
		return false;

	// Only show next/prev image buttons if the entry contains multiple images
	if (image()->size() > 1)
	{
		text_imgnum_->Show();
		spin_curimg_->Show();
		text_imgoutof_->Show();
	}
	else
	{
		text_imgnum_->Show(false);
		spin_curimg_->Show(false);
		text_imgoutof_->Show(false);
	}

	// Hack for colormaps to be 256-wide
	if (strutil::equalCI(entry->type()->name(), "colormap"))
		image()->setWidth(256);

	// Refresh everything
	refresh(entry);

	return true;
}

// -----------------------------------------------------------------------------
// Saves any changes to the entry
// -----------------------------------------------------------------------------
bool GfxEntryPanel::writeEntry(ArchiveEntry& entry)
{
	// Set offsets
	auto* image = this->image();
	image->setXOffset(spin_xoffset_->GetValue());
	image->setYOffset(spin_yoffset_->GetValue());

	// Write new image data if modified
	MemChunk data{ entry.data() };
	bool     ok = true;
	if (image_data_modified_)
	{
		auto* format = image->format();

		string error;
		ok                  = false;
		const auto writable = format ? format->canWrite(*image) : SIFormat::Writable::No;
		if (!format || format == SIFormat::unknownFormat())
			error = "Image is of unknown format";
		else if (writable == SIFormat::Writable::No)
			error = fmt::format("Writing unsupported for format \"{}\"", format->name());
		else
		{
			// Convert image if necessary (using default options)
			if (writable == SIFormat::Writable::Convert)
			{
				format->convertWritable(*image, SIFormat::ConvertOptions());
				log::info("Image converted for writing");
			}

			if (format->saveImage(*image, data, gfx_canvas_->palette()))
				ok = true;
			else
				error = "Error writing image";
		}

		if (ok)
		{
			// Update entry data
			entry.importMemChunk(data);

			// Re-detect type
			auto* oldtype = entry.type();
			EntryType::detectEntryType(entry);

			// Update extension if type changed
			if (oldtype != entry.type())
				entry.setExtensionByType();
		}
		else
			wxMessageBox(wxString::FromUTF8("Cannot save changes to image: " + error), wxS("Error"), wxICON_ERROR);
	}

	// Otherwise just set offsets
	else
	{
		gfx::setImageOffsets(data, spin_xoffset_->GetValue(), spin_yoffset_->GetValue());
		auto* etype = entry.type();
		entry.importMemChunk(data);
		entry.setType(etype, entry.typeReliability());
	}


	// PNG: Apply alPh/tRNS options
	if (entry.type()->formatId() == "img_png")
	{
		auto* etype    = entry.type();
		bool  modified = false;

		// alPh
		const bool alph = gfx::pngGetalPh(entry.data());
		if (alph != menu_custom_->IsChecked(SAction::fromId("pgfx_alph")->wxId()))
		{
			gfx::pngSetalPh(data, !alph);
			modified = true;
		}

		// tRNS
		const bool trns = gfx::pngGettRNS(entry.data());
		if (trns != menu_custom_->IsChecked(SAction::fromId("pgfx_trns")->wxId()))
		{
			gfx::pngSettRNS(data, !trns);
			modified = true;
		}

		if (modified)
		{
			entry.importMemChunk(data);
			entry.setType(etype, entry.typeReliability());
		}
	}

	return ok;
}

// -----------------------------------------------------------------------------
// Adds controls to the entry panel toolbar
// -----------------------------------------------------------------------------
void GfxEntryPanel::setupToolbars()
{
	// --- Top Toolbar ---

	// Brush options
	auto* g_brush = new SToolBarGroup(toolbar_, "Brush", true);
	button_brush_ = g_brush->addActionButton("pgfx_setbrush");
	cb_colour_    = new ColourBox(g_brush, -1, ColRGBA::BLACK, false, true, SToolBar::scaledButtonSize(this));
	cb_colour_->setPalette(gfx_canvas_->palette());
	cb_colour_->SetToolTip(wxS("Set brush colour"));
	g_brush->addCustomControl(cb_colour_);
	g_brush->addActionButton("pgfx_settrans", "");
	toolbar_->addGroup(g_brush);
	g_brush->hide();

	// Image operations
	auto* g_image = new SToolBarGroup(toolbar_, "Image");
	g_image->addActionButton("pgfx_mirror", "");
	g_image->addActionButton("pgfx_flip", "");
	g_image->addActionButton("pgfx_rotate", "");
	g_image->addActionButton("pgfx_crop", "");
	toolbar_->addGroup(g_image);

	// Colour operations
	auto* g_colour = new SToolBarGroup(toolbar_, "Colour");
	g_colour->addActionButton("pgfx_remap", "");
	g_colour->addActionButton("pgfx_colourise", "");
	g_colour->addActionButton("pgfx_tint", "");
	toolbar_->addGroup(g_colour);

	// File operations
	auto* g_file = new SToolBarGroup(toolbar_, "File");
	g_file->addActionButton("pgfx_convert", "");
	g_file->addActionButton("pgfx_pngopt", "")->Enable(false);
	toolbar_->addGroup(g_file);

	// View
	auto* g_view = new SToolBarGroup(toolbar_, "View");
	btn_arc_     = g_view->addActionButton(
        "toggle_arc", "Aspect Ratio Correction", "aspectratio", "Toggle Aspect Ratio Correction");
	btn_arc_->setChecked(gfx_arc);
	btn_tile_ = g_view->addActionButton("toggle_tile", "Tile", "tile", "Toggle tiled view");
	toolbar_->addGroup(g_view, true);



	// --- Left Toolbar ---

	// Tool
	auto* g_tool = new SToolBarGroup(toolbar_left_, "Tool");
	g_tool->addActionButton("tool_drag", "Drag offsets", "gfx_drag", "Drag image to change its offsets")
		->setChecked(true);
	g_tool->addActionButton("tool_draw", "Draw pixels", "gfx_draw", "Draw on the image");
	g_tool->addActionButton("tool_erase", "Erase pixels", "gfx_erase", "Erase pixels from the image");
	g_tool->addActionButton(
		"tool_translate", "Translate pixels", "gfx_translate", "Apply a translation to pixels of the image");
	g_tool->Bind(wxEVT_STOOLBAR_BUTTON_CLICKED, &GfxEntryPanel::onToolSelected, this);
	toolbar_left_->addGroup(g_tool);
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
	bm->AppendSubMenu(pa, wxS("Dither Patterns"));
}

// -----------------------------------------------------------------------------
// Extract all sub-images as individual PNGs
// -----------------------------------------------------------------------------
bool GfxEntryPanel::extractAll() const
{
	const auto entry = entry_.lock();
	if (!entry)
		return false;

	if (image()->size() < 2)
		return false;

	// Remember where we are
	const int imgindex = image()->index();

	auto* parent = entry->parent();
	if (parent == nullptr)
		return false;

	const int index = parent->entryIndex(entry.get(), entry->parentDir());
	auto      name  = entry->nameNoExt();

	// Loop through subimages and get things done
	int pos = 0;
	for (int i = 0; i < image()->size(); ++i)
	{
		auto newname = fmt::format("{}_{}.png", name, i);
		misc::loadImageFromEntry(image(), entry.get(), i);

		// Only process images that actually contain some pixels
		if (image()->width() && image()->height())
		{
			MemChunk img_data;
			SIFormat::getFormat("png")->saveImage(*image(), img_data, gfx_canvas_->palette());

			auto newimg = parent->addNewEntry(newname, index + pos + 1, entry->parentDir());
			if (newimg == nullptr)
				return false;

			newimg->importMemChunk(img_data);

			EntryType::detectEntryType(*newimg);
			pos++;
		}
	}

	// Reload image of where we were
	misc::loadImageFromEntry(image(), entry.get(), imgindex);

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
	theMainWindow->paletteChooser()->setGlobalFromArchive(entry->parent(), misc::detectPaletteHack(entry));
	updateImagePalette();

	// Set offset text boxes
	spin_xoffset_->SetValue(image()->offset().x);
	spin_yoffset_->SetValue(image()->offset().y);

	// Get some needed menu ids
	const int menu_gfxep_pngopt      = SAction::fromId("pgfx_pngopt")->wxId();
	const int menu_gfxep_alph        = SAction::fromId("pgfx_alph")->wxId();
	const int menu_gfxep_trns        = SAction::fromId("pgfx_trns")->wxId();
	const int menu_gfxep_extract     = SAction::fromId("pgfx_extract")->wxId();
	const int menu_gfxep_translate   = SAction::fromId("pgfx_remap")->wxId();
	const int menu_archgfx_exportpng = SAction::fromId("arch_gfx_exportpng")->wxId();
	const int menu_gfxep_offsetpaste = SAction::fromId("pgfx_offsetpaste")->wxId();

	// Set PNG check menus
	if (entry->type() != nullptr && entry->type()->formatId() == "img_png")
	{
		// Check for alph
		alph_ = gfx::pngGetalPh(entry->data());
		menu_custom_->Enable(menu_gfxep_alph, true);
		menu_custom_->Check(menu_gfxep_alph, alph_);

		// Check for trns
		trns_ = gfx::pngGettRNS(entry->data());
		menu_custom_->Enable(menu_gfxep_trns, true);
		menu_custom_->Check(menu_gfxep_trns, trns_);

		// Disable 'Export as PNG' (it already is :P)
		menu_custom_->Enable(menu_archgfx_exportpng, false);

		// Add 'Optimize PNG' option
		menu_custom_->Enable(menu_gfxep_pngopt, true);
		toolbar_->findActionButton("pgfx_pngopt")->Enable(true);
	}
	else
	{
		menu_custom_->Enable(menu_gfxep_alph, false);
		menu_custom_->Enable(menu_gfxep_trns, false);
		menu_custom_->Check(menu_gfxep_alph, false);
		menu_custom_->Check(menu_gfxep_trns, false);
		menu_custom_->Enable(menu_gfxep_pngopt, false);
		menu_custom_->Enable(menu_archgfx_exportpng, true);
		toolbar_->findActionButton("pgfx_pngopt")->Enable(false);
	}

	// Set multi-image format stuff thingies
	cur_index_ = image()->index();
	if (image()->size() > 1)
		menu_custom_->Enable(menu_gfxep_extract, true);
	else
		menu_custom_->Enable(menu_gfxep_extract, false);
	text_imgoutof_->SetLabel(WX_FMT(" out of {}", image()->size()));
	spin_curimg_->SetValue(cur_index_ + 1);
	spin_curimg_->SetRange(1, image()->size());

	// Disable paste offsets if nothing in clipboard
	menu_custom_->Enable(
		menu_gfxep_offsetpaste, app::clipboard().firstItem(ClipboardItem::Type::GfxOffsets) != nullptr);

	// Update status bar in case image dimensions changed
	updateStatus();

	// Apply offset view type
	applyViewType(entry);

	// Reset display offsets in graphics mode
	if (gfx_canvas_->viewType() != GfxView::Sprite)
		gfx_canvas_->resetViewOffsets();

	// Setup custom menu
	if (image()->type() == SImage::Type::RGBA)
		menu_custom_->Enable(menu_gfxep_translate, false);
	else
		menu_custom_->Enable(menu_gfxep_translate, true);

	// Refresh the canvas
	gfx_canvas_->window()->Refresh();
}

// -----------------------------------------------------------------------------
// Returns a string with extended editing/entry info for the status bar
// -----------------------------------------------------------------------------
string GfxEntryPanel::statusString()
{
	// Setup status string
	auto* image  = this->image();
	auto  status = fmt::format("{}x{}", image->width(), image->height());

	// Colour format
	if (image->type() == SImage::Type::RGBA)
		status += ", 32bpp";
	else
		status += ", 8bpp";

	// PNG stuff
	if (auto entry = entry_.lock(); entry->type()->formatId() == "img_png")
	{
		// alPh
		if (gfx::pngGetalPh(entry->data()))
			status += ", alPh";

		// tRNS
		if (gfx::pngGettRNS(entry->data()))
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
	gfx_canvas_->setPalette(maineditor::currentPalette());
}

// -----------------------------------------------------------------------------
// Detects the offset view type of the current entry
// -----------------------------------------------------------------------------
GfxView GfxEntryPanel::detectOffsetType(ArchiveEntry* entry) const
{
	if (!entry)
		return GfxView::Default;

	if (!entry->parent())
		return GfxView::Default;

	// Check what section of the archive the entry is in -- only PNGs or images
	// in the sprites section can be HUD or sprite
	const bool is_sprite = ("sprites" == entry->parent()->detectNamespace(entry));
	const bool is_png    = ("img_png" == entry->type()->formatId());
	if (!is_sprite && !is_png)
		return GfxView::Default;

	auto* img = image();
	if (is_png && img->offset().x == 0 && img->offset().y == 0)
		return GfxView::Default;

	const int width        = img->width();
	const int height       = img->height();
	const int left         = -img->offset().x;
	const int right        = left + width;
	const int top          = -img->offset().y;
	const int bottom       = top + height;
	const int horiz_center = (left + right) / 2;

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
	const int bottom_quartile = (bottom * 3 + top) / 4;
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
		return GfxView::HUD;
	else
		return GfxView::Sprite;
}

// -----------------------------------------------------------------------------
// Sets the view type of the gfx canvas depending on what is selected in the
// offset type combo box
// -----------------------------------------------------------------------------
void GfxEntryPanel::applyViewType(ArchiveEntry* entry) const
{
	// Tile checkbox overrides offset type selection
	if (btn_tile_->isChecked())
		gfx_canvas_->setViewType(GfxView::Tiled);
	else
	{
		// Set gfx canvas view type depending on the offset combobox selection
		const int sel = choice_offset_type_->GetSelection();
		switch (sel)
		{
		case 0:  gfx_canvas_->setViewType(detectOffsetType(entry)); break;
		case 1:  gfx_canvas_->setViewType(GfxView::Default); break;
		case 2:  gfx_canvas_->setViewType(GfxView::Sprite); break;
		case 3:  gfx_canvas_->setViewType(GfxView::HUD); break;
		default: break;
		}
	}

	// Refresh
	gfx_canvas_->window()->Refresh();
}

// ----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// ----------------------------------------------------------------------------
bool GfxEntryPanel::handleEntryPanelAction(string_view id)
{
	// We're only interested in "pgfx_" actions
	if (!strutil::startsWith(id, "pgfx_"))
		return false;

	const auto entry = entry_.lock();

	// For pgfx_brush actions, the string after pgfx is a brush name
	if (strutil::startsWith(id, "pgfx_brush"))
	{
		gfx_canvas_->setBrush(SBrush::get(string{ id }));
		button_brush_->setIcon(strutil::afterFirst(id, '_'));
	}

	// Editing - set translation
	else if (id == "pgfx_settrans")
	{
		// Create translation editor dialog
		TranslationEditorDialog ted(
			theMainWindow, *theMainWindow->paletteChooser()->selectedPalette(), " Colour Remap", image());

		// Create translation to edit
		ted.openTranslation(*edit_translation_);

		// Show the dialog
		if (ted.ShowModal() == wxID_OK)
		{
			// Set the translation
			edit_translation_->copy(ted.getTranslation());
			gfx_canvas_->setTranslation(edit_translation_.get());
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
		gfx_canvas_->window()->Refresh();

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
		gfx_canvas_->window()->Refresh();

		// Update variables
		image_data_modified_ = true;
		setModified();
	}

	// Rotate
	else if (id == "pgfx_rotate")
	{
		// Prompt for rotation angle
		vector<string> angles = { "90", "180", "270" };
		const int      choice = wxGetSingleChoiceIndex(
            wxS("Select rotation angle"), wxS("Rotate"), wxutil::arrayStringStd(angles), 0);

		// Rotate image
		switch (choice)
		{
		case 0:  image()->rotate(90); break;
		case 1:  image()->rotate(180); break;
		case 2:  image()->rotate(270); break;
		default: break;
		}

		// Update UI
		gfx_canvas_->window()->Refresh();

		// Update variables
		image_data_modified_ = true;
		setModified();
	}

	// Translate
	else if (id == "pgfx_remap")
	{
		// Create translation editor dialog
		auto*                   pal = maineditor::currentPalette();
		TranslationEditorDialog ted(theMainWindow, *pal, " Colour Remap", &gfx_canvas_->image());

		// Create translation to edit
		ted.openTranslation(*prev_translation_);

		// Show the dialog
		if (ted.ShowModal() == wxID_OK)
		{
			// Apply translation to image
			image()->applyTranslation(&ted.getTranslation(), pal);

			// Update UI
			gfx_canvas_->window()->Refresh();

			// Update variables
			image_data_modified_ = true;
			setModified();
			prev_translation_->copy(ted.getTranslation());
		}
	}

	// Colourise
	else if (id == "pgfx_colourise" && entry)
	{
		auto*              pal = maineditor::currentPalette();
		GfxColouriseDialog gcd(theMainWindow, entry.get(), *pal);
		gcd.setColour(ui::getStateString(ui::COLOURISEDIALOG_LAST_COLOUR));

		// Show colourise dialog
		if (gcd.ShowModal() == wxID_OK)
		{
			// Colourise image
			image()->colourise(gcd.colour(), pal);

			// Update UI
			gfx_canvas_->window()->Refresh();

			// Update variables
			image_data_modified_ = true;
			Refresh();
			setModified();
		}
		ui::saveStateString(ui::COLOURISEDIALOG_LAST_COLOUR, colour::toString(gcd.colour(), colour::StringFormat::RGB));
	}

	// Tint
	else if (id == "pgfx_tint" && entry)
	{
		auto*         pal = maineditor::currentPalette();
		GfxTintDialog gtd(theMainWindow, entry.get(), *pal);
		gtd.setValues(ui::getStateString(ui::TINTDIALOG_LAST_COLOUR), ui::getStateInt(ui::TINTDIALOG_LAST_AMOUNT));

		// Show tint dialog
		if (gtd.ShowModal() == wxID_OK)
		{
			// Tint image
			image()->tint(gtd.colour(), gtd.amount(), pal);

			// Update UI
			gfx_canvas_->window()->Refresh();

			// Update variables
			image_data_modified_ = true;
			Refresh();
			setModified();
		}
		ui::saveStateString(ui::TINTDIALOG_LAST_COLOUR, colour::toString(gtd.colour(), colour::StringFormat::RGB));
		ui::saveStateInt(ui::TINTDIALOG_LAST_AMOUNT, static_cast<int>(gtd.amount() * 100.0f));
	}

	// Crop
	else if (id == "pgfx_crop")
	{
		auto*         image = this->image();
		auto*         pal   = maineditor::currentPalette();
		GfxCropDialog gcd(theMainWindow, *image, pal);

		// Show crop dialog
		if (gcd.ShowModal() == wxID_OK)
		{
			// Prompt to adjust offsets
			const auto crop = gcd.cropRect();
			if (crop.tl.x > 0 || crop.tl.y > 0)
			{
				if (wxMessageBox(
						wxS("Do you want to adjust the offsets? This will keep the graphic in the same relative "
							"position it was before cropping."),
						wxS("Adjust Offsets?"),
						wxYES_NO)
					== wxYES)
				{
					spin_xoffset_->SetValue(image->offset().x - crop.tl.x);
					spin_yoffset_->SetValue(image->offset().y - crop.tl.y);
					image->setXOffset(image->offset().x - crop.tl.x);
					image->setYOffset(image->offset().y - crop.tl.y);
				}
			}

			// Crop image
			image->crop(crop.x1(), crop.y1(), crop.x2(), crop.y2());

			// Update UI
			gfx_canvas_->window()->Refresh();

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
		if (entryoperations::optimizePNG(entry.get()))
			setModified(false);
		else
			wxMessageBox(
				wxS("Warning: Couldn't optimize this image, check console log for info"),
				wxS("Warning"),
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
			auto* image  = gcd.itemImage(0);
			auto* format = gcd.itemFormat(0);

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
			const int MENU_GFXEP_PNGOPT      = SAction::fromId("pgfx_pngopt")->wxId();
			const int MENU_GFXEP_ALPH        = SAction::fromId("pgfx_alph")->wxId();
			const int MENU_GFXEP_TRNS        = SAction::fromId("pgfx_trns")->wxId();
			const int MENU_ARCHGFX_EXPORTPNG = SAction::fromId("arch_gfx_exportpng")->wxId();
			if (format->name() == "PNG")
			{
				menu_custom_->Enable(MENU_GFXEP_ALPH, true);
				menu_custom_->Enable(MENU_GFXEP_TRNS, true);
				menu_custom_->Check(MENU_GFXEP_TRNS, gfx::pngGettRNS(entry_data_));
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
			gfx_canvas_->window()->Refresh();
		}
	}

	// Copy offsets
	else if (id == "pgfx_offsetcopy")
		entryoperations::copyGfxOffsets(*entry_.lock());

	// Paste offsets
	else if (id == "pgfx_offsetpaste")
	{
		// Check for existing offsets in clipboard
		if (auto item = app::clipboard().firstItem(ClipboardItem::Type::GfxOffsets))
		{
			auto offset_item = dynamic_cast<GfxOffsetsClipboardItem*>(item);
			spin_xoffset_->SetValue(offset_item->offsets().x);
			spin_yoffset_->SetValue(offset_item->offsets().y);
			image()->setOffsets(offset_item->offsets());
			setModified();
			gfx_canvas_->window()->Refresh();
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
	SAction::fromId("pgfx_offsetcopy")->addToMenu(custom);
	SAction::fromId("pgfx_offsetpaste")->addToMenu(custom);
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
// Returns the canvas image
// -----------------------------------------------------------------------------
SImage* GfxEntryPanel::image() const
{
	if (gfx_canvas_)
		return &gfx_canvas_->image();
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void GfxEntryPanel::toolbarButtonClick(const string& action_id)
{
	if (action_id == "toggle_arc")
	{
		btn_arc_->setChecked(!btn_arc_->isChecked());
		gfx_arc = btn_arc_->isChecked();
		gfx_canvas_->window()->Refresh();
	}

	else if (action_id == "toggle_tile")
	{
		btn_tile_->setChecked(!btn_tile_->isChecked());
		choice_offset_type_->Enable(!btn_tile_->isChecked());
		applyViewType(entry_.lock().get());
	}
}


// -----------------------------------------------------------------------------
//
// GfxEntryPanel Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

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
	const int offset = spin_xoffset_->GetValue();
	if (offset == image()->offset().x)
		return;

	// Update offset & refresh
	image()->setXOffset(offset);
	setModified();
	gfx_canvas_->window()->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the Y offset value is modified
// -----------------------------------------------------------------------------
void GfxEntryPanel::onYOffsetChanged(wxCommandEvent& e)
{
	// Ignore if the value wasn't changed
	const int offset = spin_yoffset_->GetValue();
	if (offset == image()->offset().y)
		return;

	// Update offset & refresh
	image()->setYOffset(offset);
	setModified();
	gfx_canvas_->window()->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the 'type' combo box selection is changed
// -----------------------------------------------------------------------------
void GfxEntryPanel::onOffsetTypeChanged(wxCommandEvent& e)
{
	applyViewType(entry_.lock().get());
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
// Called when the 'current image' spinbox is changed
// -----------------------------------------------------------------------------
void GfxEntryPanel::onCurImgChanged(wxCommandEvent& e)
{
	const int num      = gfx_canvas_->image().size();
	auto*     entry    = entry_.lock().get();
	const int newindex = spin_curimg_->GetValue() - 1;
	if (num > 1 && entry && newindex != cur_index_)
	{
		loadEntry(entry, newindex);
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
		const Vec2i offsets = dlg.calculateOffsets(
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
// Called when a button is clicked on the tools toolbar group
// -----------------------------------------------------------------------------
void GfxEntryPanel::onToolSelected(wxCommandEvent& e)
{
	const auto id = e.GetString();

	toolbar_left_->group("Tool")->setAllButtonsChecked(false);

	// Editing - drag mode
	if (id == wxS("tool_drag"))
	{
		editing_ = false;
		gfx_canvas_->setEditingMode(GfxEditMode::None);
		toolbar_->group("Brush")->hide();
		toolbar_left_->findActionButton("tool_drag")->setChecked(true);
		toolbar_->updateLayout();
	}

	// Editing - draw mode
	else if (id == wxS("tool_draw"))
	{
		editing_ = true;
		toolbar_->group("Brush")->hide(false);
		toolbar_left_->findActionButton("tool_draw")->setChecked(true);
		gfx_canvas_->setEditingMode(GfxEditMode::Paint);
		gfx_canvas_->setPaintColour(cb_colour_->colour());
		toolbar_->updateLayout();
	}

	// Editing - erase mode
	else if (id == wxS("tool_erase"))
	{
		editing_ = true;
		toolbar_->group("Brush")->hide(false);
		toolbar_left_->findActionButton("tool_erase")->setChecked(true);
		gfx_canvas_->setEditingMode(GfxEditMode::Erase);
		toolbar_->updateLayout();
	}

	// Editing - translate mode
	else if (id == wxS("tool_translate"))
	{
		editing_ = true;
		toolbar_->group("Brush")->hide(false);
		toolbar_left_->findActionButton("tool_translate")->setChecked(true);
		gfx_canvas_->setEditingMode(GfxEditMode::Translate);
		toolbar_->updateLayout();
	}
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

// I'd love to put them in their own file, but attempting to do so
// results in a circular include nightmare and nothing works anymore.
#include "General/Console.h"
#include "MainEditor/UI/ArchivePanel.h"

namespace
{
GfxEntryPanel* getCurrentGfxPanel()
{
	auto* panel = maineditor::currentEntryPanel();
	if (panel)
	{
		if (strutil::equalCI(panel->name(), "gfx"))
		{
			return dynamic_cast<GfxEntryPanel*>(panel);
		}
	}
	return nullptr;
}
} // namespace

CONSOLE_COMMAND(rotate, 1, true)
{
	double         val;
	const wxString bluh = wxString::FromUTF8(args[0]);
	if (!bluh.ToDouble(&val))
	{
		if (!bluh.CmpNoCase(wxS("l")) || !bluh.CmpNoCase(wxS("left")))
			val = 90.;
		else if (!bluh.CmpNoCase(wxS("f")) || !bluh.CmpNoCase(wxS("flip")))
			val = 180.;
		else if (!bluh.CmpNoCase(wxS("r")) || !bluh.CmpNoCase(wxS("right")))
			val = 270.;
		else
		{
			log::error("Invalid parameter: {} is not a number.", bluh.utf8_string());
			return;
		}
	}
	const int angle = static_cast<int>(val);
	if (angle % 90)
	{
		log::error("Invalid parameter: {} is not a multiple of 90.", angle);
		return;
	}

	auto* foo = maineditor::currentArchivePanel();
	if (!foo)
	{
		log::info(1, "No active panel.");
		return;
	}
	auto* bar = foo->currentEntry();
	if (!bar)
	{
		log::info(1, "No active entry.");
		return;
	}
	auto* meep = getCurrentGfxPanel();
	if (!meep)
	{
		log::info(1, "No image selected.");
		return;
	}

	// Get current entry
	auto* entry = maineditor::currentEntry();

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
	bool       vertical;
	const auto bluh = args[0];
	if (strutil::equalCI(bluh, "y") || strutil::equalCI(bluh, "v") || strutil::equalCI(bluh, "vert")
		|| strutil::equalCI(bluh, "vertical"))
		vertical = true;
	else if (
		strutil::equalCI(bluh, "x") || strutil::equalCI(bluh, "h") || strutil::equalCI(bluh, "horz")
		|| strutil::equalCI(bluh, "horizontal"))
		vertical = false;
	else
	{
		log::error("Invalid parameter: {}{ is not a known value.", bluh);
		return;
	}
	auto* foo = maineditor::currentArchivePanel();
	if (!foo)
	{
		log::info(1, "No active panel.");
		return;
	}
	auto* bar = foo->currentEntry();
	if (!bar)
	{
		log::info(1, "No active entry.");
		return;
	}
	auto* meep = getCurrentGfxPanel();
	if (!meep)
	{
		log::info(1, "No image selected.");
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
	if (strutil::toInt(args[0], x1) && strutil::toInt(args[1], y1) && strutil::toInt(args[2], x2)
		&& strutil::toInt(args[3], y2))
	{
		auto* foo = maineditor::currentArchivePanel();
		if (!foo)
		{
			log::info(1, "No active panel.");
			return;
		}
		auto* meep = getCurrentGfxPanel();
		if (!meep)
		{
			log::info(1, "No image selected.");
			return;
		}
		auto* bar = foo->currentEntry();
		if (!bar)
		{
			log::info(1, "No active entry.");
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
	auto* foo = maineditor::currentArchivePanel();
	if (!foo)
	{
		log::info(1, "No active panel.");
		return;
	}
	auto* meep = getCurrentGfxPanel();
	if (!meep)
	{
		log::info(1, "No image selected.");
		return;
	}
	auto* bar = foo->currentEntry();
	if (!bar)
	{
		log::info(1, "No active entry.");
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
	auto* foo = maineditor::currentArchivePanel();
	if (!foo)
	{
		log::info(1, "No active panel.");
		return;
	}
	auto* meep = getCurrentGfxPanel();
	if (!meep)
	{
		log::info(1, "No image selected.");
		return;
	}
	auto* bar = foo->currentEntry();
	if (!bar)
	{
		log::info(1, "No active entry.");
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
	auto* foo = maineditor::currentArchivePanel();
	if (!foo)
	{
		log::info(1, "No active panel.");
		return;
	}
	auto* bar = foo->currentEntry();
	if (!bar)
	{
		log::info(1, "No active entry.");
		return;
	}
	auto* meep = getCurrentGfxPanel();
	if (!meep)
	{
		log::info(1, "No image selected.");
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
