
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SAuiTabArt.cpp
// Description: Custom tab art provider for wxAuiNotebook, based on
//              wxAuiGenericTabArt. Source copied over from wx and modified
//              (hence the mess). Also includes custom dock art class
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
#include "SAuiTabArt.h"
#include "Graphics/Icons.h"
#include "Utility/Colour.h"
#include "WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
const wxColor col_w10_bg(250, 250, 250);
} // namespace


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, tabs_condensed)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
wxString wxAuiChopText(const wxDC& dc, const wxString& text, int max_size)
{
	wxCoord x, y;

	// first check if the text fits with no problems
	dc.GetTextExtent(text, &x, &y);
	if (x <= max_size)
		return text;

	size_t i, len = text.Length();
	size_t last_good_length = 0;
	for (i = 0; i < len; ++i)
	{
		wxString s = text.Left(i);
		s += wxT("...");

		dc.GetTextExtent(s, &x, &y);
		if (x > max_size)
			break;

		last_good_length = i;
	}

	wxString ret = text.Left(last_good_length);
	ret += wxT("...");
	return ret;
}

void IndentPressedBitmap(wxRect* rect, int button_state)
{
	if (button_state == wxAUI_BUTTON_STATE_PRESSED)
	{
		rect->x++;
		rect->y++;
	}
}
} // namespace


// -----------------------------------------------------------------------------
//
// SAuiTabArt Class Functions
//
// -----------------------------------------------------------------------------

SAuiTabArt::SAuiTabArt(const wxWindow* window, bool close_buttons, bool main_tabs) :
	close_buttons_{ close_buttons },
	main_tabs_{ main_tabs },
	padding_(tabs_condensed ? window->FromDIP(4) : window->FromDIP(8))
{
	m_normalFont   = *wxNORMAL_FONT;
	m_selectedFont = *wxNORMAL_FONT;

	m_measuringFont = m_selectedFont;
	m_fixedTabWidth = window->FromDIP(100);
	m_tabCtrlHeight = 0;

	wxColor baseColour = wxutil::systemPanelBGColour();

	m_activeColour       = baseColour;
	m_baseColour         = baseColour;
	wxColor borderColour = baseColour.ChangeLightness(75);
	inactive_tab_colour_ = wxutil::darkColour(m_baseColour, 0.95f);

	m_borderPen       = wxPen(borderColour);
	m_baseColourPen   = wxPen(m_baseColour);
	m_baseColourBrush = wxBrush(m_baseColour);

#if wxCHECK_VERSION(3, 1, 6)
	m_activeCloseBmp    = icons::getInterfaceIcon("cross");
	close_bitmap_white_ = icons::getInterfaceIcon("cross", -1, icons::Dark);
	m_disabledCloseBmp  = icons::getInterfaceIcon("cross");

	m_activeLeftBmp   = icons::getInterfaceIcon("arrow-left");
	m_disabledLeftBmp = icons::getInterfaceIcon("arrow-left");

	m_activeRightBmp   = icons::getInterfaceIcon("arrow-right");
	m_disabledRightBmp = icons::getInterfaceIcon("arrow-right");

	m_activeWindowListBmp   = icons::getInterfaceIcon("arrow-down");
	m_disabledWindowListBmp = icons::getInterfaceIcon("arrow-down");
#else
	m_activeCloseBmp    = icons::getInterfaceIcon("cross");
	close_bitmap_white_ = icons::getInterfaceIcon("cross", -1, icons::Dark);
	m_disabledCloseBmp  = icons::getInterfaceIcon("cross").ConvertToDisabled();

	m_activeLeftBmp   = icons::getInterfaceIcon("arrow-left");
	m_disabledLeftBmp = icons::getInterfaceIcon("arrow-left").ConvertToDisabled();

	m_activeRightBmp   = icons::getInterfaceIcon("arrow-right");
	m_disabledRightBmp = icons::getInterfaceIcon("arrow-right").ConvertToDisabled();

	m_activeWindowListBmp   = icons::getInterfaceIcon("arrow-down");
	m_disabledWindowListBmp = icons::getInterfaceIcon("arrow-down").ConvertToDisabled();
#endif

	m_flags = 0;
}

SAuiTabArt::~SAuiTabArt() = default;

wxAuiTabArt* SAuiTabArt::Clone()
{
	return new SAuiTabArt(*this);
}

void SAuiTabArt::DrawBorder(wxDC& dc, wxWindow* wnd, const wxRect& rect)
{
	int    height = dynamic_cast<wxAuiNotebook*>(wnd)->GetTabCtrlHeight(); // -3;
	wxRect theRect(rect);

	dc.DrawLine(theRect.x, theRect.y + height, theRect.x, theRect.y + theRect.height);
	dc.DrawLine(
		theRect.x + theRect.width - 1, theRect.y + height, theRect.x + theRect.width - 1, theRect.y + theRect.height);
	dc.DrawLine(theRect.x, theRect.y + theRect.height - 1, theRect.x + theRect.width, theRect.y + theRect.height - 1);

	dc.SetPen(wxPen(main_tabs_ && global::win_version_major >= 10 ? col_w10_bg : m_baseColour));
	dc.DrawLine(theRect.x, theRect.y, theRect.x, theRect.y + height);
	dc.DrawLine(theRect.x + theRect.width - 1, theRect.y, theRect.x + theRect.width - 1, theRect.y + height);
	dc.DrawLine(theRect.x, theRect.y, theRect.x + theRect.width, theRect.y);
}

void SAuiTabArt::DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect)
{
	// draw background
	wxColor top_color    = main_tabs_ && global::win_version_major >= 10 ? col_w10_bg : m_baseColour;
	wxColor bottom_color = main_tabs_ && global::win_version_major >= 10 ? col_w10_bg : m_baseColour;
	wxRect  r;

	auto px1 = wnd->FromDIP(1);
	auto px2 = wnd->FromDIP(2);
	auto px4 = wnd->FromDIP(4);

	if (m_flags & wxAUI_NB_BOTTOM)
		r = wxRect(rect.x, rect.y, rect.width + px2, rect.height);
	else
		r = wxRect(rect.x, rect.y, rect.width + px2, rect.height);

	dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);

	// draw base lines
	dc.SetPen(wxPen(m_baseColour));
	int y = rect.GetHeight();
	int w = rect.GetWidth();

	if (m_flags & wxAUI_NB_BOTTOM)
	{
		dc.SetBrush(wxBrush(bottom_color));
		dc.DrawRectangle(-px1, 0, w + px2, px4);
	}
	else
	{
		// dc.SetPen(*wxTRANSPARENT_PEN);
		// dc.SetBrush(wxBrush(m_activeColour));
		// dc.SetBrush(wxBrush(wxColor(224, 238, 255)));
		// dc.DrawRectangle(-1, y - 4, w + 2, 4);

		dc.SetPen(m_borderPen);
		dc.DrawLine(-px2, y - px1, w + px2, y - px1);
	}
}


// DrawTab() draws an individual tab.
//
// dc       - output dc
// in_rect  - rectangle the tab should be confined to
// caption  - tab's caption
// active   - whether or not the tab is active
// out_rect - actual output rectangle
// x_extent - the advance x; where the next tab should start

void SAuiTabArt::DrawTab(
	wxDC&                    dc,
	wxWindow*                wnd,
	const wxAuiNotebookPage& page,
	const wxRect&            in_rect,
	int                      close_button_state,
	wxRect*                  out_tab_rect,
	wxRect*                  out_button_rect,
	int*                     x_extent)
{
	wxCoord normal_textx, normal_texty;
	wxCoord selected_textx, selected_texty;
	wxCoord texty;

	// if the caption is empty, measure some temporary text
	wxString caption = page.caption;
	if (caption.empty())
		caption = wxT("Xj");

	dc.SetFont(m_selectedFont);
	dc.GetTextExtent(caption, &selected_textx, &selected_texty);

	dc.SetFont(m_normalFont);
	dc.GetTextExtent(caption, &normal_textx, &normal_texty);

	bool bluetab = false;
	if (page.window->GetName() == "startpage")
		bluetab = true;

	// figure out the size of the tab
	wxSize tab_size = GetTabSize(dc, wnd, page.caption, page.bitmap, page.active, close_button_state, x_extent);

	// I know :P This stuff should probably be completely rewritten,
	// but this will do for now
	auto px2 = wnd->FromDIP(2);
	auto px3 = wnd->FromDIP(3);
	auto px4 = wnd->FromDIP(4);

	wxCoord tab_height = m_tabCtrlHeight + px2;
	wxCoord tab_width  = tab_size.x;
	wxCoord tab_x      = in_rect.x;
	wxCoord tab_y      = in_rect.y + in_rect.height - tab_height + px3;


	if (!page.active)
	{
		tab_height -= px2;
		tab_y += px2;
	}

	caption = page.caption;


	// select pen, brush and font for the tab to be drawn
	if (page.active)
	{
		dc.SetFont(m_selectedFont);
		texty = selected_texty;
	}
	else
	{
		dc.SetFont(m_normalFont);
		texty = normal_texty;
	}


	// create points that will make the tab outline
	int clip_width = tab_width;
	if (tab_x + clip_width > in_rect.x + in_rect.width)
		clip_width = in_rect.x + in_rect.width - tab_x;
	dc.SetClippingRegion(tab_x, tab_y, clip_width + 1, tab_height - px3);

	wxPoint border_points[6];
	if (m_flags & wxAUI_NB_BOTTOM)
	{
		border_points[0] = wxPoint(tab_x, tab_y);
		border_points[1] = wxPoint(tab_x, tab_y + tab_height - px4);
		border_points[2] = wxPoint(tab_x, tab_y + tab_height - px4);
		border_points[3] = wxPoint(tab_x + tab_width, tab_y + tab_height - px4);
		border_points[4] = wxPoint(tab_x + tab_width, tab_y + tab_height - px4);
		border_points[5] = wxPoint(tab_x + tab_width, tab_y);
	}
	else
	{
		border_points[0] = wxPoint(tab_x, tab_y + tab_height - px4);
		border_points[1] = wxPoint(tab_x, tab_y);
		border_points[2] = wxPoint(tab_x + px2, tab_y);
		border_points[3] = wxPoint(tab_x + tab_width - px2, tab_y);
		border_points[4] = wxPoint(tab_x + tab_width, tab_y);
		border_points[5] = wxPoint(tab_x + tab_width, tab_y + tab_height - px4);
	}

	int drawn_tab_yoff   = border_points[1].y + 1;
	int drawn_tab_height = border_points[0].y - border_points[1].y;


	wxColour bgcol;
	wxColour bluetab_colour(116, 135, 175);
	if (page.active)
	{
		// draw active tab
		bgcol = m_activeColour;

		// draw base background color
		wxRect r(tab_x, tab_y, tab_width, tab_height);
		dc.SetPen(wxPen(bluetab ? bluetab_colour : m_activeColour));
		dc.SetBrush(wxBrush(bluetab ? bluetab_colour : m_activeColour));
		dc.DrawRectangle(r.x + 1, r.y + 1, r.width - 1, r.height - 5);

		// highlight top of tab
		if (!bluetab)
		{
			wxColour col_hilight = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(wxBrush(col_hilight));
			dc.DrawRectangle(r.x + 1, r.y + 1, r.width - 1, px2);
		}
	}
	else
	{
		bgcol = inactive_tab_colour_;

		wxRect r(tab_x, tab_y, tab_width, tab_height);
		// wxPoint mouse = wnd->ScreenToClient(wxGetMousePosition());
		dc.SetPen(wxPen(inactive_tab_colour_));
		dc.SetBrush(wxBrush(inactive_tab_colour_));
		dc.DrawRectangle(r.x + 1, r.y + 1, r.width - 1, r.height - px4);
	}

	// draw tab outline
	dc.SetPen(m_borderPen);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawPolygon(std::size(border_points), border_points);

	// there are two horizontal grey lines at the bottom of the tab control,
	// this gets rid of the top one of those lines in the tab control
	if (page.active)
	{
		if (m_flags & wxAUI_NB_BOTTOM)
			dc.SetPen(wxPen(m_baseColour.ChangeLightness(170)));
		else
			dc.SetPen(wxPen(bluetab ? bluetab_colour : m_activeColour));
		dc.DrawLine(border_points[0].x + 1, border_points[0].y, border_points[5].x, border_points[5].y);
	}

	// draw icon if set
	if (page.bitmap.IsOk())
	{
#if wxCHECK_VERSION(3, 1, 6)
		const auto& bmp = page.bitmap.GetBitmapFor(wnd);
		dc.DrawBitmap(bmp, tab_x + padding_, drawn_tab_yoff + drawn_tab_height / 2 - bmp.GetHeight() / 2, true);
#else
		dc.DrawBitmap(
			page.bitmap,
			tab_x + padding_,
			drawn_tab_yoff + (drawn_tab_height / 2) - (page.bitmap.GetHeight() / 2),
			true);
#endif
	}

	// draw tab text
	dc.SetTextForeground(
		page.active && bluetab ? wxColor(255, 255, 255) : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	dc.DrawText(
		caption,
		tab_x + static_cast<float>(tab_width) * 0.5f - static_cast<float>(selected_textx) * 0.5f,
		drawn_tab_yoff + drawn_tab_height / 2 - texty / 2);

	// draw close button if necessary
	if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
	{
		int offsetY = tab_y;
		if (m_flags & wxAUI_NB_BOTTOM)
			offsetY = 1;

#if wxCHECK_VERSION(3, 1, 6)
		int close_button_width  = m_activeCloseBmp.GetPreferredBitmapSizeFor(wnd).x;
		int close_button_height = m_activeCloseBmp.GetPreferredBitmapSizeFor(wnd).y;
#else
		int close_button_width  = m_activeCloseBmp.GetWidth();
		int close_button_height = m_activeCloseBmp.GetHeight();
#endif

		wxRect rect(
			tab_x + tab_width - close_button_width - padding_,
			offsetY + tab_height / 2 - close_button_height / 2,
			close_button_width,
			tab_height);

		IndentPressedBitmap(&rect, close_button_state);

		bool close_white = bluetab && page.active;

		if (close_button_state == wxAUI_BUTTON_STATE_HOVER || close_button_state == wxAUI_BUTTON_STATE_PRESSED)
		{
			dc.SetPen(wxPen(wxutil::darkColour(close_white ? bluetab_colour : bgcol, 2.0f)));
			dc.SetBrush(wxBrush(wxutil::lightColour(close_white ? bluetab_colour : bgcol, 1.0f)));
			dc.DrawRectangle(rect.x, rect.y, rect.width, rect.width);

			const auto& bmp = close_white ? close_bitmap_white_ : m_activeCloseBmp;
#if wxCHECK_VERSION(3, 1, 6)
			dc.DrawBitmap(bmp.GetBitmapFor(wnd), rect.x, rect.y);
#else
			dc.DrawBitmap(bmp, rect.x, rect.y);
#endif
		}
		else
		{
			const auto& bmp = close_white ? close_bitmap_white_ : m_disabledCloseBmp;
#if wxCHECK_VERSION(3, 1, 6)
			dc.DrawBitmap(bmp.GetBitmapFor(wnd).ConvertToDisabled(), rect.x, rect.y);
#else
			dc.DrawBitmap(bmp, rect.x, rect.y);
#endif
		}

		*out_button_rect = rect;
	}

	*out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);

	dc.DestroyClippingRegion();
}

#if wxCHECK_VERSION(3, 1, 6)
wxSize SAuiTabArt::GetTabSize(
	wxDC&                 dc,
	wxWindow*             wnd,
	const wxString&       caption,
	const wxBitmapBundle& bitmap,
	bool                  WXUNUSED(active),
	int                   close_button_state,
	int*                  x_extent)
#else
wxSize SAuiTabArt::GetTabSize(
	wxDC&           dc,
	wxWindow*       WXUNUSED(wnd),
	const wxString& caption,
	const wxBitmap& bitmap,
	bool            WXUNUSED(active),
	int             close_button_state,
	int*            x_extent)
#endif
{
	wxCoord measured_textx, measured_texty, tmp;

	dc.SetFont(m_measuringFont);
	dc.GetTextExtent(caption, &measured_textx, &measured_texty);

	dc.GetTextExtent(wxT("ABCDEFXj"), &tmp, &measured_texty);

	// add padding around the text
	wxCoord tab_width  = measured_textx;
	wxCoord tab_height = measured_texty;

#if wxCHECK_VERSION(3, 1, 6)
	// if close buttons are enabled, add space for one
	if (close_buttons_)
		tab_width += m_activeCloseBmp.GetPreferredBitmapSizeFor(wnd).x + padding_;

	// if there's a bitmap, add space for it
	if (bitmap.IsOk())
	{
		tab_width += bitmap.GetPreferredBitmapSizeFor(wnd).x;
		tab_width += padding_; // right side bitmap padding
		tab_height = wxMax(tab_height, bitmap.GetPreferredBitmapSizeFor(wnd).y);
	}
	else if (tabs_condensed)
		tab_width += padding_ * 2; // a bit extra padding if there isn't an icon in condensed mode
#else
	// if close buttons are enabled, add space for one
	if (close_buttons_)
		tab_width += m_activeCloseBmp.GetWidth() + padding_;

	// if there's a bitmap, add space for it
	if (bitmap.IsOk())
	{
		tab_width += bitmap.GetWidth();
		tab_width += padding_; // right side bitmap padding
		tab_height = wxMax(tab_height, bitmap.GetHeight());
	}
	else if (tabs_condensed)
		tab_width += (padding_ * 2); // a bit extra padding if there isn't an icon in condensed mode
#endif

	// add padding
	tab_width += padding_ * 2;
	tab_height += 10;

	// minimum width
	int min_width = tabs_condensed ? 48 : 64;
	if (tab_width < min_width)
		tab_width = min_width;

	*x_extent = tab_width;

	return { tab_width, tab_height };
}

void SAuiTabArt::SetSelectedFont(const wxFont& font)
{
	// m_selectedFont = font;
}


// -----------------------------------------------------------------------------
//
// SAuiDockArt Class Functions
//
// -----------------------------------------------------------------------------

SAuiDockArt::SAuiDockArt(const wxWindow* window)
{
	caption_back_colour_ = wxutil::darkColour(wxutil::systemPanelBGColour(), 0.0f);

	wxColour textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	float    r = static_cast<float>(textColour.Red()) * 0.2f + static_cast<float>(caption_back_colour_.Red()) * 0.8f;
	float g = static_cast<float>(textColour.Green()) * 0.2f + static_cast<float>(caption_back_colour_.Green()) * 0.8f;
	float b = static_cast<float>(textColour.Blue()) * 0.2f + static_cast<float>(caption_back_colour_.Blue()) * 0.8f;
	caption_accent_colour_ = wxColor(r, g, b);

	m_activeCloseBitmap   = icons::getInterfaceIcon("cross");
	m_inactiveCloseBitmap = icons::getInterfaceIcon("cross"); //.ConvertToDisabled();

	if (global::win_version_major >= 10)
		m_sashBrush = wxBrush(col_w10_bg);

	m_captionSize = window->FromDIP(19);
	m_sashSize    = window->FromDIP(4);
	m_buttonSize  = window->FromDIP(16);
}

SAuiDockArt::~SAuiDockArt() = default;

void SAuiDockArt::DrawCaption(wxDC& dc, wxWindow* window, const wxString& text, const wxRect& rect, wxAuiPaneInfo& pane)
{
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetFont(m_captionFont);

	dc.SetBrush(wxBrush(caption_back_colour_));
	dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);

	// dc.SetPen(m_borderPen);
	// dc.SetBrush(wxBrush(wxutil::darkColour(caption_back_colour_, 2.0f)));
	// dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);

	wxColor sepCol;
	int     l = colour::greyscale(ColRGBA(caption_back_colour_)).r;
	if (l < 100)
		sepCol = wxutil::lightColour(caption_back_colour_, 2.0f);
	else
		sepCol = wxutil::darkColour(caption_back_colour_, 2.0f);

	// dc.SetPen(wxPen(sepCol));
	// dc.DrawLine(rect.x, rect.y + rect.height - 1, rect.x + rect.width, rect.y + rect.height - 1);

	auto px2 = window->FromDIP(2);
	auto px3 = window->FromDIP(3);
	auto px5 = window->FromDIP(5);

	dc.SetBrush(wxBrush(sepCol));
	dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height + 1);

#if wxCHECK_VERSION(3, 1, 6)
	const auto& icon = pane.icon.GetBitmap(wxDefaultSize);
#else
	const auto& icon = pane.icon;
#endif

	int caption_offset = 0;
	if (icon.IsOk())
	{
#if wxCHECK_VERSION(3, 1, 0)
		// Ensure the icon fits into the title bar.
		wxSize iconSize = icon.GetSize();
		if (iconSize.y > rect.height)
		{
			iconSize *= static_cast<double>(rect.height) / iconSize.y;
		}

		// Draw the icon centered vertically
		int xOffset = window->FromDIP(2);
		dc.DrawBitmap(icon, rect.x + xOffset, rect.y + (rect.height - icon.GetHeight()) / 2, true);
#else
		DrawIcon(dc, rect, pane);
#endif
		caption_offset += icon.GetWidth() + px3;
	}

	dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

	wxRect clip_rect = rect;
	clip_rect.width -= px3; // text offset
	clip_rect.width -= px2; // button padding
	if (pane.HasCloseButton())
		clip_rect.width -= m_buttonSize;
	if (pane.HasPinButton())
		clip_rect.width -= m_buttonSize;
	if (pane.HasMaximizeButton())
		clip_rect.width -= m_buttonSize;

	wxString draw_text = wxAuiChopText(dc, text, clip_rect.width);
	wxCoord  w, h;
	dc.GetTextExtent(draw_text, &w, &h);

	dc.SetClippingRegion(clip_rect);
#ifdef __WXMSW__
	dc.DrawText(draw_text, rect.x + px5 + caption_offset, rect.y + rect.height / 2 - h / 2);
#else
	dc.DrawText(draw_text, rect.x + px5 + caption_offset, rect.y + (rect.height / 2) - (h / 2) + 1);
#endif

	// dc.SetPen(wxPen(captionAccentColour));
	// dc.DrawLine(rect.x + w + 8, rect.y + (rect.height / 2) - 1, rect.x + rect.width - 16, rect.y + (rect.height / 2)
	// - 1); dc.DrawLine(rect.x + w + 8, rect.y + (rect.height / 2) + 1, rect.x + rect.width - 16, rect.y + (rect.height
	// / 2) + 1);

	dc.DestroyClippingRegion();
}

void SAuiDockArt::DrawPaneButton(
	wxDC&          dc,
	wxWindow*      window,
	int            button,
	int            button_state,
	const wxRect&  _rect,
	wxAuiPaneInfo& pane)
{
#if wxCHECK_VERSION(3, 1, 6)
	wxBitmapBundle bmp;
#else
	wxBitmap bmp;
#endif

	bool active = true;
	switch (button)
	{
	default:
	case wxAUI_BUTTON_CLOSE:
		if (pane.state & wxAuiPaneInfo::optionActive)
			bmp = m_activeCloseBitmap;
		else
		{
			bmp    = m_inactiveCloseBitmap;
			active = false;
		}
		break;
	case wxAUI_BUTTON_PIN:
		if (pane.state & wxAuiPaneInfo::optionActive)
			bmp = m_activePinBitmap;
		else
		{
			bmp    = m_inactivePinBitmap;
			active = false;
		}
		break;
	case wxAUI_BUTTON_MAXIMIZE_RESTORE:
		if (pane.IsMaximized())
		{
			if (pane.state & wxAuiPaneInfo::optionActive)
				bmp = m_activeRestoreBitmap;
			else
			{
				bmp    = m_inactiveRestoreBitmap;
				active = false;
			}
		}
		else
		{
			if (pane.state & wxAuiPaneInfo::optionActive)
				bmp = m_activeMaximizeBitmap;
			else
			{
				bmp    = m_inactiveMaximizeBitmap;
				active = false;
			}
		}
		break;
	}


	wxRect rect = _rect;

	int old_y = rect.y;
#if wxCHECK_VERSION(3, 1, 6)
	rect.y = rect.y + rect.height / 2 - bmp.GetPreferredBitmapSizeFor(window).y / 2;
#else
	rect.y = rect.y + (rect.height / 2) - (bmp.GetHeight() / 2);
#endif
	rect.height = old_y + rect.height - rect.y - 1;


	if (button_state == wxAUI_BUTTON_STATE_PRESSED)
	{
		rect.x++;
		rect.y++;
	}

	if (button_state == wxAUI_BUTTON_STATE_HOVER || button_state == wxAUI_BUTTON_STATE_PRESSED)
	{
		dc.SetPen(wxPen(wxutil::darkColour(wxutil::systemPanelBGColour(), 2.0f)));
		dc.SetBrush(wxBrush(wxutil::lightColour(wxutil::systemPanelBGColour(), 1.0f)));
		dc.DrawRectangle(rect.x, rect.y, rect.width + 1, rect.width + 1);

		bmp    = m_activeCloseBitmap;
		active = true;
	}


	// draw the button itself
#if wxCHECK_VERSION(3, 1, 6)
	dc.DrawBitmap(
		active ? bmp.GetBitmapFor(window) : bmp.GetBitmapFor(window).ConvertToDisabled(), rect.x, rect.y, true);
#else
	dc.DrawBitmap(bmp, rect.x, rect.y, true);
#endif
}
