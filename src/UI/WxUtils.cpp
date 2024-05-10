
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "UI/WxUtils.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "Utility/Colour.h"
#include "thirdparty/lunasvg/include/lunasvg.h"

#ifdef __WXGTK3__
#include <gtk-3.0/gtk/gtk.h>
#elif __WXGTK20__
#define GSocket GlibGSocket
#include <gtk-2.0/gtk/gtk.h>
#undef GSocket
#endif

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, font_monospace, "Consolas,Lucida Console", CVar::Flag::Save)
CVAR(Int, tab_style, 1, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// WxUtils Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Creates a wxMenuItem from the given parameters, including giving it an icon
// from slade.pk3 if specified
// -----------------------------------------------------------------------------
wxMenuItem* wxutil::createMenuItem(
	wxMenu*         menu,
	int             id,
	const wxString& label,
	const wxString& help,
	const wxString& icon)
{
	auto item = new wxMenuItem(menu, id, label, help);

	if (!icon.IsEmpty())
		item->SetBitmap(icons::getIcon(icons::Any, icon.ToStdString()));

	return item;
}

// -----------------------------------------------------------------------------
// Returns a copy of the font [base] with a monospace face/family
// -----------------------------------------------------------------------------
wxFont wxutil::monospaceFont(wxFont base)
{
	wxFont font(base.GetPointSize(), wxFONTFAMILY_MODERN, base.GetStyle(), base.GetWeight());

	auto split = wxSplit(font_monospace, ',');
	for (const auto& name : split)
	{
		if (base.SetFaceName(name))
		{
			font.SetFaceName(name);
			break;
		}
	}

	return font;
}

// -----------------------------------------------------------------------------
// Creates an image list with a DPI-scaled 'small' image size (for lists, etc.)
// -----------------------------------------------------------------------------
wxImageList* wxutil::createSmallImageList()
{
	const auto icon_size = ui::scalePx(16);
	return new wxImageList(icon_size, icon_size, false, 0);
}

// -----------------------------------------------------------------------------
// Adds [icon] of [icon_type] to the given image [list]
// -----------------------------------------------------------------------------
int wxutil::addImageListIcon(wxImageList* list, int icon_type, string_view icon)
{
#if wxCHECK_VERSION(3, 1, 6)
	return list->Add(icons::getIcon(static_cast<icons::Type>(icon_type), icon).GetBitmap(list->GetSize()));
#else
	return list->Add(icons::getIcon(static_cast<icons::Type>(icon_type), icon));
#endif
}

// -----------------------------------------------------------------------------
// Creates a wxPanel and places [control] on it, with [pad] padding around it
// -----------------------------------------------------------------------------
wxPanel* wxutil::createPadPanel(wxWindow* parent, wxWindow* control, int pad)
{
	if (pad < 0)
		pad = ui::pad();

	auto panel = new wxPanel(parent);
	control->Reparent(panel);
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	panel->GetSizer()->Add(control, 1, wxEXPAND | wxALL, pad);

	return panel;
}

// -----------------------------------------------------------------------------
// Creates a wxSpinCtrl using the SLADE sizing for spin controls
// -----------------------------------------------------------------------------
wxSpinCtrl* wxutil::createSpinCtrl(wxWindow* parent, int value, int min, int max)
{
	return new wxSpinCtrl(
		parent, -1, "", wxDefaultPosition, { ui::px(ui::Size::SpinCtrlWidth), -1 }, wxSP_ARROW_KEYS, min, max, value);
}

// -----------------------------------------------------------------------------
// Creates a simple horizontal box sizer with a [label] on the left and
// [widget] on the right
// -----------------------------------------------------------------------------
wxSizer* wxutil::createLabelHBox(wxWindow* parent, const wxString& label, wxWindow* widget)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(parent, -1, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, ui::pad());
	hbox->Add(widget, 1, wxEXPAND);
	return hbox;
}

wxSizer* wxutil::createLabelHBox(wxWindow* parent, const wxString& label, wxSizer* sizer)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(parent, -1, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, ui::pad());
	hbox->Add(sizer, 1, wxEXPAND);
	return hbox;
}

// -----------------------------------------------------------------------------
// Creates a simple vertical box sizer with a [label] on the top and [widget]
// on the bottom
// -----------------------------------------------------------------------------
wxSizer* ::wxutil::createLabelVBox(wxWindow* parent, const wxString& label, wxWindow* widget)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(new wxStaticText(parent, -1, label), 0, wxBOTTOM, ui::px(ui::Size::PadMinimum));
	vbox->Add(widget, 1, wxEXPAND);
	return vbox;
}

wxSizer* wxutil::createLabelVBox(wxWindow* parent, const wxString& label, wxSizer* sizer)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(new wxStaticText(parent, -1, label), 0, wxBOTTOM, ui::px(ui::Size::PadMinimum));
	vbox->Add(sizer, 1, wxEXPAND);
	return vbox;
}

// -----------------------------------------------------------------------------
// Creates a horizontal sizer with the given ok/cancel buttons on the right,
// ordered in the standard order for the platform:
// Windows: [btn_ok] [btn_cancel]
// Others:  [btn_cancel] [btn_ok]
// -----------------------------------------------------------------------------
wxSizer* wxutil::createDialogButtonBox(wxButton* btn_ok, wxButton* btn_cancel)
{
	auto* hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->AddStretchSpacer(1);

#ifdef __WXMSW__
	hbox->Add(btn_ok, 0, wxEXPAND | wxRIGHT, ui::pad());
	hbox->Add(btn_cancel, 0, wxEXPAND);
#else
	hbox->Add(btn_cancel, 0, wxEXPAND | wxRIGHT, ui::pad());
	hbox->Add(btn_ok, 0, wxEXPAND);
#endif

	return hbox;
}

// -----------------------------------------------------------------------------
// Shortcut function for createDialogButtonBox that creates ok/cancel buttons
// with the given [text_ok] and [text_cancel]
// -----------------------------------------------------------------------------
wxSizer* slade::wxutil::createDialogButtonBox(wxWindow* parent, const wxString& text_ok, const wxString& text_cancel)
{
	auto* btn_ok = new wxButton(parent, wxID_OK, text_ok);
	btn_ok->SetDefault();
	auto* btn_cancel = new wxButton(parent, wxID_CANCEL, text_cancel);
	return createDialogButtonBox(btn_ok, btn_cancel);
}

// -----------------------------------------------------------------------------
// Returns a horizontal box sizer containing [widgets].
// [widgets] can contain a combination of wxWindow and wxSizer objects
// -----------------------------------------------------------------------------
wxSizer* wxutil::layoutHorizontally(const vector<wxObject*>& widgets, int expand_col)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);

	// Add widgets/sizers
	for (auto a = 0u; a < widgets.size(); ++a)
	{
		const auto widget = widgets[a];

		// Widget
		if (widget->IsKindOf(&wxWindow::ms_classInfo))
		{
			hbox->Add(
				dynamic_cast<wxWindow*>(widget),
				expand_col == static_cast<int>(a) ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxLEFT,
				ui::pad());
		}

		// Sizer
		else if (widget->IsKindOf(&wxSizer::ms_classInfo))
		{
			hbox->Add(
				dynamic_cast<wxSizer*>(widget),
				expand_col == static_cast<int>(a) ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxLEFT,
				ui::pad());
		}
	}

	return hbox;
}

// -----------------------------------------------------------------------------
// Same as above, however instead of returning a new sizer, it adds it to the
// given [sizer] with [flags]
// -----------------------------------------------------------------------------
void wxutil::layoutHorizontally(wxSizer* sizer, const vector<wxObject*>& widgets, wxSizerFlags flags, int expand_col)
{
	sizer->Add(layoutHorizontally(widgets, expand_col), flags);
}

// -----------------------------------------------------------------------------
// Returns a vertical box sizer containing [widgets].
// [widgets] can contain a combination of wxWindow and wxSizer objects
// -----------------------------------------------------------------------------
wxSizer* wxutil::layoutVertically(const vector<wxObject*>& widgets, int expand_row)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);

	// Add widgets/sizers
	for (auto a = 0u; a < widgets.size(); ++a)
	{
		const auto widget = widgets[a];

		// Widget
		if (widget->IsKindOf(&wxWindow::ms_classInfo))
		{
			vbox->Add(
				dynamic_cast<wxWindow*>(widget),
				expand_row == static_cast<int>(a) ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxTOP,
				ui::pad());
		}

		// Sizer
		else if (widget->IsKindOf(&wxSizer::ms_classInfo))
		{
			vbox->Add(
				dynamic_cast<wxSizer*>(widget),
				expand_row == static_cast<int>(a) ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxTOP,
				ui::pad());
		}
	}

	return vbox;
}

// -----------------------------------------------------------------------------
// Same as above, however instead of returning a new sizer, it adds it to the
// given [sizer] with [flags]
// -----------------------------------------------------------------------------
void wxutil::layoutVertically(wxSizer* sizer, const vector<wxObject*>& widgets, wxSizerFlags flags, int expand_row)
{
	sizer->Add(layoutVertically(widgets, expand_row), flags);
}

// -----------------------------------------------------------------------------
// Returns a wxSizerFlags of [proportion], with a border at [direction] of
// [size].
// If [size] is negative, uses the default padding size (ui::pad()).
// Equivalent to wxSizerFlags([proportion]).Border([direction], [size])
// -----------------------------------------------------------------------------
wxSizerFlags wxutil::sfWithBorder(int proportion, int direction, int size)
{
	if (size < 0)
		size = ui::pad();

	return wxSizerFlags(proportion).Border(direction, size);
}

// -----------------------------------------------------------------------------
// Returns a wxSizerFlags of [proportion], with a large border at [direction].
// Equivalent to wxSizerFlags([proportion]).Border([direction], ui::padLarge())
// -----------------------------------------------------------------------------
wxSizerFlags wxutil::sfWithLargeBorder(int proportion, int direction)
{
	return wxSizerFlags(proportion).Border(direction, ui::padLarge());
}

// -----------------------------------------------------------------------------
// Returns a wxSizerFlags of [proportion], with a small border at [direction].
// Equivalent to wxSizerFlags([proportion]).Border([direction], ui::padMin())
// -----------------------------------------------------------------------------
wxSizerFlags wxutil::sfWithMinBorder(int proportion, int direction)
{
	return wxSizerFlags(proportion).Border(direction, ui::padMin());
}

// -----------------------------------------------------------------------------
// Returns a wxArrayString containing the (wx) strings in [vector]
// -----------------------------------------------------------------------------
wxArrayString wxutil::arrayString(const vector<wxString>& vector)
{
	return wxArrayString{ vector.size(), vector.data() };
}

// -----------------------------------------------------------------------------
// Returns a wxArrayString containing the (std) strings in [vector]
// -----------------------------------------------------------------------------
wxArrayString wxutil::arrayStringStd(const vector<string>& vector)
{
	wxArrayString list;
	for (const auto& str : vector)
		list.Add(str);
	return list;
}

// -----------------------------------------------------------------------------
// Returns the size [x]x[y] as a wxSize, scaled by the DPI factor.
// Will keep any -1 value
// -----------------------------------------------------------------------------
wxSize wxutil::scaledSize(int x, int y)
{
	return { x < 0 ? -1 : ui::scalePx(x), y < 0 ? -1 : ui::scalePx(y) };
}

// -----------------------------------------------------------------------------
// Returns the position [x],[y] as a wxPoint, scaled by the DPI factor
// -----------------------------------------------------------------------------
wxPoint wxutil::scaledPoint(int x, int y)
{
	return { ui::scalePx(x), ui::scalePx(y) };
}

// -----------------------------------------------------------------------------
// Returns the rect at [x],[y] of size [width]x[height] as a wxRect, scaled by
// the DPI factor
// -----------------------------------------------------------------------------
wxRect wxutil::scaledRect(int x, int y, int width, int height)
{
	return { ui::scalePx(x), ui::scalePx(y), ui::scalePx(width), ui::scalePx(height) };
}

// -----------------------------------------------------------------------------
// Sets the given [window]'s [icon]
// -----------------------------------------------------------------------------
void wxutil::setWindowIcon(wxTopLevelWindow* window, string_view icon)
{
#if wxCHECK_VERSION(3, 1, 6)
	auto wx_icon = icons::getIcon(icons::General, icon).GetIconFor(window);
#else
	wxIcon wx_icon;
	wx_icon.CopyFromBitmap(icons::getIcon(icons::General, icon));
#endif

	window->SetIcon(wx_icon);
}


// The following functions are taken from CodeLite (http://codelite.org)

wxColour wxutil::systemPanelBGColour()
{
#ifdef __WXGTK__
	static bool     intitialized(false);
	static wxColour bgColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));

	if (!intitialized)
	{
		// try to get the background colour from a menu
		GtkWidget* menu = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		GtkStyle*  def  = gtk_rc_get_style(menu);
		if (!def)
			def = gtk_widget_get_default_style();

		if (def)
		{
			GdkColor col = def->bg[GTK_STATE_NORMAL];
			bgColour     = wxColour(col);
		}
		gtk_widget_destroy(menu);
		intitialized = true;
	}
	return bgColour;
#else
	return wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
#endif
}

wxColour wxutil::systemMenuTextColour()
{
	return wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT);
}

wxColour wxutil::systemMenuBarBGColour()
{
	return wxSystemSettings::GetColour(wxSYS_COLOUR_MENU);
}

wxColour wxutil::lightColour(const wxColour& colour, float percent)
{
	if (percent == 0)
	{
		return colour;
	}

	// Convert to HSL
	ColHSL hsl = colour::rgbToHsl(ColRGBA(colour));

	// Increase luminance
	hsl.l += static_cast<float>((percent * 5.0) / 100.0);
	if (hsl.l > 1.0)
		hsl.l = 1.0;

	ColRGBA rgb = hsl.asRGB();
	return { rgb.r, rgb.g, rgb.b };
}

wxColour wxutil::darkColour(const wxColour& colour, float percent)
{
	if (percent == 0)
	{
		return colour;
	}

	// Convert to HSL
	ColHSL hsl = colour::rgbToHsl(ColRGBA(colour));

	// Decrease luminance
	hsl.l -= static_cast<float>((percent * 5.0) / 100.0);
	if (hsl.l < 0)
		hsl.l = 0;

	ColRGBA rgb = hsl.asRGB();
	return { rgb.r, rgb.g, rgb.b };
}
