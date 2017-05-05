
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PaletteEntryPanel.cpp
 * Description: PaletteEntryPanel class. The UI for editing
 *              palette (PLAYPAL) entries
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
#include "App.h"
#include "Archive/Archive.h"
#include "PaletteEntryPanel.h"
#include "General/Misc.h"
#include "Graphics/Icons.h"
#include "Graphics/Palette/PaletteManager.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/Canvas/PaletteCanvas.h"
#include "UI/PaletteChooser.h"
#include "Utility/SFileDialog.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Float, col_greyscale_r, 0.299, CVAR_SAVE)
CVAR(Float, col_greyscale_g, 0.587, CVAR_SAVE)
CVAR(Float, col_greyscale_b, 0.114, CVAR_SAVE)


/*******************************************************************
 * PALETTECOLOURISEDIALOG CLASS
 *******************************************************************
 A simple dialog for the 'Colourise' function, allows the user to
 select a colour and shows a preview of the colourised palette
 */
class PaletteColouriseDialog : public wxDialog
{
private:
	PaletteCanvas*		pal_preview;
	Palette8bit*		palette;
	wxColourPickerCtrl*	cp_colour;

public:
	PaletteColouriseDialog(wxWindow* parent, Palette8bit* pal)
		: wxDialog(parent, -1, "Colourise", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
	{
		// Init variable
		this->palette = pal;

		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "palette_colourise"));
		SetIcon(icon);

		// Setup main sizer
		wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND|wxALL, 6);

		// Add colour chooser
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);

		cp_colour = new wxColourPickerCtrl(this, -1, wxColour(255, 0, 0));
		hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(cp_colour, 0, wxEXPAND);

		// Add preview
		pal_preview = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview, 1, wxEXPAND|wxALL, 4);

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxBOTTOM|wxTOP, 4);

		// Setup preview
		pal_preview->allowSelection(2);
		pal_preview->SetInitialSize(wxSize(384, 384));
		redraw();

		// Init layout
		Layout();

		// Bind events
		cp_colour->Bind(wxEVT_COLOURPICKER_CHANGED, &PaletteColouriseDialog::onColourChanged, this);
		pal_preview->Bind(wxEVT_LEFT_UP, &PaletteColouriseDialog::onPaletteLeftUp, this);

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		SetMinSize(GetSize());
		CenterOnParent();
	}

	Palette8bit* getFinalPalette()
	{
		return &(pal_preview->getPalette());
	}

	rgba_t getColour()
	{
		wxColour col = cp_colour->GetColour();
		return rgba_t(col.Red(), col.Green(), col.Blue());
	}

	// Re-apply the changes in selection and colour on a fresh palette
	void redraw()
	{
		pal_preview->setPalette(palette);
		pal_preview->getPalette().colourise(getColour(),
		                                    pal_preview->getSelectionStart(), pal_preview->getSelectionEnd());
		pal_preview->draw();
	}

	// Events
	void onColourChanged(wxColourPickerEvent& e)
	{
		redraw();
	}

	void onPaletteLeftUp(wxMouseEvent& e)
	{
		redraw();
	}
};

/*******************************************************************
 * PALETTETINTDIALOG CLASS
 *******************************************************************
 A simple dialog for the 'Tint' function, allows the user to select
 tint colour+amount and shows a preview of the tinted palette
 */
class PaletteTintDialog : public wxDialog
{
private:
	PaletteCanvas*		pal_preview;
	Palette8bit*		palette;
	wxColourPickerCtrl*	cp_colour;
	wxSlider*			slider_amount;
	wxStaticText*		label_amount;

public:
	PaletteTintDialog(wxWindow* parent, Palette8bit* pal)
		: wxDialog(parent, -1, "Tint", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
	{
		// Init variables
		this->palette = pal;

		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "palette_tint"));
		SetIcon(icon);

		// Setup main sizer
		wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND|wxALL, 6);

		// Add colour chooser
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);

		cp_colour = new wxColourPickerCtrl(this, -1, wxColour(255, 0, 0));
		hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(cp_colour, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

		// Add 'amount' slider
		hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

		slider_amount = new wxSlider(this, -1, 50, 0, 100);
		label_amount = new wxStaticText(this, -1, "100%");
		hbox->Add(new wxStaticText(this, -1, "Amount:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(slider_amount, 1, wxEXPAND|wxRIGHT, 4);
		hbox->Add(label_amount, 0, wxALIGN_CENTER_VERTICAL);

		// Add preview
		pal_preview = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxBOTTOM|wxTOP, 4);

		// Setup preview
		pal_preview->allowSelection(2);
		pal_preview->SetInitialSize(wxSize(384, 384));
		redraw();

		// Init layout
		Layout();

		// Bind events
		cp_colour->Bind(wxEVT_COLOURPICKER_CHANGED, &PaletteTintDialog::onColourChanged, this);
		slider_amount->Bind(wxEVT_SLIDER, &PaletteTintDialog::onAmountChanged, this);
		pal_preview->Bind(wxEVT_LEFT_UP, &PaletteTintDialog::onPaletteLeftUp, this);

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		SetMinSize(GetSize());
		CenterOnParent();

		// Set values
		label_amount->SetLabel("50% ");
	}

	Palette8bit* getFinalPalette()
	{
		return &(pal_preview->getPalette());
	}

	rgba_t getColour()
	{
		wxColour col = cp_colour->GetColour();
		return rgba_t(col.Red(), col.Green(), col.Blue());
	}

	float getAmount()
	{
		return (float)slider_amount->GetValue()*0.01f;
	}

	// Re-apply the changes in selection, colour and amount on a fresh palette
	void redraw()
	{
		pal_preview->setPalette(palette);
		pal_preview->getPalette().tint(getColour(), getAmount(),
		                               pal_preview->getSelectionStart(), pal_preview->getSelectionEnd());
		pal_preview->draw();
	}

	// Events
	void onColourChanged(wxColourPickerEvent& e)
	{
		redraw();
	}

	void onAmountChanged(wxCommandEvent& e)
	{
		redraw();
		label_amount->SetLabel(S_FMT("%d%% ", slider_amount->GetValue()));
	}

	void onPaletteLeftUp(wxMouseEvent& e)
	{
		redraw();
	}
};

/*******************************************************************
 * PALETTECOLOURTWEAKDIALOG CLASS
 *******************************************************************
 A simple dialog for the 'Tweak Colours' function, allows the user to
 select hue, saturation and luminosity changes and shows a preview of
 the modified palette. TODO: More features? Maybe merge Tint, Invert
 and Colourise with it, add an "Apply Change" button so that it isn't
 needed anymore to click "OK" and close it after each change, etc.
 */
class PaletteColourTweakDialog : public wxDialog
{
private:
	PaletteCanvas*		pal_preview;
	Palette8bit*		palette;
	wxSlider*			slider_hue;
	wxSlider*			slider_sat;
	wxSlider*			slider_lum;
	wxStaticText*		label_hue;
	wxStaticText*		label_sat;
	wxStaticText*		label_lum;

public:
	PaletteColourTweakDialog(wxWindow* parent, Palette8bit* pal)
		: wxDialog(parent, -1, "Tweak Colours", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
	{
		// Init variables
		this->palette = pal;

		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "palette_tweak"));
		SetIcon(icon);

		// Setup main sizer
		wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND|wxALL, 6);
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);

		// Add 'hue shift' slider
		hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

		slider_hue = new wxSlider(this, -1, 0, 0, 500);
		label_hue = new wxStaticText(this, -1, "0.000");
		hbox->Add(new wxStaticText(this, -1, "Hue Shift:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(slider_hue, 1, wxEXPAND|wxRIGHT, 4);
		hbox->Add(label_hue, 0, wxALIGN_CENTER_VERTICAL);

		// Add 'Saturation' slider
		hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

		slider_sat = new wxSlider(this, -1, 100, 0, 200);
		label_sat = new wxStaticText(this, -1, "100%");
		hbox->Add(new wxStaticText(this, -1, "Saturation:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(slider_sat, 1, wxEXPAND|wxRIGHT, 4);
		hbox->Add(label_sat, 0, wxALIGN_CENTER_VERTICAL);

		// Add 'Luminosity' slider
		hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

		slider_lum = new wxSlider(this, -1, 100, 0, 200);
		label_lum = new wxStaticText(this, -1, "100%");
		hbox->Add(new wxStaticText(this, -1, "Luminosity:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(slider_lum, 1, wxEXPAND|wxRIGHT, 4);
		hbox->Add(label_lum, 0, wxALIGN_CENTER_VERTICAL);

		// Add preview
		pal_preview = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxBOTTOM|wxTOP, 4);

		// Setup preview
		pal_preview->allowSelection(2);
		pal_preview->SetInitialSize(wxSize(384, 384));
		redraw();

		// Init layout
		Layout();

		// Bind events
		slider_hue->Bind(wxEVT_SLIDER, &PaletteColourTweakDialog::onHueChanged, this);
		slider_sat->Bind(wxEVT_SLIDER, &PaletteColourTweakDialog::onSatChanged, this);
		slider_lum->Bind(wxEVT_SLIDER, &PaletteColourTweakDialog::onLumChanged, this);
		pal_preview->Bind(wxEVT_LEFT_UP, &PaletteColourTweakDialog::onPaletteLeftUp, this);

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		SetMinSize(GetSize());
		CenterOnParent();

		// Set values
		label_hue->SetLabel("0.000 ");
		label_sat->SetLabel("100% ");
		label_lum->SetLabel("100% ");
	}

	Palette8bit* getFinalPalette()
	{
		return &(pal_preview->getPalette());
	}

	float getHue()
	{
		return (float)slider_hue->GetValue()*0.002f;
	}

	float getSat()
	{
		return (float)slider_sat->GetValue()*0.01f;
	}

	float getLum()
	{
		return (float)slider_lum->GetValue()*0.01f;
	}

	// Re-apply the changes in selection, hue, saturation and luminosity on a fresh palette
	void redraw()
	{
		pal_preview->setPalette(palette);
		pal_preview->getPalette().shift(getHue(),
		                                pal_preview->getSelectionStart(), pal_preview->getSelectionEnd());
		pal_preview->getPalette().saturate(getSat(),
		                                   pal_preview->getSelectionStart(), pal_preview->getSelectionEnd());
		pal_preview->getPalette().illuminate(getLum(),
		                                     pal_preview->getSelectionStart(), pal_preview->getSelectionEnd());
		pal_preview->draw();
	}

	// Events
	void onHueChanged(wxCommandEvent& e)
	{
		redraw();
		label_hue->SetLabel(S_FMT("%1.3f", getHue()));
	}

	void onSatChanged(wxCommandEvent& e)
	{
		redraw();
		label_sat->SetLabel(S_FMT("%d%%", slider_sat->GetValue()));
	}

	void onLumChanged(wxCommandEvent& e)
	{
		redraw();
		label_lum->SetLabel(S_FMT("%d%%", slider_lum->GetValue()));
	}

	void onPaletteLeftUp(wxMouseEvent& e)
	{
		redraw();
	}
};

/*******************************************************************
 * PALETTEINVERTDIALOG CLASS
 *******************************************************************
 A simple dialog for the 'Invert' function, allows the user to
 invert the colours and shows a preview of the inverted palette
 */
class PaletteInvertDialog : public wxDialog
{
private:
	PaletteCanvas*		pal_preview;
	Palette8bit*		palette;

public:
	PaletteInvertDialog(wxWindow* parent, Palette8bit* pal)
		: wxDialog(parent, -1, "Invert", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
	{
		// Init variable
		this->palette = pal;

		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "palette_invert"));
		SetIcon(icon);

		// Setup main sizer
		wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND|wxALL, 6);

		// Add preview
		pal_preview = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview, 1, wxEXPAND|wxALL, 4);

		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxBOTTOM|wxTOP, 4);

		// Setup preview
		pal_preview->allowSelection(2);
		pal_preview->SetInitialSize(wxSize(384, 384));
		redraw();

		// Init layout
		Layout();

		// Bind events
		pal_preview->Bind(wxEVT_LEFT_UP, &PaletteInvertDialog::onPaletteLeftUp, this);

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		SetMinSize(GetSize());
		CenterOnParent();
	}

	Palette8bit* getFinalPalette()
	{
		return &(pal_preview->getPalette());
	}

	// Re-apply the changes in selection on a fresh palette
	void redraw()
	{
		pal_preview->setPalette(palette);
		pal_preview->getPalette().invert(
		    pal_preview->getSelectionStart(), pal_preview->getSelectionEnd());
		pal_preview->draw();
	}

	// Events
	void onColourChanged(wxColourPickerEvent& e)
	{
		redraw();
	}

	void onPaletteLeftUp(wxMouseEvent& e)
	{
		redraw();
	}
};

/*******************************************************************
 * GENERATEPALETTESDIALOG CLASS
 *******************************************************************
 A simple dialog for the 'Generate Palettes' function, allows to
 choose between generating the 14 palettes appropriate for Doom,
 Heretic and Strife, or the 28 palettes appropriate for Hexen.
 */
class GeneratePalettesDialog : public wxDialog
{
private:
	wxRadioButton*	rb_doom;
	wxRadioButton*	rb_hexen;

public:
	GeneratePalettesDialog(wxWindow* parent)
		: wxDialog(parent, -1, "Generate Palettes", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
	{

		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::ENTRY, "palette"));
		SetIcon(icon);

		// Setup main sizer
		wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND|wxALL, 6);

		// Add buttons
		rb_doom = new wxRadioButton(this, -1, "Doom (14 Palettes)", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		sizer->Add(rb_doom, 0, wxEXPAND|wxALL, 4);
		rb_hexen = new wxRadioButton(this, -1, "Hexen (28 Palettes)");
		sizer->Add(rb_hexen, 0, wxEXPAND|wxALL, 4);

		sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxBOTTOM|wxTOP, 4);

		// Init layout
		Layout();

		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		SetMinSize(GetSize());
		CenterOnParent();
	}

	int getChoice()
	{
		if (rb_doom->GetValue())
			return 1;
		else if  (rb_hexen->GetValue())
			return 2;
		else
			return 0;
	}
};

/*******************************************************************
 * PALETTEGRADIENTDIALOG CLASS
 *******************************************************************
 A dialog for the 'Gradient' function, allows the user to create a 
 gradient between two colours, and apply it to a range of indexes in 
 the palette.
 */
class PaletteGradientDialog : public wxDialog
{
private:
	PaletteCanvas*		pal_preview;
	Palette8bit*		palette;
	wxColourPickerCtrl*	cp_startcolour;
	wxColourPickerCtrl*	cp_endcolour;
	
public:
	PaletteGradientDialog(wxWindow* parent, Palette8bit* pal)
	: wxDialog(parent, -1, "Gradient", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
	{
		// Init variable
		this->palette = pal;
		
		// Set dialog icon
		wxIcon icon;
		icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "palette_gradient"));
		SetIcon(icon);
		
		// Setup main sizer
		wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		msizer->Add(sizer, 1, wxEXPAND|wxALL, 6);
		
		// Add colour choosers
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
		
		cp_startcolour = new wxColourPickerCtrl(this, -1, wxColour(0, 0, 0));
		hbox->Add(new wxStaticText(this, -1, "Start Colour:"), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(cp_startcolour, 0, wxEXPAND);
		
		cp_endcolour = new wxColourPickerCtrl(this, -1, wxColour(255, 255, 255));
		hbox->Add(new wxStaticText(this, -1, "End Colour:"), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
		hbox->Add(cp_endcolour, 0, wxEXPAND);
		
		// Add preview
		pal_preview = new PaletteCanvas(this, -1);
		sizer->Add(pal_preview, 1, wxEXPAND|wxALL, 4);
		
		// Add buttons
		sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxBOTTOM|wxTOP, 4);
		
		// Setup preview
		pal_preview->allowSelection(2);
		pal_preview->SetInitialSize(wxSize(384, 384));
		redraw();
		
		// Init layout
		Layout();
		
		// Bind events
		cp_startcolour->Bind(wxEVT_COLOURPICKER_CHANGED, &PaletteGradientDialog::onColourChanged, this);
		cp_endcolour->Bind(wxEVT_COLOURPICKER_CHANGED, &PaletteGradientDialog::onColourChanged, this);
		pal_preview->Bind(wxEVT_LEFT_UP, &PaletteGradientDialog::onPaletteLeftUp, this);
		
		// Setup dialog size
		SetInitialSize(wxSize(-1, -1));
		SetMinSize(GetSize());
		CenterOnParent();
	}
	
	Palette8bit* getFinalPalette()
	{
		return &(pal_preview->getPalette());
	}
	
	rgba_t getStartColour()
	{
		wxColour col = cp_startcolour->GetColour();
		return rgba_t(col.Red(), col.Green(), col.Blue());
	}
	
	rgba_t getEndColour()
	{
		wxColour col = cp_endcolour->GetColour();
		return rgba_t(col.Red(), col.Green(), col.Blue());
	}
	
	// Re-apply the changes in selection and colour on a fresh palette
	void redraw()
	{
		pal_preview->setPalette(palette);
		pal_preview->getPalette().setGradient(
			pal_preview->getSelectionStart(), pal_preview->getSelectionEnd(),
											  getStartColour(), getEndColour());
		pal_preview->draw();
	}
	
	// Events
	void onColourChanged(wxColourPickerEvent& e)
	{
		redraw();
	}
	
	void onPaletteLeftUp(wxMouseEvent& e)
	{
		redraw();
	}
};



/*******************************************************************
 * PALETTEENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* PaletteEntryPanel::PaletteEntryPanel
 * PaletteEntryPanel class constructor
 *******************************************************************/
PaletteEntryPanel::PaletteEntryPanel(wxWindow* parent)
	: EntryPanel(parent, "palette")
{
	// Add palette canvas
	pal_canvas = new PaletteCanvas(this, -1);
	pal_canvas->allowSelection(1);
	sizer_main->Add(pal_canvas->toPanel(this), 1, wxEXPAND, 0);

	// Setup custom menu
	menu_custom = new wxMenu();
	fillCustomMenu(menu_custom);
	custom_menu_name = "Palette";

	// --- Setup custom toolbar groups ---

	// Palette
	SToolBarGroup* group_palette = new SToolBarGroup(toolbar, "Palette", true);
	group_palette->addActionButton("pal_prev", "Previous Palette", "left", "");
	text_curpal = new wxStaticText(group_palette, -1, "XX/XX");
	group_palette->addCustomControl(text_curpal);
	group_palette->addActionButton("pal_next", "Next Palette", "right", "");
	toolbar->addGroup(group_palette);

	// Current Palette
	string actions = "ppal_moveup;ppal_movedown;ppal_duplicate;ppal_remove;ppal_removeothers";
	toolbar->addActionGroup("Palette Organisation", wxSplit(actions, ';'));

	// Colour Operations
	actions = "ppal_colourise;ppal_tint;ppal_invert;ppal_tweak;ppal_gradient";
	toolbar->addActionGroup("Colours", wxSplit(actions, ';'));

	// Palette Operations
	actions = "ppal_addcustom;ppal_exportas;ppal_importfrom;ppal_test;ppal_generate";
	toolbar->addActionGroup("Palette Operations", wxSplit(actions, ';'));

	// Bind events
	pal_canvas->Bind(wxEVT_LEFT_DOWN, &PaletteEntryPanel::onPalCanvasMouseEvent, this);
	pal_canvas->Bind(wxEVT_RIGHT_DOWN, &PaletteEntryPanel::onPalCanvasMouseEvent, this);

	// Init variables
	cur_palette = 0;

	Layout();
}

/* PaletteEntryPanel::~PaletteEntryPanel
 * PaletteEntryPanel class destructor
 *******************************************************************/
PaletteEntryPanel::~PaletteEntryPanel()
{
	for (size_t a = 0; a < palettes.size(); a++)
		delete palettes[a];
}

/* PaletteEntryPanel::loadEntry
 * Reads all palettes in the PLAYPAL entry and shows the first one
 *******************************************************************/
bool PaletteEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Clear any existing palettes
	for (size_t a = 0; a < palettes.size(); a++)
		delete palettes[a];
	palettes.clear();

	// Determine how many palettes are in the entry
	int n_palettes = entry->getSize() / 768;

	// Load each palette
	entry->seek(0, SEEK_SET);
	uint8_t pal_data[768];
	for (int a = 0; a < n_palettes; a++)
	{
		// Read palette data
		entry->read(&pal_data, 768);

		// Create palette
		Palette8bit* pal = new Palette8bit();
		pal->loadMem(pal_data, 768);

		// Add palette
		palettes.push_back(pal);
	}

	// Show first palette
	cur_palette = 0;
	showPalette(0);

	setModified(false);

	return true;
}

/* PaletteEntryPanel::saveEntry
 * Writes all loaded palettes in the palette entry
 *******************************************************************/
bool PaletteEntryPanel::saveEntry()
{
	MemChunk full;
	MemChunk mc;

	// Write each palette as raw data
	for (size_t a = 0; a < palettes.size(); a++)
	{
		palettes[a]->saveMem(mc, Palette8bit::FORMAT_RAW);
		full.write(mc.getData(), 768);
	}
	entry->importMemChunk(full);
	setModified(false);

	return true;
}

/* PaletteEntryPanel::statusString
 * Returns a string with extended editing/entry info for the status
 * bar
 *******************************************************************/
string PaletteEntryPanel::statusString()
{
	// Get current colour
	rgba_t col = pal_canvas->getSelectedColour();
	hsl_t col2 = Misc::rgbToHsl(col);

	return S_FMT("Index %i\tR %d, G %d, B %d\tH %1.3f, S %1.3f, L %1.3f",
	             pal_canvas->getSelectionStart(),
	             col.r, col.g, col.b, col2.h, col2.s, col2.l);
}

/* PaletteEntryPanel::showPalette
 * Shows the palette at [index]. Returns false if [index] is out of
 * bounds, true otherwise
 *******************************************************************/
bool PaletteEntryPanel::showPalette(uint32_t index)
{
	// Check index range
	if (index >= palettes.size())
		return false;

	// Copy palette at index into canvas
	pal_canvas->getPalette().copyPalette(palettes[index]);

	// Set current palette text
	text_curpal->SetLabel(S_FMT("%u/%lu", index+1, palettes.size()));

	// Refresh
	Layout();
	pal_canvas->Refresh();
	// The color values of the selected index have probably changed.
	updateStatus();

	return true;
}

/* PaletteEntryPanel::refreshPanel
 * Redraws the panel
 *******************************************************************/
void PaletteEntryPanel::refreshPanel()
{
	if (entry)
	{
		uint32_t our_palette = cur_palette;
		//loadEntry(entry);
		if (our_palette > 0 && our_palette < palettes.size())
			showPalette(our_palette);
	}
	Update();
	Refresh();
}

/* PaletteEntryPanel::toolbarButtonClick
 * Called when a (EntryPanel) toolbar button is clicked
 *******************************************************************/
void PaletteEntryPanel::toolbarButtonClick(string action_id)
{
	// Prev. palette
	if (action_id == "pal_prev")
	{
		if (cur_palette == 0)
			cur_palette = palettes.size();
		if (showPalette(cur_palette - 1))
			cur_palette--;
	}

	// Next palette
	else if (action_id == "pal_next")
	{
		if (cur_palette + 1 == palettes.size())
			cur_palette = -1;
		if (showPalette(cur_palette + 1))
			cur_palette++;
	}
}

/* PaletteEntryPanel::addCustomPalette
 * Adds the current palette to the custom user palettes folder, so
 * it can be selected via the palette selector
 *******************************************************************/
bool PaletteEntryPanel::addCustomPalette()
{
	// Get name to export as
	string name = wxGetTextFromUser("Enter name for Palette:", "Add to Palettes");
	if (name.IsEmpty())
		return false;

	// Write current palette to the user palettes directory
	string path = App::path(S_FMT("palettes/%s.pal", name), App::Dir::User);
	palettes[cur_palette]->saveFile(path);

	// Add to palette manager and main palette chooser
	auto pal = std::make_unique<Palette8bit>();
	pal->copyPalette(palettes[cur_palette]);
	App::paletteManager()->addPalette(std::move(pal), name);
	theMainWindow->getPaletteChooser()->addPalette(name);

	return true;
}

/* PaletteEntryPanel::testPalette
 * A "lite" version of addCustomPalette, which does not add to the
 * palette folder so the palette is only available for the current
 * session.
 *******************************************************************/
bool PaletteEntryPanel::testPalette()
{
	// Get name to export as
	string name = "Test: " + wxGetTextFromUser("Enter name for Palette:", "Test Palettes");

	// Add to palette manager and main palette chooser
	auto pal = std::make_unique<Palette8bit>();
	pal->copyPalette(palettes[cur_palette]);
	App::paletteManager()->addPalette(std::move(pal), name);
	theMainWindow->getPaletteChooser()->addPalette(name);
	theMainWindow->getPaletteChooser()->selectPalette(name);

	return true;
}

/* PaletteEntryPanel::exportAs
 * Exports the current palette to a file, in the selected format
 *******************************************************************/
bool PaletteEntryPanel::exportAs()
{
	// Run save file dialog
	SFileDialog::fd_info_t info;
	string extensions = "Raw Palette (*.pal)|*.pal|PNG File (*.png)|*.png|CSV Palette (*.csv)|*.csv|JASC Palette (*.pal)|*.pal|GIMP Palette (*.gpl)|*.gpl";
	if (SFileDialog::saveFile(info, "Export Palette As", extensions, this))
		return palettes[cur_palette]->saveFile(info.filenames[0], info.ext_index);

	return false;
}

/* PaletteEntryPanel::importFrom
 * Imports the selected file in the current palette
 *******************************************************************/
bool PaletteEntryPanel::importFrom()
{
	bool ret = false;

	// Run open file dialog
	SFileDialog::fd_info_t info;
	string extensions = "Raw Palette (*.pal)|*.pal|PNG File (*.png)|*.png|CSV Palette (*.csv)|*.csv|JASC Palette (*.pal)|*.pal|GIMP Palette (*.gpl)|*.gpl";
	if (SFileDialog::openFile(info, "Import Palette As", extensions, this))
	{
		// Load palette
		ret = palettes[cur_palette]->loadFile(info.filenames[0], info.ext_index);
		if (ret)
		{
			setModified();
			showPalette(cur_palette);
		}
		else
			wxMessageBox(Global::error, "Import Failed", wxICON_ERROR | wxOK);
	}
	return ret;
}

/* PaletteEntryPanel::clearOne
 * Deletes the current palette from the list
 *******************************************************************/
bool PaletteEntryPanel::clearOne()
{
	// Always keep at least one palette
	if (cur_palette == 0 && palettes.size() == 1)
	{
		LOG_MESSAGE(1, "Palette cannot be removed, no other palette in this entry.");
		return false;
	}

	// Erase palette
	delete palettes[cur_palette];
	palettes.erase(palettes.begin() + cur_palette);

	// Display the next, or previous, palette instead
	if (cur_palette >= palettes.size())
		--cur_palette;
	showPalette(cur_palette);
	setModified();
	return true;
}

/* PaletteEntryPanel::clearOne
 * Deletes all palettes except the current one from the list
 *******************************************************************/
bool PaletteEntryPanel::clearOthers()
{
	// Nothing to do if there's already only one
	if (palettes.size() == 1)
		return true;

	// Keeping a pointer to the palette we keep
	Palette8bit* pal = palettes[cur_palette];

	// Swap current palette with the first one if needed
	if (cur_palette != 0)
	{
		palettes[cur_palette] = palettes[0];
		palettes[0] = pal;
	}

	// Delete all palettes after the first
	for (size_t i = 1; i < palettes.size(); ++i)
	{
		delete palettes[i];
	}
	// Empty the palette list and add back the single palette
	palettes.clear();
	palettes.push_back(pal);

	// Display the only remaining palette
	cur_palette = 0;
	showPalette(0);
	setModified();
	return true;
}

/* PaletteEntryPanel::duplicate
 * Make a copy of the current palette and add it to the list
 *******************************************************************/
bool PaletteEntryPanel::duplicate()
{
	Palette8bit* newpalette = new Palette8bit;
	if (!newpalette)
		return false;

	newpalette->copyPalette(palettes[cur_palette]);
	palettes.push_back(newpalette);

	// Refresh the display to show the updated amount of palettes
	showPalette(cur_palette);
	setModified();
	return true;
}

/* PaletteEntryPanel::move
 * Shifts the current palette's position in the list
 *******************************************************************/
bool PaletteEntryPanel::move(bool infront)
{
	// Avoid invalid moves
	if (palettes.size() == 1)
		return false;
	uint32_t newpos = cur_palette;
	if (infront)
	{
		if (newpos == 0)
			newpos = palettes.size() - 1;
		else --newpos;
	}
	else /* behind*/
	{
		if (newpos + 1 == palettes.size())
			newpos = 0;
		else ++newpos;
	}
	Palette8bit* tmp = palettes[cur_palette];
	palettes[cur_palette] = palettes[newpos];
	palettes[newpos] = tmp;

	// Refresh the display to show the updated amount of palettes
	cur_palette = newpos;
	showPalette(newpos);
	setModified();
	return true;
}

/* PaletteEntryPanel::tint
 * Tints the colours of the current palette
 *******************************************************************/
bool PaletteEntryPanel::tint()
{
	Palette8bit* pal = new Palette8bit;
	if (pal == NULL) return false;
	pal->copyPalette(palettes[cur_palette]);
	PaletteTintDialog ptd(theMainWindow, pal);
	if (ptd.ShowModal() == wxID_OK)
	{
		palettes[cur_palette]->copyPalette(ptd.getFinalPalette());
		showPalette(cur_palette);
		setModified();
	}
	delete pal;
	return true;
}

/* PaletteEntryPanel::colourise
 * Colourises the colours of the current palette
 *******************************************************************/
bool PaletteEntryPanel::colourise()
{
	Palette8bit* pal = new Palette8bit;
	if (pal == NULL) return false;
	pal->copyPalette(palettes[cur_palette]);
	PaletteColouriseDialog pcd(theMainWindow, pal);
	if (pcd.ShowModal() == wxID_OK)
	{
		palettes[cur_palette]->copyPalette(pcd.getFinalPalette());
		showPalette(cur_palette);
		setModified();
	}
	delete pal;
	return true;
}

/* PaletteEntryPanel::tweak
 * Tweaks the colours of the current palette
 *******************************************************************/
bool PaletteEntryPanel::tweak()
{
	Palette8bit* pal = new Palette8bit;
	if (pal == NULL) return false;
	pal->copyPalette(palettes[cur_palette]);
	PaletteColourTweakDialog pctd(theMainWindow, pal);
	if (pctd.ShowModal() == wxID_OK)
	{
		palettes[cur_palette]->copyPalette(pctd.getFinalPalette());
		showPalette(cur_palette);
		setModified();
	}
	delete pal;
	return true;
}

/* PaletteEntryPanel::invert
 * Inverts the colours of the current palette
 *******************************************************************/
bool PaletteEntryPanel::invert()
{
	Palette8bit* pal = new Palette8bit;
	if (pal == NULL) return false;
	pal->copyPalette(palettes[cur_palette]);
	PaletteInvertDialog pid(theMainWindow, pal);
	if (pid.ShowModal() == wxID_OK)
	{
		palettes[cur_palette]->copyPalette(pid.getFinalPalette());
		showPalette(cur_palette);
		setModified();
	}
	delete pal;
	return true;
}

/* PaletteEntryPanel::gradient
 * Applies a gradient to the palette
 *******************************************************************/
bool PaletteEntryPanel::gradient()
{
	Palette8bit* pal = new Palette8bit;
	if (pal == NULL) return false;
	pal->copyPalette(palettes[cur_palette]);
	PaletteGradientDialog pgd(theMainWindow, pal);
	if (pgd.ShowModal() == wxID_OK)
	{
		palettes[cur_palette]->copyPalette(pgd.getFinalPalette());
		showPalette(cur_palette);
		setModified();
	}
	delete pal;
	return true;
}

#include "Graphics/SImage/SIFormat.h"
/* PaletteEntryPanel::generateColormaps
 * Generates a COLORMAP lump from the current palette
 *******************************************************************/
#define GREENMAP 255
#define GRAYMAP 32
#define DIMINISH(color, level) color = (uint8_t)((((float)color)*(32.0-level)+16.0)/32.0)
bool PaletteEntryPanel::generateColormaps()
{
	if (!entry || !entry->getParent() || ! palettes[0])
		return false;

	MemChunk mc;
	SImage img;
	MemChunk imc;
	mc.reSize(34*256);
	mc.seek(0, SEEK_SET);
	imc.reSize(34*256*4);
	imc.seek(0, SEEK_SET);
	uint8_t rgba[4];
	rgba[3] = 255;

	rgba_t rgb;
	float grey;
	// Generate 34 maps: the first 32 for diminishing light levels,
	// the 33th for the inverted grey map used by invulnerability.
	// The 34th colormap remains empty and black.
	for (size_t l = 0; l < 34; ++l)
	{
		for (size_t c = 0; c < 256; ++c)
		{
			rgb = palettes[0]->colour(c);
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
				grey = ((float)rgb.r/256.0 * col_greyscale_r) + ((float)rgb.g/256.0 * col_greyscale_g) + ((float)rgb.b/256.0 * col_greyscale_b);
				grey = 1.0 - grey;
				// Clamp value: with Id Software's values, the sum is greater than 1.0 (0.299+0.587+0.144=1.030)
				// This means the negation above can give a negative value (for example, with RGB values of 247 or more),
				// which will not be converted correctly to unsigned 8-bit int in the rgba_t struct.
				if (grey < 0.0) grey = 0;
				rgb.r = rgb.g = rgb.b = grey*255;
			}
			else
			{
				// Fill with 0
				rgb = palettes[0]->colour(0);
			}
			rgba[0] = rgb.r; rgba[1] = rgb.g; rgba[2] = rgb.b;
			imc.write(&rgba, 4);
			mc[(256*l)+c] = palettes[0]->nearestColour(rgb);
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
	colormap = entry->getParent()->getEntry("COLORMAP", true);
	bool preexisting = colormap != NULL;
	if (!colormap)
	{
		// We need to create this entry
		colormap = new ArchiveEntry("COLORMAP.lmp", 34*256);
	}
	if (!colormap)
		return false;
	colormap->importMemChunk(mc);
	if (!preexisting)
	{
		entry->getParent()->addEntry(colormap);
	}
	return true;
}
#undef DIMINISH

/* PaletteEntryPanel::generatePalette
 * Just a helper for generatePalettes to make the code less redundant
 *******************************************************************/
void PaletteEntryPanel::generatePalette(int r, int g, int b, int shift, int steps)
{
	// Create a new palette
	Palette8bit* pal = new Palette8bit;
	if (pal == NULL) return;

	// Seed it with the basic palette
	pal->copyPalette(palettes[0]);

	// Tint palette with given values
	pal->idtint(r, g, b, shift, steps);

	// Add it to the palette list
	palettes.push_back(pal);
}

/* PaletteEntryPanel::generatePalettes
 * Generates the full complement of palettes needed by the game
 *******************************************************************/
bool PaletteEntryPanel::generatePalettes()
{
	GeneratePalettesDialog gpd(theMainWindow);
	if (gpd.ShowModal() == wxID_OK)
	{
		// Get choice
		int choice = gpd.getChoice();
		if (choice == 0) return false;

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
		cur_palette = 0;
		showPalette(0);
		setModified();
	}
	return true;
}

/* PaletteEntryPanel::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool PaletteEntryPanel::handleAction(string id)
{
	// Ignore if hidden
	if (!isActivePanel())
		return false;

	// Only interested in "ppal_" events
	if (!id.StartsWith("ppal_"))
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

/* PaletteEntryPanel::fillCustomMenu
 * Fills the given menu with the panel's custom actions. Used by
 * both the constructor to create the main window's custom menu,
 * and ArchivePanel::onEntryListRightClick to fill the context
 * menu with context-appropriate stuff
 *******************************************************************/
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




//  Just some reverse-engineering stuff.
#define GPALCOMPANALYSIS
#define PALETTECHECK 1
void PaletteEntryPanel::analysePalettes()
{
	if (palettes.size() < PALETTECHECK + 1)
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
	string report = "\n";
#ifdef GPALCOMPANALYSIS
	int i = PALETTECHECK;
	if (i)
	{
		report += S_FMT("Deviation between palettes 0 and %i:\n\n", i);
		devR = devG = devB = 0;
		maxR = maxG = maxB = -1;
		minR = minG = minB = 256;
		wrongcount = 0;
#else
	report += S_FMT("Changes between %u palettes compared to the first:\n\n", palettes.size());
	for (size_t i = 1; i < palettes.size(); ++i)
	{
		for (int j = 0; j < 256; ++j)
			reds[j] = blues[j] = greens[j] = 999;
#endif
		report += S_FMT("\n==============\n= Palette %02u =\n==============\n", i);
		for (size_t c = 0; c < 256; ++c)
		{
			rgba_t ref1 = palettes[0]->colour(c);
			rgba_t cmp1 = palettes[i]->colour(c);
			hsl_t  ref2 = Misc::rgbToHsl(ref1);
			hsl_t  cmp2 = Misc::rgbToHsl(cmp1);
#ifdef GPALCOMPANALYSIS
			int r, g, b;
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
			if (r > maxR) maxR = r; if (r < minR) minR = r;
			if (g > maxG) maxG = g; if (g < minG) minG = g;
			if (b > maxB) maxB = b; if (b < minB) minB = b;
			if (r | g | b)
			{
				++wrongcount;
				report += S_FMT("Index %003u: [%003i %003i %003i | %1.3f %1.3f %1.3f]->[%003i %003i %003i | %1.3f %1.3f %1.3f]\t\tR %+003i\tG %+003i\tB %+003i\t\t\tH %+1.3f\tS %+1.3f\tL %+1.3f\n",
				                c,
				                ref1.r, ref1.g, ref1.b, ref2.h, ref2.s, ref2.l,
				                cmp1.r, cmp1.g, cmp1.b, cmp2.h, cmp2.s, cmp2.l,
				                r, g, b, h, s, l);
			}
#else
			if (reds[ref1.r] != cmp1.r && reds[ref1.r] != 999)
				DPrintf("Discrepency for red channel at index %i, value %i: %d vs. %d set before",
				        c, ref1.r, cmp1.r, reds[ref1.r]);
			if (greens[ref1.g] != cmp1.g && greens[ref1.g] != 999)
				DPrintf("Discrepency for green channel at index %i, value %i: %d vs. %d set before",
				        c, ref1.g, cmp1.g, greens[ref1.g]);
			if (blues[ref1.b] != cmp1.b && blues[ref1.b] != 999)
				DPrintf("Discrepency for blue channel at index %i, value %i: %d vs. %d set before",
				        c, ref1.b, cmp1.b, blues[ref1.b]);
			reds[ref1.r] = cmp1.r;
			greens[ref1.g] = cmp1.g;
			blues[ref1.b] = cmp1.b;
#endif
		}
#ifdef GPALCOMPANALYSIS
		report += S_FMT("Deviation sigma: R %+003i G %+003i B %+003i\t%s\n", devR, devG, devB, entry->getName(true));
		report += S_FMT("Min R %+003i Min G %+003i Min B %+003i Max R %+003i Max G %+003i Max B %+003i \nError count: %i\n",
		                minR, minG, minB, maxR, maxG, maxB, wrongcount);
#else
		report += "Shift table for existing channel values:\n|  I  |  R  |  G  |  B  |\n";
		for (size_t i = 0; i < 256; ++i)
		{
			if (reds[i] < 999 || greens[i] < 999 || blues[i] < 999)
				report += S_FMT("| %003d | %003d | %003d | %003d |\n", i, reds[i], greens[i], blues[i]);
		}
		report.Replace("999", "   ");
#endif
	}

	LOG_MESSAGE(1, report);

}

/*******************************************************************
 * PALETTEENTRYPANEL CLASS EVENTS
 *******************************************************************/

/* PaletteEntryPanel::onPalCanvasMouseEvent
 * Called when a mouse event happens within the palette canvas (eg.
 * button clicked, pointer moved, etc)
 *******************************************************************/
void PaletteEntryPanel::onPalCanvasMouseEvent(wxMouseEvent& e)
{
	// Update colour info label with selected colour (if any)
	if (e.LeftDown())
	{
		// Send to palette canvas
		pal_canvas->onMouseLeftDown(e);

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
		pal_canvas->onMouseLeftDown(e);
		int sel = pal_canvas->getSelectionStart();

		// There actually was a colour selected
		if (sel > -1)
		{
			rgba_t col = pal_canvas->getSelectedColour();
			// Open a colour dialog
			wxColour cd = wxGetColourFromUser(GetParent(), WXCOL(col));

			if (cd.Ok())
			{
				col.r = cd.Red();
				col.g = cd.Green();
				col.b = cd.Blue();

				palettes[cur_palette]->setColour(sel, col);
				setModified();
				showPalette(cur_palette);
			}
		}
	}
}
