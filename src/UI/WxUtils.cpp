
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    WxUtils.cpp
// Description: WxWidgets-related utility functions
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "UI/WxUtils.h"
#include "Graphics/Icons.h"
#include "General/UI.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
CVAR(String, font_monospace, "Consolas,Lucida Console", CVAR_SAVE)
CVAR(Int, tab_style, 1, CVAR_SAVE)


// ----------------------------------------------------------------------------
//
// WxUtils Namespace Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// WxUtils::createMenuItem
//
// Creates a wxMenuItem from the given parameters, including giving it an icon
// from slade.pk3 if specified
// ----------------------------------------------------------------------------
wxMenuItem* WxUtils::createMenuItem(wxMenu* menu, int id, const string& label, const string& help, const string& icon)
{
	auto item = new wxMenuItem(menu, id, label, help);

	if (!icon.IsEmpty())
		item->SetBitmap(Icons::getIcon(-1, icon));

	return item;
}

// ----------------------------------------------------------------------------
// WxUtils::getMonospaceFont
//
// Returns a copy of the font [base] with a monospace face/family
// ----------------------------------------------------------------------------
wxFont WxUtils::getMonospaceFont(wxFont base)
{
	wxFont font(base.GetPointSize(), wxFONTFAMILY_MODERN, base.GetStyle(), base.GetWeight());

	auto split = wxSplit(font_monospace, ',');
	for (unsigned a = 0; a < split.size(); a++)
	{
		if (base.SetFaceName(split[a]))
		{
			font.SetFaceName(split[a]);
			break;
		}
	}

	return font;
}

// ----------------------------------------------------------------------------
// WxUtils::createSmallImageList
//
// Creates an image list with a DPI-scaled 'small' image size (for lists, etc.)
// ----------------------------------------------------------------------------
wxImageList* WxUtils::createSmallImageList()
{
	auto icon_size = UI::scaleFactor() > 1.25 ? 32 : 16;
	return new wxImageList(icon_size, icon_size, false, 0);
}

// ----------------------------------------------------------------------------
// WxUtils::createPadPanel
//
// Creates a wxPanel and places [control] on it, with [pad] padding around it
// ----------------------------------------------------------------------------
wxPanel* WxUtils::createPadPanel(wxWindow* parent, wxWindow* control, int pad)
{
	if (pad < 0)
		pad = UI::pad();

	auto panel = new wxPanel(parent);
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	panel->GetSizer()->Add(control, 1, wxEXPAND | wxALL, pad);
	control->Reparent(panel);

	return panel;
}

// ----------------------------------------------------------------------------
// WxUtils::createLabelHBox
//
// Creates a simple horizontal box sizer with a [label] on the left and
// [widget] on the right
// ----------------------------------------------------------------------------
wxSizer* WxUtils::createLabelHBox(wxWindow* parent, const string& label, wxWindow* widget)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(parent, -1, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(widget, 1, wxEXPAND);
	return hbox;
}

wxSizer* WxUtils::createLabelHBox(wxWindow* parent, const string& label, wxSizer* sizer)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(parent, -1, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(sizer, 1, wxEXPAND);
	return hbox;
}

// ----------------------------------------------------------------------------
// WxUtils::createLabelVBox
//
// Creates a simple vertical box sizer with a [label] on the top and [widget]
// on the bottom
// ----------------------------------------------------------------------------
wxSizer *::WxUtils::createLabelVBox(wxWindow *parent, const string &label, wxWindow *widget)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(new wxStaticText(parent, -1, label), 0, wxBOTTOM, UI::px(UI::Size::PadMinimum));
	vbox->Add(widget, 1, wxEXPAND);
	return vbox;
}

wxSizer* WxUtils::createLabelVBox(wxWindow* parent, const string& label, wxSizer* sizer)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(new wxStaticText(parent, -1, label), 0, wxBOTTOM, UI::px(UI::Size::PadMinimum));
	vbox->Add(sizer, 1, wxEXPAND);
	return vbox;
}

// ----------------------------------------------------------------------------
// WxUtils::layoutHorizontally
//
// Returns a horizontal box sizer containing [widgets].
// [widgets] can contain a combination of wxWindow and wxSizer objects
// ----------------------------------------------------------------------------
wxSizer* WxUtils::layoutHorizontally(vector<wxObject*> widgets, int expand_col)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);

	// Add widgets/sizers
	for (auto a = 0u; a < widgets.size(); ++a)
	{
		auto widget = widgets[a];

		// Widget
		if (widget->IsKindOf(&wxWindow::ms_classInfo))
		{
			hbox->Add(
				(wxWindow*)widget,
				expand_col == a ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxLEFT,
				UI::pad()
			);
		}

		// Sizer
		else if (widget->IsKindOf(&wxSizer::ms_classInfo))
		{
			hbox->Add(
				(wxSizer*)widget,
				expand_col == a ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxLEFT,
				UI::pad()
			);
		}
	}

	return hbox;
}

// ----------------------------------------------------------------------------
// WxUtils::layoutHorizontally
//
// Same as above, however instead of returning a new sizer, it adds it to the
// given [sizer] with [flags]
// ----------------------------------------------------------------------------
void WxUtils::layoutHorizontally(wxSizer* sizer, vector<wxObject*> widgets, wxSizerFlags flags, int expand_col)
{
	sizer->Add(layoutHorizontally(widgets, expand_col), flags);
}

// ----------------------------------------------------------------------------
// WxUtils::layoutVertically
//
// Returns a vertical box sizer containing [widgets].
// [widgets] can contain a combination of wxWindow and wxSizer objects
// ----------------------------------------------------------------------------
wxSizer* WxUtils::layoutVertically(vector<wxObject*> widgets, int expand_row)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);

	// Add widgets/sizers
	for (auto a = 0u; a < widgets.size(); ++a)
	{
		auto widget = widgets[a];

		// Widget
		if (widget->IsKindOf(&wxWindow::ms_classInfo))
		{
			vbox->Add(
				(wxWindow*)widget,
				expand_row == a ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxTOP,
				UI::pad()
			);
		}

		// Sizer
		else if (widget->IsKindOf(&wxSizer::ms_classInfo))
		{
			vbox->Add(
				(wxSizer*)widget,
				expand_row == a ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxTOP,
				UI::pad()
			);
		}
	}

	return vbox;
}

// ----------------------------------------------------------------------------
// WxUtils::layoutVertically
//
// Same as above, however instead of returning a new sizer, it adds it to the
// given [sizer] with [flags]
// ----------------------------------------------------------------------------
void WxUtils::layoutVertically(wxSizer* sizer, vector<wxObject*> widgets, wxSizerFlags flags, int expand_row)
{
	sizer->Add(layoutVertically(widgets, expand_row), flags);
}

// ----------------------------------------------------------------------------
// WxUtils::arrayString
//
// Returns a wxArrayString containing the strings in [vector]
// ----------------------------------------------------------------------------
wxArrayString WxUtils::arrayString(vector<string> vector)
{
	return wxArrayString{ vector.size(), vector.data() };
}

wxSize WxUtils::scaledSize(int x, int y)
{
	return wxSize(
		x < 0 ? -1 : UI::scalePx(x),
		y < 0 ? -1 : UI::scalePx(y)
	);
}

wxPoint WxUtils::scaledPoint(int x, int y)
{
	return wxPoint(UI::scalePx(x), UI::scalePx(y));
}

wxRect WxUtils::scaledRect(int x, int y, int width, int height)
{
	return wxRect(UI::scalePx(x), UI::scalePx(y), UI::scalePx(width), UI::scalePx(height));
}
