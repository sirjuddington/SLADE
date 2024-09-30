
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Splitter.cpp
// Description: A wxSplitterWindow specialisation that increases the splitter
//              sash size and draws an indicator (Windows only, unchanged
//              elsewhere)
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
#include "Splitter.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Splitter Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// wxSplitterWindow::SashHitTest override
// -----------------------------------------------------------------------------
bool Splitter::SashHitTest(int x, int y)
{
#ifndef __WXMSW__
	return wxSplitterWindow::SashHitTest(x, y);
#else
	if (m_windowTwo == nullptr || m_sashPosition == 0)
		return false; // No sash

	int z      = m_splitMode == wxSPLIT_VERTICAL ? x : y;
	int hitMax = m_sashPosition + getSashSize() - 1;

	return z >= m_sashPosition && z <= hitMax;
#endif
}

// -----------------------------------------------------------------------------
// wxSplitterWindow::SizeWindows override
// -----------------------------------------------------------------------------
void Splitter::SizeWindows()
{
#ifndef __WXMSW__
	wxSplitterWindow::SizeWindows();
#else
	// check if we have delayed setting the real sash position
	if (m_requestedSashPosition != INT_MAX)
	{
		int newSashPosition = ConvertSashPosition(m_requestedSashPosition);
		if (newSashPosition != m_sashPosition)
		{
			DoSetSashPosition(newSashPosition);
		}

		if (newSashPosition <= m_sashPosition && newSashPosition >= m_sashPosition - GetBorderSize())
		{
			// don't update it any more
			m_requestedSashPosition = INT_MAX;
		}
	}

	int w, h;
	GetClientSize(&w, &h);

	if (GetWindow1() && !GetWindow2())
	{
		GetWindow1()->SetSize(GetBorderSize(), GetBorderSize(), w - 2 * GetBorderSize(), h - 2 * GetBorderSize());
	}
	else if (GetWindow1() && GetWindow2())
	{
		const int border = GetBorderSize(), sash = getSashSize();

		int size1 = GetSashPosition() - border, size2 = GetSashPosition() + sash;

		int x2, y2, w1, h1, w2, h2;
		if (GetSplitMode() == wxSPLIT_VERTICAL)
		{
			w1 = size1;
			w2 = w - 2 * border - sash - w1;
			if (w2 < 0)
				w2 = 0;
			h2 = h - 2 * border;
			if (h2 < 0)
				h2 = 0;
			h1 = h2;
			x2 = size2;
			y2 = border;
		}
		else // horz splitter
		{
			w2 = w - 2 * border;
			if (w2 < 0)
				w2 = 0;
			w1 = w2;
			h1 = size1;
			h2 = h - 2 * border - sash - h1;
			if (h2 < 0)
				h2 = 0;
			x2 = border;
			y2 = size2;
		}

		GetWindow2()->SetSize(x2, y2, w2, h2);
		GetWindow1()->SetSize(border, border, w1, h1);
	}

	wxClientDC dc(this);
	DrawSash(dc);
#endif
}

// -----------------------------------------------------------------------------
// wxSplitterWindow::DrawSash override
// -----------------------------------------------------------------------------
void Splitter::DrawSash(wxDC& dc)
{
#ifndef __WXMSW__
	wxSplitterWindow::DrawSash(dc);
#else
	if (HasFlag(wxSP_3DBORDER))
		wxRendererNative::Get().DrawSplitterBorder(this, dc, GetClientRect());

	// don't draw sash if we're not split
	if (m_sashPosition == 0 || !m_windowTwo)
		return;

	// nor if we're configured to not show it
	if (IsSashInvisible())
		return;

	// Background
	auto bgcol = GetBackgroundColour();
	auto size  = GetClientSize();
	dc.SetBrush(wxBrush(bgcol));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(m_sashPosition, 0, getSashSize(), size.y);

	// Indicator
	auto colour = (bgcol.GetLuminance() > 0.5) ? bgcol.ChangeLightness(m_isHot ? 50 : 80)
											   : bgcol.ChangeLightness(m_isHot ? 150 : 120);
	dc.SetBrush(wxBrush(colour));
	auto line_x   = m_sashPosition + getSashSize() / 2;
	auto line_top = size.y / 2 - FromDIP(24);
	if (line_top < 0)
		line_top = 0;
	auto line_height = FromDIP(48);
	if (line_top + line_height > size.y)
		line_height = size.y - line_top;
	dc.DrawRoundedRectangle(line_x - FromDIP(1), line_top, FromDIP(2), line_height, FromDIP(1));
#endif
}

// -----------------------------------------------------------------------------
// wxSplitterWindow::DoGetBestSize override
// -----------------------------------------------------------------------------
wxSize Splitter::DoGetBestSize() const
{
#ifndef __WXMSW__
	return wxSplitterWindow::DoGetBestSize();
#else
	// get best sizes of subwindows
	wxSize size1, size2;
	if (m_windowOne)
		size1 = m_windowOne->GetEffectiveMinSize();
	if (m_windowTwo)
		size2 = m_windowTwo->GetEffectiveMinSize();

	// sum them
	//
	// pSash points to the size component to which sash size must be added
	int*   pSash;
	wxSize sizeBest;
	if (m_splitMode == wxSPLIT_VERTICAL)
	{
		sizeBest.y = wxMax(size1.y, size2.y);
		sizeBest.x = wxMax(size1.x, m_minimumPaneSize) + wxMax(size2.x, m_minimumPaneSize);

		pSash = &sizeBest.x;
	}
	else // wxSPLIT_HORIZONTAL
	{
		sizeBest.x = wxMax(size1.x, size2.x);
		sizeBest.y = wxMax(size1.y, m_minimumPaneSize) + wxMax(size2.y, m_minimumPaneSize);

		pSash = &sizeBest.y;
	}

	// account for the sash if the window is actually split
	if (m_windowOne && m_windowTwo)
		*pSash += getSashSize();

	// account for the border too
	int border = 2 * GetBorderSize();
	sizeBest.x += border;
	sizeBest.y += border;

	return sizeBest;
#endif
}
