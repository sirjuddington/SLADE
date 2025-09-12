
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SAuiToolBarArt.cpp
// Description:
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
#include "SAuiToolBarArt.h"
#include "App.h"
#include "Graphics/Icons.h"
#include "SAuiToolBar.h"
#include <wx/dcgraph.h>

using namespace slade;


namespace
{
wxBitmap disabledBitmap(const wxBitmap& bmp, const wxColour& background)
{
#if wxCHECK_VERSION(3, 3, 0)
	return bmp.ConvertToDisabled(255 * background.GetLuminance());
#else
	return bmp.CreateDisabled();
#endif
}
} // namespace


// -----------------------------------------------------------------------------
//
// SAuiToolBarArt Class Functions
//
// -----------------------------------------------------------------------------

int SAuiToolBarArt::GetElementSize(int elementId)
{
	switch (elementId)
	{
	case wxAUI_TBART_GRIPPER_SIZE: return 0;
	default:                       return wxAuiGenericToolBarArt::GetElementSize(elementId);
	}
}

wxSize SAuiToolBarArt::GetToolSize(wxReadOnlyDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item)
{
	auto s_item = toolbar_ ? toolbar_->itemByWxId(item.GetId()) : nullptr;

	if (s_item && s_item->show_text)
	{
		// Below is just copied from the wxAuiGenericToolBarArt implementation,
		// but acting as if the wxAUI_TB_HORZ_TEXT is set

		const wxBitmap& bmp = item.GetBitmapBundle().GetBitmapFor(wnd);
		if (!bmp.IsOk() && !(m_flags & wxAUI_TB_TEXT))
			return wnd->FromDIP(wxSize(16, 16));

		int width  = bmp.IsOk() ? bmp.GetLogicalWidth() : 0;
		int height = bmp.IsOk() ? bmp.GetLogicalHeight() : 0;

		dc.SetFont(m_font);
		int tx, ty;

		width += wnd->FromDIP(3); // space between left border and bitmap
		width += wnd->FromDIP(3); // space between bitmap and text

		if (!item.GetLabel().empty())
		{
			dc.GetTextExtent(item.GetLabel(), &tx, &ty);
			width += tx;
			height = wxMax(height, ty);
		}

		// if the tool has a dropdown button, add it to the width
		// and add some extra space in front of the drop down button
		if (item.HasDropDown())
		{
#if wxCHECK_VERSION(3, 3, 0)
			int dropdownWidth = GetElementSizeForWindow(wxAUI_TBART_DROPDOWN_SIZE, wnd);
#else
			int dropdownWidth = GetElementSize(wxAUI_TBART_DROPDOWN_SIZE);
#endif
			width += dropdownWidth + wnd->FromDIP(4);
		}

		return wxSize(width, height);
	}

	return wxAuiGenericToolBarArt::GetToolSize(dc, wnd, item);
}

void SAuiToolBarArt::DrawPlainBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect)
{
#ifdef __WXMSW__
	dc.SetBrush(wnd->GetBackgroundColour());
#else
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
#endif
	dc.SetPen(*wxTRANSPARENT_PEN);

	dc.DrawRectangle(rect);

#ifdef __WXMSW__
	if (main_toolbar_)
	{
		dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
		auto right  = rect.x + rect.width;
		auto bottom = rect.y + rect.height - 1;
		dc.DrawLine(rect.x, rect.y, right, rect.y);
		dc.DrawLine(rect.x, bottom, right, bottom);
	}
#endif
}

void SAuiToolBarArt::DrawButton(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect)
{
	auto s_item         = toolbar_ ? toolbar_->itemByWxId(item.GetId()) : nullptr;
	auto item_show_text = s_item && s_item->show_text;
	int  text_width = 0, text_height = 0;

	// Determine text size if it's being drawn
	if (m_flags & wxAUI_TB_TEXT || item_show_text)
	{
		dc.SetFont(m_font);

		int tx, ty;
		dc.GetTextExtent(wxT("ABCDHgj"), &tx, &text_height);
		dc.GetTextExtent(item.GetLabel(), &text_width, &ty);
	}

	// Determine icon (and text) position and size
	int             bmp_x = 0, bmp_y = 0;
	int             text_x = 0, text_y = 0;
	const wxBitmap& bmp      = item.GetBitmapBundle().GetBitmapFor(wnd);
	const wxSize    bmp_size = bmp.IsOk() ? bmp.GetLogicalSize() : wxSize(0, 0);
	if (m_textOrientation == wxAUI_TBTOOL_TEXT_BOTTOM)
	{
		bmp_x = rect.x + (rect.width / 2) - (bmp_size.x / 2);
		bmp_y = rect.y + ((rect.height - text_height) / 2) - (bmp_size.y / 2);

		text_x = rect.x + (rect.width / 2) - (text_width / 2) + 1;
		text_y = rect.y + rect.height - text_height - 1;
	}
	if (m_textOrientation == wxAUI_TBTOOL_TEXT_RIGHT || item_show_text)
	{
		bmp_x = rect.x + wnd->FromDIP(3);
		bmp_y = rect.y + (rect.height / 2) - (bmp_size.y / 2);

		text_x = bmp_x + wnd->FromDIP(3) + bmp_size.x;
		text_y = rect.y + (rect.height / 2) - (text_height / 2);
	}

	// Get colours
	auto col_hilight    = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	auto col_background = wnd->GetBackgroundColour();

	// Draw button background
	bool checked = item.GetState() & wxAUI_BUTTON_STATE_CHECKED;
	bool hover   = item.GetState() & wxAUI_BUTTON_STATE_HOVER;
	bool pressed = item.GetState() & wxAUI_BUTTON_STATE_PRESSED;
	if (!(item.GetState() & wxAUI_BUTTON_STATE_DISABLED))
	{
		// Create buffer bitmap for background since we want to use a
		// wxGraphicsContext to draw it for better looking round edges
		wxBitmap bmp_buffer;
		bmp_buffer.Create(rect.width, rect.height, 32);
		bmp_buffer.UseAlpha(true);
		wxGCDC gcdc{ bmp_buffer };
		auto   gc = gcdc.GetGraphicsContext();

		// Draw background on mouseover
		if (hover || pressed)
		{
			// Determine background colour
			auto col = app::isDarkTheme() ? col_background.ChangeLightness(pressed ? 125 : 115)
										  : col_background.ChangeLightness(pressed ? 70 : 80);

			// Draw background
			gcdc.SetBrush(col);
			gcdc.SetPen(*wxTRANSPARENT_PEN);
			gcdc.DrawRoundedRectangle(0, 0, rect.width, rect.height, 3.0 * wnd->GetDPIScaleFactor());
		}

		// Draw checked outline
		if (checked)
		{
			gcdc.SetBrush(*wxTRANSPARENT_BRUSH);
			gcdc.SetPen(wxPen(col_hilight, 2));
			auto px = wnd->FromDIP(1);
			gcdc.DrawRoundedRectangle(px, px, rect.width - px, rect.height - px, 3.0 * wnd->GetDPIScaleFactor());
		}

		// Draw buffer contents
		gc->Flush();
		dc.DrawBitmap(bmp_buffer, rect.x, rect.y, true);
	}

	// Draw icon
	auto disabled = item.GetState() & wxAUI_BUTTON_STATE_DISABLED;
	auto bgcol    = wnd->GetBackgroundColour();
	if (bmp.IsOk())
		dc.DrawBitmap(disabled ? disabledBitmap(bmp, bgcol) : bmp, bmp_x, bmp_y, true);

	// Determine text colour
	dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
	if (disabled)
		dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));

	// Draw text
	if ((m_flags & wxAUI_TB_TEXT || item_show_text) && !item.GetLabel().empty())
		dc.DrawText(item.GetLabel(), text_x, text_y);
}

void SAuiToolBarArt::DrawDropDownButton(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect)
{
	auto s_item         = toolbar_ ? toolbar_->itemByWxId(item.GetId()) : nullptr;
	auto item_show_text = s_item && s_item->show_text;
	int  text_width = 0, text_height = 0;

	// Determine text size if it's being drawn
	if (m_flags & wxAUI_TB_TEXT || item_show_text)
	{
		dc.SetFont(m_font);

		int tx, ty;
		dc.GetTextExtent(wxT("ABCDHgj"), &tx, &text_height);
		dc.GetTextExtent(item.GetLabel(), &text_width, &ty);
	}

	// Determine icon (and text) position and size
	int             bmp_x = 0, bmp_y = 0;
	int             text_x = 0, text_y = 0;
	const wxBitmap& bmp      = item.GetCurrentBitmapFor(wnd);
	const wxSize    bmp_size = bmp.IsOk() ? bmp.GetLogicalSize() : wxSize(0, 0);
	if (m_textOrientation == wxAUI_TBTOOL_TEXT_BOTTOM)
	{
		bmp_x = rect.x + wnd->FromDIP(3);
		bmp_y = rect.y + ((rect.height - text_height) / 2) - (bmp_size.y / 2);

		text_x = rect.x + (rect.width / 2) - (text_width / 2) + 1;
		text_y = rect.y + rect.height - text_height - 1;
	}
	if (m_textOrientation == wxAUI_TBTOOL_TEXT_RIGHT || item_show_text)
	{
		bmp_x = rect.x + wnd->FromDIP(3);
		bmp_y = rect.y + (rect.height / 2) - (bmp_size.y / 2);

		text_x = bmp_x + wnd->FromDIP(3) + bmp_size.x;
		text_y = rect.y + (rect.height / 2) - (text_height / 2);
	}

	// Get colours
	auto col_hilight    = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	auto col_background = wnd->GetBackgroundColour();

	// Draw button background
	bool checked = item.GetState() & wxAUI_BUTTON_STATE_CHECKED;
	bool hover   = item.GetState() & wxAUI_BUTTON_STATE_HOVER;
	bool pressed = item.GetState() & wxAUI_BUTTON_STATE_PRESSED;
	if (!(item.GetState() & wxAUI_BUTTON_STATE_DISABLED))
	{
		// Create buffer bitmap for background since we want to use a
		// wxGraphicsContext to draw it for better looking round edges
		wxBitmap bmp_buffer;
		bmp_buffer.Create(rect.width, rect.height, 32);
		bmp_buffer.UseAlpha(true);
		wxGCDC gcdc{ bmp_buffer };
		auto   gc = gcdc.GetGraphicsContext();

		// Draw background on mouseover
		if (hover || pressed)
		{
			// Determine background colour
			auto col = app::isDarkTheme() ? col_background.ChangeLightness(pressed ? 125 : 115)
										  : col_background.ChangeLightness(pressed ? 70 : 80);

			// Draw background
			gcdc.SetBrush(col);
			gcdc.SetPen(*wxTRANSPARENT_PEN);
			gcdc.DrawRoundedRectangle(0, 0, rect.width, rect.height, 3.0 * wnd->GetDPIScaleFactor());
		}

		// Draw checked outline
		if (checked)
		{
			gcdc.SetBrush(*wxTRANSPARENT_BRUSH);
			gcdc.SetPen(wxPen(col_hilight, 2));
			auto px = wnd->FromDIP(1);
			gcdc.DrawRoundedRectangle(px, px, rect.width - px, rect.height - px, 3.0 * wnd->GetDPIScaleFactor());
		}

		// Draw buffer contents
		gc->Flush();
		dc.DrawBitmap(bmp_buffer, rect.x, rect.y, true);
	}

	// Draw icon
	auto disabled = item.GetState() & wxAUI_BUTTON_STATE_DISABLED;
	auto bgcol    = wnd->GetBackgroundColour();
	if (bmp.IsOk())
		dc.DrawBitmap(disabled ? disabledBitmap(bmp, bgcol) : bmp, bmp_x, bmp_y, true);

	// Determine text colour
	dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
	if (disabled)
		dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));

	// Draw text
	if ((m_flags & wxAUI_TB_TEXT || item_show_text) && !item.GetLabel().empty())
		dc.DrawText(item.GetLabel(), text_x, text_y);

	// Draw dropdown arrow
	auto arrow_down = icons::getInterfaceIcon("arrow-down").GetBitmap(wxDefaultSize);
	if (arrow_down.IsOk())
		dc.DrawBitmap(
			disabled ? disabledBitmap(arrow_down, bgcol) : arrow_down,
			rect.x + rect.width - arrow_down.GetWidth() - wnd->FromDIP(3),
			rect.y + (rect.height / 2) - (arrow_down.GetHeight() / 2),
			true);
}
