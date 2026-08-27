
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Splitter.cpp
// Description: A wxSplitterWindow specialisation that increases the splitter
//              sash size (on Windows) and draws an indicator
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
#include "UI/State.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Splitter Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the sash size (platform dependant)
// -----------------------------------------------------------------------------
int Splitter::getSashSize() const
{
#ifndef __WXMSW__
	auto size = GetSashSize();
#else
	// Double width on windows
	auto size = GetSashSize() * 2;
#endif

	// Ensure size is even so the indicator is properly centered
	if (size % 2 != 0)
		size++;

	return size;
}

// -----------------------------------------------------------------------------
// Splits the window with a vertical sash, restoring the sash position
// previously saved to ui state under [state_id]/[archive] (or [default_pos] if
// none saved), and saves it to the same key whenever it's changed
// -----------------------------------------------------------------------------
bool Splitter::splitVertically(
	wxWindow*      window1,
	wxWindow*      window2,
	string_view    state_id,
	int            default_pos,
	const Archive* archive)
{
	state_id_      = state_id;
	state_archive_ = archive;

	if (ui::hasSavedState(state_id_, state_archive_, true))
		default_pos = ui::getStateInt(state_id_, state_archive_);

	Bind(wxEVT_SPLITTER_SASH_POS_CHANGING, &Splitter::onSashPosChanging, this);
	Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, &Splitter::onSashPosChanged, this);

	return wxSplitterWindow::SplitVertically(window1, window2, FromDIP(default_pos));
}

// -----------------------------------------------------------------------------
// Splits the window with a horizontal sash, restoring the sash position
// previously saved to ui state under [state_id]/[archive] (or [default_pos] if
// none saved), and saves it to the same key whenever it's changed
// -----------------------------------------------------------------------------
bool Splitter::splitHorizontally(
	wxWindow*      window1,
	wxWindow*      window2,
	string_view    state_id,
	int            default_pos,
	const Archive* archive)
{
	state_id_      = state_id;
	state_archive_ = archive;

	if (ui::hasSavedState(state_id_, state_archive_, true))
		default_pos = ui::getStateInt(state_id_, state_archive_);

	Bind(wxEVT_SPLITTER_SASH_POS_CHANGING, &Splitter::onSashPosChanging, this);
	Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, &Splitter::onSashPosChanged, this);

	return wxSplitterWindow::SplitHorizontally(window1, window2, FromDIP(default_pos));
}

// -----------------------------------------------------------------------------
// wxSplitterWindow::SashHitTest override
// -----------------------------------------------------------------------------
bool Splitter::SashHitTest(int x, int y)
{
	if (m_windowTwo == nullptr || m_sashPosition == 0)
		return false; // No sash

	int z      = m_splitMode == wxSPLIT_VERTICAL ? x : y;
	int hitMax = m_sashPosition + getSashSize() - 1;

	return z >= m_sashPosition && z <= hitMax;
}

// -----------------------------------------------------------------------------
// wxSplitterWindow::SizeWindows override
// -----------------------------------------------------------------------------
void Splitter::SizeWindows()
{
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
}

// -----------------------------------------------------------------------------
// wxSplitterWindow::DrawSash override
// -----------------------------------------------------------------------------
void Splitter::DrawSash(wxDC& dc)
{
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
	if (m_splitMode == wxSPLIT_VERTICAL)
		dc.DrawRectangle(m_sashPosition, 0, getSashSize(), size.y);
	else
		dc.DrawRectangle(0, m_sashPosition, size.x, getSashSize());

	// Indicator
	auto colour = (bgcol.GetLuminance() > 0.5) ? bgcol.ChangeLightness(m_isHot ? 50 : 80)
											   : bgcol.ChangeLightness(m_isHot ? 140 : 120);
	dc.SetBrush(wxBrush(colour));

	if (m_splitMode == wxSPLIT_VERTICAL)
	{
		auto line_x      = m_sashPosition + std::ceil(getSashSize() / 2.0);
		auto line_top    = size.y / 2 - FromDIP(28);
		line_top         = std::max(line_top, 0);
		auto line_height = FromDIP(56);
		if (line_top + line_height > size.y)
			line_height = size.y - line_top;
		dc.DrawRoundedRectangle(line_x - FromDIP(1), line_top, FromDIP(2), line_height, FromDIP(1));
	}
	else
	{
		auto line_y     = m_sashPosition + std::ceil(getSashSize() / 2.0);
		auto line_left  = size.x / 2 - FromDIP(28);
		line_left       = std::max(line_left, 0);
		auto line_width = FromDIP(56);
		if (line_left + line_width > size.x)
			line_width = size.x - line_left;
		dc.DrawRoundedRectangle(line_left, line_y - FromDIP(1), line_width, FromDIP(2), FromDIP(1));
	}
}

// -----------------------------------------------------------------------------
// wxSplitterWindow::DoGetBestSize override
// -----------------------------------------------------------------------------
wxSize Splitter::DoGetBestSize() const
{
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
}

// -----------------------------------------------------------------------------
// Called when the sash position is about to change
// -----------------------------------------------------------------------------
void Splitter::onSashPosChanging(wxSplitterEvent& e)
{
	// Wx only sends SASH_POS_CHANGING while the user is actively dragging the
	// sash, so we can use this to determine whether the change was
	// user-initiated or programmatic
	if (e.GetEventObject() == this)
		user_dragging_sash_ = true;
}

// -----------------------------------------------------------------------------
// Called when the sash position has changed
// -----------------------------------------------------------------------------
void Splitter::onSashPosChanged(wxSplitterEvent& e)
{
	// For some reason *any* child splitter will trigger this event
	if (e.GetEventObject() != this)
		return;

	// Save the new sash position to ui state only if it was changed via drag
	if (user_dragging_sash_)
	{
		ui::saveStateInt(state_id_, ToDIP(gravityAdjustedSashPos(e.GetSashPosition())), state_archive_);
		user_dragging_sash_ = false;
	}
}

// -----------------------------------------------------------------------------
// Returns [sash_pos] taking into account the sash gravity - if it's 1.0, the
// position is converted to the negative offset from the right/bottom edge
// -----------------------------------------------------------------------------
int Splitter::gravityAdjustedSashPos(int sash_pos) const
{
	if (GetSashGravity() < 1.0)
		return sash_pos;

	auto dim = GetSplitMode() == wxSPLIT_VERTICAL ? GetClientSize().x : GetClientSize().y;
	return sash_pos - dim;
}
