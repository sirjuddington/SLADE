
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LayoutHelper.cpp
// Description: Struct for helping with layout of wxWidgets controls
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
#include "Layout.h"
#include "UI/UI.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
// Returns a horizontal box sizer containing [widgets].
// [widgets] can contain a combination of wxWindow and wxSizer objects
// -----------------------------------------------------------------------------
wxSizer* LayoutHelper::layoutHorizontally(const vector<wxObject*>& widgets, int expand_col) const
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
				window->FromDIP(ui::pad()));
		}

		// Sizer
		else if (widget->IsKindOf(&wxSizer::ms_classInfo))
		{
			hbox->Add(
				dynamic_cast<wxSizer*>(widget),
				expand_col == static_cast<int>(a) ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxLEFT,
				window->FromDIP(ui::pad()));
		}
	}

	return hbox;
}

// -----------------------------------------------------------------------------
// Overload of layoutHorizontally which, instead of returning the new sizer,
// adds it to the given [sizer] with [flags]
// -----------------------------------------------------------------------------
void LayoutHelper::layoutHorizontally(
	wxSizer*                 sizer,
	const vector<wxObject*>& widgets,
	wxSizerFlags             flags,
	int                      expand_col) const
{
	sizer->Add(layoutHorizontally(widgets, expand_col), flags);
}

// -----------------------------------------------------------------------------
// Returns a vertical box sizer containing [widgets].
// [widgets] can contain a combination of wxWindow and wxSizer objects
// -----------------------------------------------------------------------------
wxSizer* LayoutHelper::layoutVertically(const vector<wxObject*>& widgets, int expand_row) const
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
				window->FromDIP(ui::pad()));
		}

		// Sizer
		else if (widget->IsKindOf(&wxSizer::ms_classInfo))
		{
			vbox->Add(
				dynamic_cast<wxSizer*>(widget),
				expand_row == static_cast<int>(a) ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxTOP,
				window->FromDIP(ui::pad()));
		}
	}

	return vbox;
}

// -----------------------------------------------------------------------------
// Overload of layoutVertically which, instead of returning the new sizer,
// adds it to the given [sizer] with [flags]
// -----------------------------------------------------------------------------
void LayoutHelper::layoutVertically(
	wxSizer*                 sizer,
	const vector<wxObject*>& widgets,
	wxSizerFlags             flags,
	int                      expand_row) const
{
	sizer->Add(layoutVertically(widgets, expand_row), flags);
}

// -----------------------------------------------------------------------------
// Returns a wxSizerFlags of [proportion], with a border at [direction] of
// [size].
// If [size] is negative, uses the default padding size.
// Equivalent to wxSizerFlags([proportion]).Border([direction], [size])
// -----------------------------------------------------------------------------
wxSizerFlags LayoutHelper::sfWithBorder(int proportion, int direction, int size) const
{
	return wxSizerFlags(proportion).Border(direction, window->FromDIP(size < 0 ? ui::pad() : size));
}

// -----------------------------------------------------------------------------
// Returns a wxSizerFlags of [proportion], with a large border at [direction].
// Equivalent to wxSizerFlags([proportion]).Border([direction], ui::padLarge())
// -----------------------------------------------------------------------------
wxSizerFlags LayoutHelper::sfWithLargeBorder(int proportion, int direction) const
{
	return wxSizerFlags(proportion).Border(direction, window->FromDIP(ui::padLarge()));
}

// -----------------------------------------------------------------------------
// Returns a wxSizerFlags of [proportion], with a large border at [direction].
// Equivalent to wxSizerFlags([proportion]).Border([direction], ui::padXLarge())
// -----------------------------------------------------------------------------
wxSizerFlags LayoutHelper::sfWithXLargeBorder(int proportion, int direction) const
{
	return wxSizerFlags(proportion).Border(direction, window->FromDIP(ui::padXLarge()));
}

// -----------------------------------------------------------------------------
// Returns a wxSizerFlags of [proportion], with a small border at [direction].
// Equivalent to wxSizerFlags([proportion]).Border([direction], ui::padSmall())
// -----------------------------------------------------------------------------
wxSizerFlags LayoutHelper::sfWithSmallBorder(int proportion, int direction) const
{
	return wxSizerFlags(proportion).Border(direction, window->FromDIP(ui::padSmall()));
}

// -----------------------------------------------------------------------------
// Converts a size in pixels to DIP
// -----------------------------------------------------------------------------
int LayoutHelper::px(int size) const
{
	return window->FromDIP(size);
}

// -----------------------------------------------------------------------------
// Converts a wxSize in pixels to DIP
// -----------------------------------------------------------------------------
wxSize LayoutHelper::size(int width, int height) const
{
	return window->FromDIP(wxSize(width, height));
}

// -----------------------------------------------------------------------------
// Converts a wxPoint in pixels to DIP
// -----------------------------------------------------------------------------
wxPoint LayoutHelper::point(int x, int y) const
{
	return window->FromDIP(wxPoint(x, y));
}

// -----------------------------------------------------------------------------
// Converts a wxRect in pixels to DIP
// -----------------------------------------------------------------------------
wxRect LayoutHelper::rect(int x, int y, int width, int height) const
{
	return { window->FromDIP(wxPoint(x, y)), window->FromDIP(wxSize(width, height)) };
}

// -----------------------------------------------------------------------------
// Returns the default padding size in DIP
// -----------------------------------------------------------------------------
int LayoutHelper::pad() const
{
	return ui::pad(window);
}

// -----------------------------------------------------------------------------
// Returns the large padding size in DIP
// -----------------------------------------------------------------------------
int LayoutHelper::padLarge() const
{
	return ui::padLarge(window);
}

// -----------------------------------------------------------------------------
// Returns the extra large padding size in DIP
// -----------------------------------------------------------------------------
int LayoutHelper::padXLarge() const
{
	return ui::padXLarge(window);
}

// -----------------------------------------------------------------------------
// Returns the small padding size in DIP
// -----------------------------------------------------------------------------
int LayoutHelper::padSmall() const
{
	return ui::padSmall(window);
}

// -----------------------------------------------------------------------------
// Returns the default size for a spin control in DIP
// -----------------------------------------------------------------------------
wxSize LayoutHelper::spinSize() const
{
	return window->FromDIP(wxSize(sizePx(Size::SpinCtrlWidth), -1));
}

// -----------------------------------------------------------------------------
// Returns the default size for a slider control in DIP
// -----------------------------------------------------------------------------
wxSize LayoutHelper::sliderSize() const
{
	return window->FromDIP(wxSize(sizePx(Size::SliderWidth), -1));
}
