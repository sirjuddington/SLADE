
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PaletteEntryPanel.cpp
// Description: PaletteEntryPanel class. The UI for editing palette (PLAYPAL)
//              entries
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
#include "PaletteEntryPanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "Graphics/Palette/PaletteManager.h"
#include "Graphics/SImage/SIFormat.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/Canvas/PaletteCanvas.h"
#include "UI/Controls/PaletteChooser.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Float, col_greyscale_r, 0.299, CVar::Flag::Save)
CVAR(Float, col_greyscale_g, 0.587, CVar::Flag::Save)
CVAR(Float, col_greyscale_b, 0.114, CVar::Flag::Save)

namespace
{
wxString extensions =
	"Raw Palette (*.pal)|*.pal|"
	"PNG File (*.png)|*.png|"
	"CSV Palette (*.csv)|*.csv|"
	"JASC Palette (*.pal)|*.pal|"
	"GIMP Palette (*.gpl)|*.gpl";

vector<Palette::Format> pal_formats = { Palette::Format::Raw,
										Palette::Format::Image,
										Palette::Format::CSV,
										Palette::Format::JASC,
										Palette::Format::GIMP };
} // namespace


// -----------------------------------------------------------------------------
// PaletteColouriseDialog Class
//
// A simple dialog for the 'Colourise' function, allows the user to select a
// colour and shows a preview of the colourised palette
// -----------------------------------------------------------------------------
class PaletteColouriseDialog : public wxDialog
{
public:
	PaletteColouriseDialog(wxWindow* parent, Palette* pal) :
		wxDialog(parent, -1, "Colourise", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
		palette_{ pal }
	{
		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::General, "palette_colourise"));
		SetIcon(icon);

		// Setup main sizer
		auto msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		auto sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

		// Add colour chooser
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());

		cp_colour_ = new wxColourPickerCtrl(this, -1, wxColour(255, 0, 0));
		hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
		hbox->Add(cp_colour_, 0, wxEXPAND);

		// Add preview
		pal_preview_ = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview_, 1, wxEXPAND | wxBOTTOM, UI::pad());

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND);

		// Setup preview
		pal_preview_->setSelectionType(PaletteCanvas::SelectionType::Range);
		pal_preview_->SetInitialSize(wxSize(UI::scalePx(384), UI::scalePx(384)));
		redraw();

		// Init layout
		wxWindowBase::Layout();

		// Bind events
		cp_colour_->Bind(wxEVT_COLOURPICKER_CHANGED, [&](wxColourPickerEvent&) { redraw(); });
		pal_preview_->Bind(wxEVT_LEFT_UP, [&](wxMouseEvent&) { redraw(); });

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		wxTopLevelWindowBase::SetMinSize(GetSize());
		CenterOnParent();
	}

	Palette* finalPalette() const { return &(pal_preview_->palette()); }

	ColRGBA colour() const
	{
		wxColour col = cp_colour_->GetColour();
		return { COLWX(col) };
	}

	// Re-apply the changes in selection and colour on a fresh palette
	void redraw() const
	{
		pal_preview_->setPalette(palette_);
		pal_preview_->palette().colourise(colour(), pal_preview_->selectionStart(), pal_preview_->selectionEnd());
		pal_preview_->draw();
	}

private:
	PaletteCanvas*      pal_preview_ = nullptr;
	Palette*            palette_     = nullptr;
	wxColourPickerCtrl* cp_colour_   = nullptr;
};


// -----------------------------------------------------------------------------
// PaletteTintDialog Class
//
// A simple dialog for the 'Tint' function, allows the user to select tint
// colour+amount and shows a preview of the tinted palette
// -----------------------------------------------------------------------------
class PaletteTintDialog : public wxDialog
{
public:
	PaletteTintDialog(wxWindow* parent, Palette* pal) :
		wxDialog(parent, -1, "Tint", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
		palette_{ pal }
	{
		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::General, "palette_tint"));
		SetIcon(icon);

		// Setup main sizer
		auto msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		auto sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

		// Add colour chooser
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());

		cp_colour_ = new wxColourPickerCtrl(this, -1, wxColour(255, 0, 0));
		hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
		hbox->Add(cp_colour_, 0, wxALIGN_CENTER_VERTICAL);

		// Add 'amount' slider
		hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());

		slider_amount_ = new wxSlider(this, -1, 50, 0, 100);
		label_amount_  = new wxStaticText(this, -1, "100%");
		hbox->Add(new wxStaticText(this, -1, "Amount:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
		hbox->Add(slider_amount_, 1, wxEXPAND | wxRIGHT, UI::pad());
		hbox->Add(label_amount_, 0, wxALIGN_CENTER_VERTICAL);

		// Add preview
		pal_preview_ = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview_, 1, wxEXPAND | wxBOTTOM, UI::pad());

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND);

		// Setup preview
		pal_preview_->setSelectionType(PaletteCanvas::SelectionType::Range);
		pal_preview_->SetInitialSize(wxSize(UI::scalePx(384), UI::scalePx(384)));
		redraw();

		// Init layout
		wxWindowBase::Layout();

		// Bind events
		cp_colour_->Bind(wxEVT_COLOURPICKER_CHANGED, [&](wxColourPickerEvent&) { redraw(); });
		slider_amount_->Bind(wxEVT_SLIDER, [&](wxCommandEvent&) {
			redraw();
			label_amount_->SetLabel(wxString::Format("%d%% ", slider_amount_->GetValue()));
		});
		pal_preview_->Bind(wxEVT_LEFT_UP, [&](wxMouseEvent&) { redraw(); });

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		wxTopLevelWindowBase::SetMinSize(GetSize());
		CenterOnParent();

		// Set values
		label_amount_->SetLabel("50% ");
	}

	Palette* finalPalette() const { return &(pal_preview_->palette()); }

	ColRGBA colour() const
	{
		wxColour col = cp_colour_->GetColour();
		return { COLWX(col) };
	}

	float amount() const { return (float)slider_amount_->GetValue() * 0.01f; }

	// Re-apply the changes in selection, colour and amount on a fresh palette
	void redraw() const
	{
		pal_preview_->setPalette(palette_);
		pal_preview_->palette().tint(colour(), amount(), pal_preview_->selectionStart(), pal_preview_->selectionEnd());
		pal_preview_->draw();
	}

private:
	PaletteCanvas*      pal_preview_   = nullptr;
	Palette*            palette_       = nullptr;
	wxColourPickerCtrl* cp_colour_     = nullptr;
	wxSlider*           slider_amount_ = nullptr;
	wxStaticText*       label_amount_  = nullptr;
};


// -----------------------------------------------------------------------------
// PaletteColourTweakDialog Class
//
// A simple dialog for the 'Tweak Colours' function, allows the user to select
// hue, saturation and luminosity changes and shows a preview of the modified
// palette.
//
// TODO: More features? Maybe merge Tint, Invert and Colourise with it, add an
// "Apply Change" button so that it isn't needed anymore to click "OK" and
// close it after each change, etc.
// -----------------------------------------------------------------------------
class PaletteColourTweakDialog : public wxDialog
{
public:
	PaletteColourTweakDialog(wxWindow* parent, Palette* pal) :
		wxDialog(
			parent,
			-1,
			"Tweak Colours",
			wxDefaultPosition,
			wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
		palette_{ pal }
	{
		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::General, "palette_tweak"));
		SetIcon(icon);

		// Setup main sizer
		auto msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		auto sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

		// Add 'hue shift' slider
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());

		slider_hue_ = new wxSlider(this, -1, 0, 0, 500);
		label_hue_  = new wxStaticText(this, -1, "0.000");
		hbox->Add(new wxStaticText(this, -1, "Hue Shift:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
		hbox->Add(slider_hue_, 1, wxEXPAND | wxRIGHT, UI::pad());
		hbox->Add(label_hue_, 0, wxALIGN_CENTER_VERTICAL);

		// Add 'Saturation' slider
		hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());

		slider_sat_ = new wxSlider(this, -1, 100, 0, 200);
		label_sat_  = new wxStaticText(this, -1, "100%");
		hbox->Add(new wxStaticText(this, -1, "Saturation:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
		hbox->Add(slider_sat_, 1, wxEXPAND | wxRIGHT, UI::pad());
		hbox->Add(label_sat_, 0, wxALIGN_CENTER_VERTICAL);

		// Add 'Luminosity' slider
		hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());

		slider_lum_ = new wxSlider(this, -1, 100, 0, 200);
		label_lum_  = new wxStaticText(this, -1, "100%");
		hbox->Add(new wxStaticText(this, -1, "Luminosity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
		hbox->Add(slider_lum_, 1, wxEXPAND | wxRIGHT, UI::pad());
		hbox->Add(label_lum_, 0, wxALIGN_CENTER_VERTICAL);

		// Add preview
		pal_preview_ = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview_, 1, wxEXPAND | wxBOTTOM, UI::pad());

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND);

		// Setup preview
		pal_preview_->setSelectionType(PaletteCanvas::SelectionType::Range);
		pal_preview_->SetInitialSize(wxSize(UI::scalePx(384), UI::scalePx(384)));
		redraw();

		// Init layout
		wxWindowBase::Layout();

		// Bind events
		slider_hue_->Bind(wxEVT_SLIDER, [&](wxCommandEvent&) {
			redraw();
			label_hue_->SetLabel(wxString::Format("%1.3f", hue()));
		});
		slider_sat_->Bind(wxEVT_SLIDER, [&](wxCommandEvent&) {
			redraw();
			label_sat_->SetLabel(wxString::Format("%d%%", slider_sat_->GetValue()));
		});
		slider_lum_->Bind(wxEVT_SLIDER, [&](wxCommandEvent&) {
			redraw();
			label_lum_->SetLabel(wxString::Format("%d%%", slider_lum_->GetValue()));
		});
		pal_preview_->Bind(wxEVT_LEFT_UP, [&](wxMouseEvent&) { redraw(); });

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		wxTopLevelWindowBase::SetMinSize(GetSize());
		CenterOnParent();

		// Set values
		label_hue_->SetLabel("0.000 ");
		label_sat_->SetLabel("100% ");
		label_lum_->SetLabel("100% ");
	}

	Palette* finalPalette() const { return &(pal_preview_->palette()); }

	float hue() const { return (float)slider_hue_->GetValue() * 0.002f; }
	float sat() const { return (float)slider_sat_->GetValue() * 0.01f; }
	float lum() const { return (float)slider_lum_->GetValue() * 0.01f; }

	// Re-apply the changes in selection, hue, saturation and luminosity on a fresh palette
	void redraw() const
	{
		pal_preview_->setPalette(palette_);
		pal_preview_->palette().shift(hue(), pal_preview_->selectionStart(), pal_preview_->selectionEnd());
		pal_preview_->palette().saturate(sat(), pal_preview_->selectionStart(), pal_preview_->selectionEnd());
		pal_preview_->palette().illuminate(lum(), pal_preview_->selectionStart(), pal_preview_->selectionEnd());
		pal_preview_->draw();
	}

private:
	PaletteCanvas* pal_preview_ = nullptr;
	Palette*       palette_     = nullptr;
	wxSlider*      slider_hue_  = nullptr;
	wxSlider*      slider_sat_  = nullptr;
	wxSlider*      slider_lum_  = nullptr;
	wxStaticText*  label_hue_   = nullptr;
	wxStaticText*  label_sat_   = nullptr;
	wxStaticText*  label_lum_   = nullptr;
};


// -----------------------------------------------------------------------------
// PaletteInvertDialog Class
//
// A simple dialog for the 'Invert' function, allows the user to invert the
// colours and shows a preview of the inverted palette
// -----------------------------------------------------------------------------
class PaletteInvertDialog : public wxDialog
{
public:
	PaletteInvertDialog(wxWindow* parent, Palette* pal) :
		wxDialog(parent, -1, "Invert", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
		palette_{ pal }
	{
		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::General, "palette_invert"));
		SetIcon(icon);

		// Setup main sizer
		auto msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		auto sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

		// Add preview
		pal_preview_ = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview_, 1, wxEXPAND | wxBOTTOM, UI::pad());

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND);

		// Setup preview
		pal_preview_->setSelectionType(PaletteCanvas::SelectionType::Range);
		pal_preview_->SetInitialSize(wxSize(UI::scalePx(384), UI::scalePx(384)));
		redraw();

		// Init layout
		wxWindowBase::Layout();

		// Bind events
		pal_preview_->Bind(wxEVT_LEFT_UP, [&](wxMouseEvent&) { redraw(); });

		// Setup dialog size
		SetInitialSize({ -1, -1 });
		wxTopLevelWindowBase::SetMinSize(GetSize());
		CenterOnParent();
	}

	Palette* finalPalette() const { return &(pal_preview_->palette()); }

	// Re-apply the changes in selection on a fresh palette
	void redraw() const
	{
		pal_preview_->setPalette(palette_);
		pal_preview_->palette().invert(pal_preview_->selectionStart(), pal_preview_->selectionEnd());
		pal_preview_->draw();
	}

private:
	PaletteCanvas* pal_preview_ = nullptr;
	Palette*       palette_     = nullptr;
};


// -----------------------------------------------------------------------------
// GeneratePalettesDialog Class
//
// A simple dialog for the 'Generate Palettes' function, allows to choose
// between generating the 14 palettes appropriate for Doom, Heretic and Strife,
// or the 28 palettes appropriate for Hexen.
// -----------------------------------------------------------------------------
class GeneratePalettesDialog : public wxDialog
{
public:
	GeneratePalettesDialog(wxWindow* parent) :
		wxDialog(
			parent,
			-1,
			"Generate Palettes",
			wxDefaultPosition,
			wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	{
		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::Entry, "palette"));
		SetIcon(icon);

		// Setup main sizer
		auto msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		auto sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

		// Add buttons
		rb_doom_ = new wxRadioButton(this, -1, "Doom (14 Palettes)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		sizer->Add(rb_doom_, 0, wxEXPAND | wxBOTTOM, UI::pad());
		rb_hexen_ = new wxRadioButton(this, -1, "Hexen (28 Palettes)");
		sizer->Add(rb_hexen_, 0, wxEXPAND);

		sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND);

		// Init layout
		wxWindowBase::Layout();

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		wxTopLevelWindowBase::SetMinSize(GetSize());
		CenterOnParent();
	}

	int choice() const
	{
		if (rb_doom_->GetValue())
			return 1;
		if (rb_hexen_->GetValue())
			return 2;

		return 0;
	}

private:
	wxRadioButton* rb_doom_  = nullptr;
	wxRadioButton* rb_hexen_ = nullptr;
};


// -----------------------------------------------------------------------------
// PaletteGradientDialog Class
//
// A dialog for the 'Gradient' function, allows the user to create a gradient
// between two colours, and apply it to a range of indexes in the palette.
// -----------------------------------------------------------------------------
class PaletteGradientDialog : public wxDialog
{
public:
	PaletteGradientDialog(wxWindow* parent, Palette* pal) :
		wxDialog(parent, -1, "Gradient", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
		palette_{ pal }
	{
		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::General, "palette_gradient"));
		SetIcon(icon);

		// Setup main sizer
		auto msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		auto sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

		// Add colour choosers
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());

		cp_startcolour_ = new wxColourPickerCtrl(this, -1, wxColour(0, 0, 0));
		hbox->Add(new wxStaticText(this, -1, "Start Colour:"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
		hbox->Add(cp_startcolour_, 0, wxEXPAND);

		cp_endcolour_ = new wxColourPickerCtrl(this, -1, wxColour(255, 255, 255));
		hbox->Add(new wxStaticText(this, -1, "End Colour:"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
		hbox->Add(cp_endcolour_, 0, wxEXPAND);

		// Add preview
		pal_preview_ = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview_, 1, wxEXPAND | wxBOTTOM, UI::pad());

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND);

		// Setup preview
		pal_preview_->setSelectionType(PaletteCanvas::SelectionType::Range);
		pal_preview_->SetInitialSize(wxSize(UI::scalePx(384), UI::scalePx(384)));
		redraw();

		// Init layout
		wxWindowBase::Layout();

		// Bind events
		cp_startcolour_->Bind(wxEVT_COLOURPICKER_CHANGED, [&](wxColourPickerEvent&) { redraw(); });
		cp_endcolour_->Bind(wxEVT_COLOURPICKER_CHANGED, [&](wxColourPickerEvent&) { redraw(); });
		pal_preview_->Bind(wxEVT_LEFT_UP, [&](wxMouseEvent&) { redraw(); });

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		wxTopLevelWindowBase::SetMinSize(GetSize());
		CenterOnParent();
	}

	Palette* finalPalette() const { return &(pal_preview_->palette()); }

	ColRGBA startColour() const
	{
		wxColour col = cp_startcolour_->GetColour();
		return { COLWX(col) };
	}

	ColRGBA endColour() const
	{
		wxColour col = cp_endcolour_->GetColour();
		return { COLWX(col) };
	}

	// Re-apply the changes in selection and colour on a fresh palette
	void redraw() const
	{
		pal_preview_->setPalette(palette_);
		pal_preview_->palette().setGradient(
			pal_preview_->selectionStart(), pal_preview_->selectionEnd(), startColour(), endColour());
		pal_preview_->draw();
	}

private:
	PaletteCanvas*      pal_preview_    = nullptr;
	Palette*            palette_        = nullptr;
	wxColourPickerCtrl* cp_startcolour_ = nullptr;
	wxColourPickerCtrl* cp_endcolour_   = nullptr;
};


// -----------------------------------------------------------------------------
//
// PaletteEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PaletteEntryPanel class constructor
// -----------------------------------------------------------------------------
PaletteEntryPanel::PaletteEntryPanel(wxWindow* parent) : EntryPanel(parent, "palette")
{
	// Add palette canvas
	pal_canvas_ = new PaletteCanvas(this, -1);
	pal_canvas_->setSelectionType(PaletteCanvas::SelectionType::One);
	sizer_main_->Add(pal_canvas_->toPanel(this), 1, wxEXPAND, 0);

	// Setup custom menu
	menu_custom_ = new wxMenu();
	PaletteEntryPanel::fillCustomMenu(menu_custom_);
	custom_menu_name_ = "Palette";

	// --- Setup custom toolbar groups ---

	// Palette
	auto group_palette = new SToolBarGroup(toolbar_, "Palette", true);
	group_palette->addActionButton("pal_prev", "Previous Palette", "left", "");
	text_curpal_ = new wxStaticText(group_palette, -1, "XX/XX");
	group_palette->addCustomControl(text_curpal_);
	group_palette->addActionButton("pal_next", "Next Palette", "right", "");
	toolbar_->addGroup(group_palette);

	// Current Palette
	wxString actions = "ppal_moveup;ppal_movedown;ppal_duplicate;ppal_remove;ppal_removeothers";
	toolbar_->addActionGroup("Palette Organisation", wxSplit(actions, ';'));

	// Colour Operations
	actions = "ppal_colourise;ppal_tint;ppal_invert;ppal_tweak;ppal_gradient";
	toolbar_->addActionGroup("Colours", wxSplit(actions, ';'));

	// Palette Operations
	actions = "ppal_addcustom;ppal_exportas;ppal_importfrom;ppal_test;ppal_generate";
	toolbar_->addActionGroup("Palette Operations", wxSplit(actions, ';'));

	// Bind events
	pal_canvas_->Bind(wxEVT_LEFT_DOWN, &PaletteEntryPanel::onPalCanvasMouseEvent, this);
	pal_canvas_->Bind(wxEVT_RIGHT_DOWN, &PaletteEntryPanel::onPalCanvasMouseEvent, this);

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Reads all palettes in the PLAYPAL entry and shows the first one
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Clear any existing palettes
	palettes_.clear();

	// Determine how many palettes are in the entry
	const int n_palettes = entry->size() / 768;

	// Load each palette
	entry->seek(0, SEEK_SET);
	uint8_t pal_data[768];
	for (int a = 0; a < n_palettes; a++)
	{
		// Read palette data
		entry->read(&pal_data, 768);

		// Create palette
		auto pal = std::make_unique<Palette>();
		pal->loadMem(pal_data, 768);

		// Add palette
		palettes_.push_back(std::move(pal));
	}

	// Show first palette
	cur_palette_ = 0;
	showPalette(0);

	setModified(false);

	return true;
}

// -----------------------------------------------------------------------------
// Writes all loaded palettes in the palette entry
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::saveEntry()
{
	MemChunk full;
	MemChunk mc;

	// Write each palette as raw data
	for (auto& palette : palettes_)
	{
		palette->saveMem(mc);
		full.write(mc.data(), 768);
	}
	entry_->importMemChunk(full);
	setModified(false);

	return true;
}

// -----------------------------------------------------------------------------
// Returns a string with extended editing/entry info for the status bar
// -----------------------------------------------------------------------------
wxString PaletteEntryPanel::statusString()
{
	// Get current colour
	ColRGBA col  = pal_canvas_->selectedColour();
	ColHSL  col2 = col.asHSL();

	return wxString::Format(
		"Index %i\tR %d, G %d, B %d\tH %1.3f, S %1.3f, L %1.3f",
		pal_canvas_->selectionStart(),
		col.r,
		col.g,
		col.b,
		col2.h,
		col2.s,
		col2.l);
}

// -----------------------------------------------------------------------------
// Shows the palette at [index].
// Returns false if [index] is out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::showPalette(uint32_t index)
{
	// Check index range
	if (index >= palettes_.size())
		return false;

	// Copy palette at index into canvas
	pal_canvas_->palette().copyPalette(palettes_[index].get());

	// Set current palette text
	text_curpal_->SetLabel(wxString::Format("%u/%lu", index + 1, palettes_.size()));

	// Refresh
	Layout();
	pal_canvas_->Refresh();
	// The color values of the selected index have probably changed.
	updateStatus();

	return true;
}

// -----------------------------------------------------------------------------
// Redraws the panel
// -----------------------------------------------------------------------------
void PaletteEntryPanel::refreshPanel()
{
	if (entry_)
	{
		uint32_t our_palette = cur_palette_;
		if (our_palette > 0 && our_palette < palettes_.size())
			showPalette(our_palette);
	}
	Update();
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when a (EntryPanel) toolbar button is clicked
// -----------------------------------------------------------------------------
void PaletteEntryPanel::toolbarButtonClick(const wxString& action_id)
{
	// Prev. palette
	if (action_id == "pal_prev")
	{
		if (cur_palette_ == 0)
			cur_palette_ = palettes_.size();
		if (showPalette(cur_palette_ - 1))
			cur_palette_--;
	}

	// Next palette
	else if (action_id == "pal_next")
	{
		if (cur_palette_ + 1 == palettes_.size())
			cur_palette_ = -1;
		if (showPalette(cur_palette_ + 1))
			cur_palette_++;
	}
}

// -----------------------------------------------------------------------------
// Adds the current palette to the custom user palettes folder, so it can be
// selected via the palette selector
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::addCustomPalette()
{
	// Get name to export as
	auto name = wxGetTextFromUser("Enter name for Palette:", "Add to Palettes").ToStdString();
	if (name.empty())
		return false;

	// Write current palette to the user palettes directory
	auto path = App::path(fmt::format("palettes/{}.pal", name), App::Dir::User);
	palettes_[cur_palette_]->saveFile(path);

	// Add to palette manager and main palette chooser
	auto pal = std::make_unique<Palette>(*palettes_[cur_palette_]);
	App::paletteManager()->addPalette(std::move(pal), name);
	theMainWindow->paletteChooser()->addPalette(name);

	return true;
}

// -----------------------------------------------------------------------------
// A "lite" version of addCustomPalette, which does not add to the palette
// folder so the palette is only available for the current session.
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::testPalette()
{
	// Get name to export as
	wxString name = "Test: " + wxGetTextFromUser("Enter name for Palette:", "Test Palettes");

	// Add to palette manager and main palette chooser
	auto pal = std::make_unique<Palette>(*palettes_[cur_palette_]);
	App::paletteManager()->addPalette(std::move(pal), name.ToStdString());
	theMainWindow->paletteChooser()->addPalette(name);
	theMainWindow->paletteChooser()->selectPalette(name);

	return true;
}

// -----------------------------------------------------------------------------
// Exports the current palette to a file, in the selected format
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::exportAs()
{
	// Run save file dialog
	SFileDialog::FDInfo info;
	if (SFileDialog::saveFile(info, "Export Palette As", extensions, this))
		return palettes_[cur_palette_]->saveFile(info.filenames[0].ToStdString(), pal_formats[info.ext_index]);

	return false;
}

// -----------------------------------------------------------------------------
// Imports the selected file in the current palette
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::importFrom()
{
	bool ret = false;

	// Run open file dialog
	SFileDialog::FDInfo info;
	if (SFileDialog::openFile(info, "Import Palette As", extensions, this))
	{
		// Load palette
		ret = palettes_[cur_palette_]->loadFile(info.filenames[0].ToStdString(), pal_formats[info.ext_index]);
		if (ret)
		{
			setModified();
			showPalette(cur_palette_);
		}
		else
			wxMessageBox(Global::error, "Import Failed", wxICON_ERROR | wxOK);
	}
	return ret;
}

// -----------------------------------------------------------------------------
// Deletes the current palette from the list
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::clearOne()
{
	// Always keep at least one palette
	if (cur_palette_ == 0 && palettes_.size() == 1)
	{
		Log::warning("Palette cannot be removed, no other palette in this entry.");
		return false;
	}

	// Erase palette
	palettes_.erase(palettes_.begin() + cur_palette_);

	// Display the next, or previous, palette instead
	if (cur_palette_ >= palettes_.size())
		--cur_palette_;
	showPalette(cur_palette_);
	setModified();
	return true;
}

// -----------------------------------------------------------------------------
// Deletes all palettes except the current one from the list
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::clearOthers()
{
	// Nothing to do if there's already only one
	if (palettes_.size() == 1)
		return true;

	// Take current palette, clear them all and put it back as the first
	auto pal = std::move(palettes_[cur_palette_]);
	palettes_.clear();
	palettes_.push_back(std::move(pal));

	// Display the only remaining palette
	cur_palette_ = 0;
	showPalette(0);
	setModified();
	return true;
}

// -----------------------------------------------------------------------------
// Make a copy of the current palette and add it to the list
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::duplicate()
{
	palettes_.push_back(std::make_unique<Palette>(*palettes_[cur_palette_]));

	// Refresh the display to show the updated amount of palettes
	showPalette(cur_palette_);
	setModified();
	return true;
}

// -----------------------------------------------------------------------------
// Shifts the current palette's position in the list
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::move(bool infront)
{
	// Avoid invalid moves
	if (palettes_.size() == 1)
		return false;
	uint32_t newpos = cur_palette_;
	if (infront)
	{
		if (newpos == 0)
			newpos = palettes_.size() - 1;
		else
			--newpos;
	}
	else /* behind*/
	{
		if (newpos + 1 == palettes_.size())
			newpos = 0;
		else
			++newpos;
	}
	palettes_[cur_palette_].swap(palettes_[newpos]);

	// Refresh the display to show the updated amount of palettes
	cur_palette_ = newpos;
	showPalette(newpos);
	setModified();
	return true;
}

// -----------------------------------------------------------------------------
// Tints the colours of the current palette
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::tint()
{
	Palette           pal(*palettes_[cur_palette_]);
	PaletteTintDialog ptd(theMainWindow, &pal);
	if (ptd.ShowModal() == wxID_OK)
	{
		palettes_[cur_palette_]->copyPalette(ptd.finalPalette());
		showPalette(cur_palette_);
		setModified();
	}
	return true;
}

// -----------------------------------------------------------------------------
// Colourises the colours of the current palette
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::colourise()
{
	Palette                pal(*palettes_[cur_palette_]);
	PaletteColouriseDialog pcd(theMainWindow, &pal);
	if (pcd.ShowModal() == wxID_OK)
	{
		palettes_[cur_palette_]->copyPalette(pcd.finalPalette());
		showPalette(cur_palette_);
		setModified();
	}
	return true;
}

// -----------------------------------------------------------------------------
// Tweaks the colours of the current palette
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::tweak()
{
	Palette                  pal(*palettes_[cur_palette_]);
	PaletteColourTweakDialog pctd(theMainWindow, &pal);
	if (pctd.ShowModal() == wxID_OK)
	{
		palettes_[cur_palette_]->copyPalette(pctd.finalPalette());
		showPalette(cur_palette_);
		setModified();
	}
	return true;
}

// -----------------------------------------------------------------------------
// Inverts the colours of the current palette
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::invert()
{
	Palette             pal(*palettes_[cur_palette_]);
	PaletteInvertDialog pid(theMainWindow, &pal);
	if (pid.ShowModal() == wxID_OK)
	{
		palettes_[cur_palette_]->copyPalette(pid.finalPalette());
		showPalette(cur_palette_);
		setModified();
	}
	return true;
}

// -----------------------------------------------------------------------------
// Applies a gradient to the palette
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::gradient()
{
	Palette               pal(*palettes_[cur_palette_]);
	PaletteGradientDialog pgd(theMainWindow, &pal);
	if (pgd.ShowModal() == wxID_OK)
	{
		palettes_[cur_palette_]->copyPalette(pgd.finalPalette());
		showPalette(cur_palette_);
		setModified();
	}
	return true;
}

// -----------------------------------------------------------------------------
// Generates a COLORMAP lump from the current palette
// -----------------------------------------------------------------------------
#define DIMINISH(color, level) color = (uint8_t)((((float)(color)) * (32.0 - level) + 16.0) / 32.0)
bool PaletteEntryPanel::generateColormaps()
{
	static const int GREENMAP = 255;
	static const int GRAYMAP  = 32;

	if (!entry_ || !entry_->parent() || !palettes_[0])
		return false;

	MemChunk mc;
	SImage   img;
	MemChunk imc;
	mc.reSize(34 * 256);
	mc.seek(0, SEEK_SET);
	imc.reSize(34 * 256 * 4);
	imc.seek(0, SEEK_SET);
	uint8_t rgba[4];
	rgba[3] = 255;

	ColRGBA rgb;
	float   grey;
	// Generate 34 maps: the first 32 for diminishing light levels,
	// the 33th for the inverted grey map used by invulnerability.
	// The 34th colormap remains empty and black.
	for (size_t l = 0; l < 34; ++l)
	{
		for (size_t c = 0; c < 256; ++c)
		{
			rgb = palettes_[0]->colour(c);
			if (l < 32)
			{
				// Generate light maps
				DIMINISH(rgb.r, l);
				DIMINISH(rgb.g, l);
				DIMINISH(rgb.b, l);
#if (0)
			}
			else if (l == GREENMAP)
			{
				// Point of mostly useless trivia: the green "light amp" colormap in the Press Release beta
				// have colors that, on average, correspond to a bit less than (R*75/256, G*225/256, B*115/256)
#endif
			}
			else if (l == GRAYMAP)
			{
				// Generate inverse map
				grey = ((float)rgb.r / 256.0 * col_greyscale_r) + ((float)rgb.g / 256.0 * col_greyscale_g)
					   + ((float)rgb.b / 256.0 * col_greyscale_b);
				grey = 1.0 - grey;
				// Clamp value: with Id Software's values, the sum is greater than 1.0 (0.299+0.587+0.144=1.030)
				// This means the negation above can give a negative value (for example, with RGB values of 247 or
				// more), which will not be converted correctly to unsigned 8-bit int in the ColRGBA struct.
				if (grey < 0.0)
					grey = 0;
				rgb.r = rgb.g = rgb.b = grey * 255;
			}
			else
			{
				// Fill with 0
				rgb = palettes_[0]->colour(0);
			}
			rgba[0] = rgb.r;
			rgba[1] = rgb.g;
			rgba[2] = rgb.b;
			imc.write(&rgba, 4);
			mc[(256 * l) + c] = palettes_[0]->nearestColour(rgb);
		}
	}
#if 0
	// Create truecolor image
	uint8_t* imd = new uint8_t[256*34*4];
	memcpy(imd, imc.getData(), 256*34*4);
	img.setImageData(imd, 256, 34, RGBA);
	// imd will be freed by img's destructor
	ArchiveEntry* tcolormap;
	string name = entry->getName(true) + "-tcm.png";
	tcolormap = new ArchiveEntry(name);
	if (tcolormap)
	{
		entry->getParent()->addEntry(tcolormap);
		SIFormat::getFormat("png")->saveImage(img, tcolormap->getMCData());
		EntryType::detectEntryType(tcolormap);
	}
#endif
	// Now override or create new entry
	ArchiveEntry* colormap;
	colormap         = entry_->parent()->entry("COLORMAP", true);
	bool preexisting = colormap != nullptr;
	if (!colormap)
	{
		// We need to create this entry
		colormap = new ArchiveEntry("COLORMAP.lmp", 34 * 256);
	}
	if (!colormap)
		return false;
	colormap->importMemChunk(mc);
	if (!preexisting)
	{
		entry_->parent()->addEntry(colormap);
	}
	return true;
}
#undef DIMINISH

// -----------------------------------------------------------------------------
// Just a helper for generatePalettes to make the code less redundant
// -----------------------------------------------------------------------------
void PaletteEntryPanel::generatePalette(int r, int g, int b, int shift, int steps)
{
	// Seed it with the basic palette
	auto pal = std::make_unique<Palette>(*palettes_[0]);

	// Tint palette with given values
	pal->idtint(r, g, b, shift, steps);

	// Add it to the palette list
	palettes_.push_back(std::move(pal));
}

// -----------------------------------------------------------------------------
// Generates the full complement of palettes needed by the game
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::generatePalettes()
{
	GeneratePalettesDialog gpd(theMainWindow);
	if (gpd.ShowModal() == wxID_OK)
	{
		// Get choice
		int choice = gpd.choice();
		if (choice == 0)
			return false;

		// Make sure the current palette is the only one
		clearOthers();

		// The first thirteen palettes are common

		// Generate the eight REDPALS
		for (int a = 1; a < 9; ++a)
			generatePalette(255, 0, 0, a, 9);

		// Then the four BONUSPALS
		for (int a = 1; a < 5; ++a)
			generatePalette(215, 186, 69, a, 8);

		// And here we are at the crossroad
		if (choice == 1)
		{
			// Write the Doom/Heretic/Strife palettes, that is to say:

			// Write RADIATIONPAL with its oversaturated green
			generatePalette(0, 256, 0, 1, 8);
		}
		else
		{
			// Write all the Hexen palettes

			// Starting with the eight POISONPALS
			for (int a = 1; a < 9; ++a)
				generatePalette(44, 92, 36, a, 10);

			// Then the ICEPAL
			generatePalette(0, 0, 224, 1, 2);

			// The three HOLYPALS
			generatePalette(130, 130, 130, 1, 2);
			generatePalette(100, 100, 100, 1, 2);
			generatePalette(70, 70, 70, 1, 2);

			// And lastly the three SCOURGEPAL
			generatePalette(150, 110, 0, 1, 2);
			generatePalette(125, 92, 0, 1, 2);
			generatePalette(100, 73, 0, 1, 2);
		}

		// Refresh view to show changed amount of palettes
		cur_palette_ = 0;
		showPalette(0);
		setModified();
	}
	return true;
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// ----------------------------------------------------------------------------
bool PaletteEntryPanel::handleEntryPanelAction(std::string_view id)
{
	// Only interested in "ppal_" events
	if (!StrUtil::startsWith(id, "ppal_"))
		return false;

	// Add to custom palettes
	if (id == "ppal_addcustom")
	{
		addCustomPalette();
		return true;
	}

	// Test palette
	else if (id == "ppal_test")
	{
		testPalette();
		return true;
	}

	// Export As
	else if (id == "ppal_exportas")
	{
		exportAs();
		return true;
	}

	// Import From
	else if (id == "ppal_importfrom")
	{
		importFrom();
		return true;
	}

	// Generate Palettes
	if (id == "ppal_generate")
	{
		generatePalettes();
		return true;
	}

	// Generate Colormaps
	if (id == "ppal_colormap")
	{
		generateColormaps();
		return true;
	}

	// Colourise
	else if (id == "ppal_colourise")
	{
		colourise();
		return true;
	}

	// Tint
	else if (id == "ppal_tint")
	{
		tint();
		return true;
	}

	// Tweak
	else if (id == "ppal_tweak")
	{
		tweak();
		return true;
	}

	// Invert
	else if (id == "ppal_invert")
	{
		invert();
		return true;
	}

	// Gradient
	else if (id == "ppal_gradient")
	{
		gradient();
		return true;
	}

	// Move Up
	else if (id == "ppal_moveup")
	{
		move(true);
		return true;
	}

	// Move Down
	else if (id == "ppal_movedown")
	{
		move(false);
		return true;
	}

	// Duplicate
	else if (id == "ppal_duplicate")
	{
		duplicate();
		return true;
	}

	// Remove
	else if (id == "ppal_remove")
	{
		clearOne();
		return true;
	}

	// Remove Others
	else if (id == "ppal_removeothers")
	{
		clearOthers();
		return true;
	}

	// Some debug/reverse engineering stuff
	else if (id == "ppal_report")
	{
		analysePalettes();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Fills the given menu with the panel's custom actions. Used by both the
// constructor to create the main window's custom menu, and
// ArchivePanel::onEntryListRightClick to fill the context menu with
// context-appropriate stuff
// -----------------------------------------------------------------------------
bool PaletteEntryPanel::fillCustomMenu(wxMenu* custom)
{
	SAction::fromId("ppal_addcustom")->addToMenu(custom);
	SAction::fromId("ppal_exportas")->addToMenu(custom);
	SAction::fromId("ppal_importfrom")->addToMenu(custom);
	custom->AppendSeparator();
	SAction::fromId("ppal_colourise")->addToMenu(custom);
	SAction::fromId("ppal_tint")->addToMenu(custom);
	SAction::fromId("ppal_tweak")->addToMenu(custom);
	SAction::fromId("ppal_invert")->addToMenu(custom);
	SAction::fromId("ppal_gradient")->addToMenu(custom);
	SAction::fromId("ppal_test")->addToMenu(custom);
	custom->AppendSeparator();
	SAction::fromId("ppal_generate")->addToMenu(custom);
	SAction::fromId("ppal_duplicate")->addToMenu(custom);
	SAction::fromId("ppal_remove")->addToMenu(custom);
	SAction::fromId("ppal_removeothers")->addToMenu(custom);
	SAction::fromId("ppal_colormap")->addToMenu(custom);
	custom->AppendSeparator();
	SAction::fromId("ppal_moveup")->addToMenu(custom);
	SAction::fromId("ppal_movedown")->addToMenu(custom);
	//	custom->AppendSeparator();
	//	SAction::fromId("ppal_report")->addToMenu(custom);

	return true;
}


// -----------------------------------------------------------------------------
//
// PaletteEntryPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a mouse event happens within the palette canvas (eg. button
// clicked, pointer moved, etc)
// -----------------------------------------------------------------------------
void PaletteEntryPanel::onPalCanvasMouseEvent(wxMouseEvent& e)
{
	// Update colour info label with selected colour (if any)
	if (e.LeftDown())
	{
		// Send to palette canvas
		pal_canvas_->onMouseLeftDown(e);

		// Update status bar
		updateStatus();
	}
	else if (e.RightDown())
	{
		// Would this be better if the colour picking was handled by
		// the canvas' onMouseRightDown() function? The problem here
		// being that the canvas processes its events after the panel.
		// So for the left click we can afford to call it from there
		// first and let it harmlessly process it again, but for the
		// right click it would result in the colour box being shown
		// twice to the user, the second time being ignored. So it is
		// preferrable to handle all this on this side rather than try
		// to make the canvas do the work.
		// Pretend there was a left click to get the selected colour.
		pal_canvas_->onMouseLeftDown(e);
		int sel = pal_canvas_->selectionStart();

		// There actually was a colour selected
		if (sel > -1)
		{
			ColRGBA col = pal_canvas_->selectedColour();
			// Open a colour dialog
			wxColour cd = wxGetColourFromUser(GetParent(), WXCOL(col));

			if (cd.Ok())
			{
				col.r = cd.Red();
				col.g = cd.Green();
				col.b = cd.Blue();

				palettes_[cur_palette_]->setColour(sel, col);
				setModified();
				showPalette(cur_palette_);
			}
		}
	}
}










//  Just some reverse-engineering stuff.
#define GPALCOMPANALYSIS
#define PALETTECHECK 1
void PaletteEntryPanel::analysePalettes()
{
	if (palettes_.size() < PALETTECHECK + 1)
		return;
#ifdef GPALCOMPANALYSIS
	int devR, devG, devB;
	int minR, minG, minB;
	int maxR, maxG, maxB;
	int wrongcount;
#else
	unsigned int reds[256];
	unsigned int greens[256];
	unsigned int blues[256];
#endif
	wxString report = "\n";
#ifdef GPALCOMPANALYSIS
	int i = PALETTECHECK;
	if (i)
	{
		report += wxString::Format("Deviation between palettes 0 and %i:\n\n", i);
		devR = devG = devB = 0;
		maxR = maxG = maxB = -1;
		minR = minG = minB = 256;
		wrongcount         = 0;
#else
	report += wxString::Format("Changes between %lu palettes compared to the first:\n\n", palettes.size());
	for (size_t i = 1; i < palettes.size(); ++i)
	{
		for (int j = 0; j < 256; ++j)
			reds[j] = blues[j] = greens[j] = 999;
#endif
		report += wxString::Format("\n==============\n= Palette %02u =\n==============\n", i);
		for (size_t c = 0; c < 256; ++c)
		{
			ColRGBA ref1 = palettes_[0]->colour(c);
			ColRGBA cmp1 = palettes_[i]->colour(c);
			ColHSL  ref2 = ref1.asHSL();
			ColHSL  cmp2 = cmp1.asHSL();
#ifdef GPALCOMPANALYSIS
			int    r, g, b;
			double h, s, l;
			r = cmp1.r - ref1.r;
			g = cmp1.g - ref1.g;
			b = cmp1.b - ref1.b;
			h = cmp2.h - ref2.h;
			s = cmp2.s - ref2.s;
			l = cmp2.l - ref2.l;
			devR += r;
			devG += g;
			devB += b;
			if (r > maxR)
				maxR = r;
			if (r < minR)
				minR = r;
			if (g > maxG)
				maxG = g;
			if (g < minG)
				minG = g;
			if (b > maxB)
				maxB = b;
			if (b < minB)
				minB = b;
			if (r | g | b)
			{
				++wrongcount;
				report += wxString::Format(
					"Index %003u: [%003i %003i %003i | %1.3f %1.3f %1.3f]->[%003i %003i %003i | %1.3f %1.3f "
					"%1.3f]\t\tR %+003i\tG %+003i\tB %+003i\t\t\tH %+1.3f\tS %+1.3f\tL %+1.3f\n",
					c,
					ref1.r,
					ref1.g,
					ref1.b,
					ref2.h,
					ref2.s,
					ref2.l,
					cmp1.r,
					cmp1.g,
					cmp1.b,
					cmp2.h,
					cmp2.s,
					cmp2.l,
					r,
					g,
					b,
					h,
					s,
					l);
			}
#else
			if (reds[ref1.r] != cmp1.r && reds[ref1.r] != 999)
				DPrintf(
					"Discrepency for red channel at index %i, value %i: %d vs. %d set before",
					c,
					ref1.r,
					cmp1.r,
					reds[ref1.r]);
			if (greens[ref1.g] != cmp1.g && greens[ref1.g] != 999)
				DPrintf(
					"Discrepency for green channel at index %i, value %i: %d vs. %d set before",
					c,
					ref1.g,
					cmp1.g,
					greens[ref1.g]);
			if (blues[ref1.b] != cmp1.b && blues[ref1.b] != 999)
				DPrintf(
					"Discrepency for blue channel at index %i, value %i: %d vs. %d set before",
					c,
					ref1.b,
					cmp1.b,
					blues[ref1.b]);
			reds[ref1.r]   = cmp1.r;
			greens[ref1.g] = cmp1.g;
			blues[ref1.b]  = cmp1.b;
#endif
		}
#ifdef GPALCOMPANALYSIS
		report += wxString::Format(
			"Deviation sigma: R %+003i G %+003i B %+003i\t%s\n", devR, devG, devB, entry_->name());
		report += wxString::Format(
			"Min R %+003i Min G %+003i Min B %+003i Max R %+003i Max G %+003i Max B %+003i \nError count: %i\n",
			minR,
			minG,
			minB,
			maxR,
			maxG,
			maxB,
			wrongcount);
#else
		report += "Shift table for existing channel values:\n|  I  |  R  |  G  |  B  |\n";
		for (size_t i = 0; i < 256; ++i)
		{
			if (reds[i] < 999 || greens[i] < 999 || blues[i] < 999)
				report += wxString::Format("| %003d | %003d | %003d | %003d |\n", i, reds[i], greens[i], blues[i]);
		}
		report.Replace("999", "   ");
#endif
	}

	Log::info(1, report);
}
