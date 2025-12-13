
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ANSIEntryPanel.cpp
// Description: ANSIEntryPanel class. Views/edits ANSI entry data content
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
#include "ANSIEntryPanel.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Graphics/ANSIScreen.h"
#include "UI/Canvas/AnsiCanvas.h"
#include "UI/SAuiToolBar.h"
#include "UI/WxUtils.h"
#include <utility>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Constants
//
// -----------------------------------------------------------------------------
wxDEFINE_EVENT(EVT_CHARMAP_PICKED, wxCommandEvent);


// -----------------------------------------------------------------------------
// CharMapPanel Class
//
// Simple panel showing 16x16 glyphs from VGA font; clicking applies the glyph
// -----------------------------------------------------------------------------
class CharMapPanel : public wxPanel
{
public:
	CharMapPanel(wxWindow* parent, const uint8_t* fontdata, int char_height) :
		wxPanel(parent),
		fontdata_(fontdata),
		char_h_(char_height)
	{
		SetDoubleBuffered(true);

		auto cell_w = char_w_ * scale_ + PADDING;
		auto cell_h = char_h_ * scale_ + PADDING;
		SetMinSize(wxSize(16 * cell_w + PADDING, 16 * cell_h + PADDING));

		Bind(wxEVT_PAINT, &CharMapPanel::onPaint, this);
		Bind(wxEVT_LEFT_DOWN, &CharMapPanel::onMouseDown, this);
		Bind(wxEVT_MOTION, &CharMapPanel::onMouseMove, this);
		Bind(wxEVT_LEAVE_WINDOW, &CharMapPanel::onMouseLeave, this);
	}

private:
	const uint8_t* fontdata_ = nullptr;
	u8             char_h_   = 8;
	u8             char_w_   = 8;
	u8             scale_    = 2;
	i32            hover_ch_ = -1;

	static constexpr u8 PADDING = 2;

	// -------------------------------------------------------------------------
	// Called when the panel needs to be repainted
	// -------------------------------------------------------------------------
	void onPaint(wxPaintEvent&)
	{
		wxPaintDC dc(this);

		wxBrush bg_brush = app::isDarkTheme() ? *wxBLACK_BRUSH : *wxWHITE_BRUSH;
		wxBrush glyph_brush(wxColour(160, 160, 160));
		wxBrush hover_brush = app::isDarkTheme() ? *wxWHITE_BRUSH : *wxBLACK_BRUSH;
		for (i32 ch = 0; ch <= 255; ++ch)
		{
			auto gx = ch % 16u;
			auto gy = ch / 16u;
			auto x0 = gx * (char_w_ * scale_ + PADDING) + PADDING;
			auto y0 = gy * (char_h_ * scale_ + PADDING) + PADDING;
			dc.SetBrush(bg_brush);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(x0, y0, char_w_ * scale_, char_h_ * scale_);
			const uint8_t* fnt = fontdata_ + (char_h_ * ch);
			dc.SetBrush(hover_ch_ == ch ? hover_brush : glyph_brush);
			for (u8 y = 0; y < char_h_; ++y)
				for (u8 x = 0; x < char_w_; ++x)
					if (fnt[y] & (1 << (char_w_ - 1 - x)))
						dc.DrawRectangle(x0 + x * scale_, y0 + y * scale_, scale_, scale_);
		}
	}

	// -------------------------------------------------------------------------
	// Called when the mouse is clicked on the panel
	// -------------------------------------------------------------------------
	void onMouseDown(wxMouseEvent& e)
	{
		// Get actual x/y
		int x = e.GetX() - PADDING;
		int y = e.GetY() - PADDING;
		if (x < 0 || y < 0)
			return;

		// Get cell at x/y
		int cell_w = char_w_ * scale_ + PADDING;
		int cell_h = char_h_ * scale_ + PADDING;
		int col    = x / cell_w;
		int row    = y / cell_h;
		if (col < 0 || row < 0 || col >= 16 || row >= 16)
			return;

		// Get character
		uint8_t ch = static_cast<uint8_t>(row * 16 + col);

		// Send event
		wxCommandEvent evt(EVT_CHARMAP_PICKED, GetId());
		evt.SetEventObject(this);
		evt.SetInt(ch);
		GetEventHandler()->ProcessEvent(evt);
	}

	// -------------------------------------------------------------------------
	// Called when the mouse is moved over the panel
	// -------------------------------------------------------------------------
	void onMouseMove(wxMouseEvent& e)
	{
		// Get actual x/y
		int x = e.GetX() - PADDING;
		int y = e.GetY() - PADDING;

		// Get character at x/y
		int cell_w = char_w_ * scale_ + PADDING;
		int cell_h = char_h_ * scale_ + PADDING;
		int col    = (x >= 0 ? x / cell_w : -1);
		int row    = (y >= 0 ? y / cell_h : -1);
		int ch     = (col >= 0 && col < 16 && row >= 0 && row < 16) ? (row * 16 + col) : -1;

		// Update hover character
		if (ch != hover_ch_)
		{
			hover_ch_ = ch;
			Refresh(false);
		}
	}

	// -------------------------------------------------------------------------
	// Called when the mouse leaves the panel
	// -------------------------------------------------------------------------
	void onMouseLeave(wxMouseEvent&)
	{
		// Clear hover character
		if (hover_ch_ != -1)
		{
			hover_ch_ = -1;
			Refresh(false);
		}
	}
};


// -----------------------------------------------------------------------------
//
// ANSIEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ANSIEntryPanel class constructor
// -----------------------------------------------------------------------------
ANSIEntryPanel::ANSIEntryPanel(wxWindow* parent) : EntryPanel(parent, "ansi"), ansi_screen_{ new gfx::ANSIScreen() }
{
	// Layout: canvas + sidebar
	auto* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer_main_->Add(sizer, wxSizerFlags(1).Expand());
	ansi_canvas_ = new ui::ANSICanvas(this);
	sizer->Add(ansi_canvas_, wxSizerFlags(1).Expand());

	// Sidebar
	auto* sidebar    = new wxPanel(this);
	auto* side_sizer = new wxBoxSizer(wxVERTICAL);
	sidebar->SetSizer(side_sizer);

	// Foreground colour chooser
	fg_choice_ = new wxChoice(sidebar, wxID_ANY);
	fg_choice_->Insert(wxEmptyString, 0);
	fg_choice_->Append(
		wxutil::arrayStringStd(
			{ "Black",
			  "Blue",
			  "Green",
			  "Cyan",
			  "Red",
			  "Magenta",
			  "Brown",
			  "Light Grey",
			  "Dark Grey",
			  "Light Blue",
			  "Light Green",
			  "Light Cyan",
			  "Light Red",
			  "Light Magenta",
			  "Yellow",
			  "White" }));
	fg_choice_->SetSelection(0);
	side_sizer->Add(new wxStaticText(sidebar, wxID_ANY, wxS("Foreground")), wxSizerFlags().Border(wxALL, 5));
	side_sizer->Add(fg_choice_, wxSizerFlags().Border(wxALL, 5).Expand());

	// Background colour chooser
	bg_choice_ = new wxChoice(sidebar, wxID_ANY);
	bg_choice_->Insert(wxEmptyString, 0);
	bg_choice_->Append(
		wxutil::arrayStringStd({ "Black", "Blue", "Green", "Cyan", "Red", "Magenta", "Brown", "Light Grey" }));
	bg_choice_->SetSelection(0);
	side_sizer->Add(new wxStaticText(sidebar, wxID_ANY, wxS("Background")), wxSizerFlags().Border(wxALL, 5));
	side_sizer->Add(bg_choice_, wxSizerFlags().Border(wxALL, 5).Expand());

	// Character map
	auto           res_archive = app::archiveManager().programResourceArchive();
	const uint8_t* fontdata    = nullptr;
	int            char_h      = 8;
	if (res_archive)
	{
		auto ansi_font = res_archive->entryAtPath("vga-rom-font.16");
		if (ansi_font && ansi_font->size() > 0 && ansi_font->size() % 256 == 0)
		{
			fontdata = ansi_font->rawData();
			char_h   = ansi_font->size() / 256;
		}
	}
	side_sizer->Add(new wxStaticText(sidebar, wxID_ANY, wxS("Character Map")), wxSizerFlags().Border(wxALL, 5));
	auto cmap = new CharMapPanel(sidebar, fontdata, char_h);
	side_sizer->Add(cmap, wxSizerFlags(1).Border(wxALL, 5).Expand());

	sizer->Add(sidebar, wxSizerFlags().Border(wxALL, 5).Expand());

	// Scale dropdown
	auto scale_choice = new wxChoice(toolbar_, wxID_ANY);
	scale_choice->Append(wxS("1x"));
	scale_choice->Append(wxS("2x"));
	scale_choice->Append(wxS("3x"));
	scale_choice->Append(wxS("4x"));
	scale_choice->SetSelection(0);
	toolbar_->registerCustomControl("scale", scale_choice);
	toolbar_->loadLayoutFromResource("entry_ansi_top");

	// Bind events
	Bind(EVT_CHARMAP_PICKED, &ANSIEntryPanel::onCharMapPicked, this, cmap->GetId());
	ansi_canvas_->Bind(wxEVT_LEFT_DOWN, &ANSIEntryPanel::onCanvasMouseDown, this);
	ansi_canvas_->Bind(wxEVT_MOTION, &ANSIEntryPanel::onCanvasMouseMove, this);
	ansi_canvas_->Bind(wxEVT_LEFT_UP, &ANSIEntryPanel::onCanvasMouseUp, this);
	ansi_canvas_->Bind(wxEVT_KEY_DOWN, &ANSIEntryPanel::onCanvasKeyDown, this);
	ansi_canvas_->Bind(wxEVT_CHAR_HOOK, &ANSIEntryPanel::onCanvasKeyDown, this);
	ansi_canvas_->Bind(wxEVT_CHAR, &ANSIEntryPanel::onCanvasChar, this);
	fg_choice_->Bind(
		wxEVT_CHOICE,
		[this](wxCommandEvent&)
		{
			if (updating_ || !ansi_screen_->hasSelection())
				return;
			int choice = fg_choice_->GetSelection();
			if (choice <= 0)
				return;
			u8 fg = static_cast<u8>(choice - 1);
			ansi_screen_->setSelectionForeground(fg);
		});
	bg_choice_->Bind(
		wxEVT_CHOICE,
		[this](wxCommandEvent&)
		{
			if (updating_ || !ansi_screen_->hasSelection())
				return;
			int choice = bg_choice_->GetSelection();
			if (choice <= 0)
				return;
			u8 bg = static_cast<u8>(choice - 1);
			ansi_screen_->setSelectionBackground(bg);
		});
	scale_choice->Bind(
		wxEVT_CHOICE,
		[this, scale_choice](wxCommandEvent&)
		{
			int sel   = scale_choice->GetSelection();
			int scale = 1;
			switch (sel)
			{
			case 0:  scale = 1; break;
			case 1:  scale = 2; break;
			case 2:  scale = 3; break;
			case 3:  scale = 4; break;
			default: scale = 1; break;
			}
			ansi_canvas_->setScale(scale);
		});

	updateControls();
	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Loads an entry to the panel
// -----------------------------------------------------------------------------
bool ANSIEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Check entry exists
	if (!entry)
		return false;

	if (entry->size() == DATASIZE)
	{
		ansi_screen_->open(entry->data());
		ansi_canvas_->openScreen(ansi_screen_.get());
		Layout();
		Refresh();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Writes the current content to [entry]
// -----------------------------------------------------------------------------
bool ANSIEntryPanel::writeEntry(ArchiveEntry& entry)
{
	return entry.importMemChunk(ansi_screen_->data());
}

// -----------------------------------------------------------------------------
// Updates editing controls based on the current selection
// -----------------------------------------------------------------------------
void ANSIEntryPanel::updateControls()
{
	bool has_sel = ansi_screen_->hasSelection();
	fg_choice_->Enable(has_sel);
	bg_choice_->Enable(has_sel);
	updating_ = true;
	if (!has_sel)
	{
		fg_choice_->SetSelection(0);
		bg_choice_->SetSelection(0);
		updating_ = false;
		return;
	}

	// Determine common fg/bg
	u8   fg_first = 255, bg_first = 255;
	bool multi_fg = false, multi_bg = false;
	for (u8 col = 0u; col < gfx::ANSIScreen::NUMCOLS; ++col)
	{
		for (u8 row = 0u; row < gfx::ANSIScreen::NUMROWS; ++row)
		{
			if (!ansi_screen_->isSelected(col, row))
				continue;

			auto fg = ansi_screen_->foregroundAt(col, row);
			auto bg = ansi_screen_->backgroundAt(col, row);
			if (fg_first == 255)
			{
				fg_first = fg;
				bg_first = bg;
			}
			else
			{
				if (fg != fg_first)
					multi_fg = true;
				if (bg != bg_first)
					multi_bg = true;
			}
		}
	}

	fg_choice_->SetSelection(multi_fg ? 0 : (fg_first + 1));
	bg_choice_->SetSelection(multi_bg ? 0 : (bg_first + 1));
	updating_ = false;
}

// -----------------------------------------------------------------------------
// Called when the mouse is pressed down on the canvas
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onCanvasMouseDown(wxMouseEvent& e)
{
	ansi_canvas_->SetFocus();

	// Check if we clicked on a character
	auto hit = ansi_canvas_->hitTest(e.GetPosition());
	if (!hit.has_value())
		return;
	auto idx = *hit;

	// Clear selection if no modifiers held
	bool ctrl  = e.ControlDown();
	bool shift = e.ShiftDown();
	if (!ctrl && !shift)
		ansi_screen_->clearSelection();

	// Shift selects a rectangle between last selected and current
	if (shift && last_selected_ != UINT_MAX)
	{
		auto col_prev    = last_selected_ % gfx::ANSIScreen::NUMCOLS;
		auto row_prev    = last_selected_ / gfx::ANSIScreen::NUMCOLS;
		auto col_current = idx % gfx::ANSIScreen::NUMCOLS;
		auto row_current = idx / gfx::ANSIScreen::NUMCOLS;
		auto col_min     = std::min(col_prev, col_current);
		auto col_max     = std::max(col_prev, col_current);
		auto row_min     = std::min(row_prev, row_current);
		auto row_max     = std::max(row_prev, row_current);

		ansi_screen_->clearSelection();

		vector<unsigned> selected_indices;
		for (auto r = row_min; r <= row_max; ++r)
			for (auto c = col_min; c <= col_max; ++c)
				selected_indices.push_back(r * gfx::ANSIScreen::NUMCOLS + c);

		ansi_screen_->select(selected_indices, true);
	}
	else
	{
		// Shift not held, toggle selection of clicked cell
		ansi_screen_->toggleSelection(idx);
		last_selected_ = idx;
	}

	// Setup drag state
	drag_anchor_ = idx;
	drag_ctrl_   = ctrl;
	dragging_    = true;

	// Update UI
	Refresh();
	updateControls();
}

// -----------------------------------------------------------------------------
// Called when the mouse is moved on the canvas
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onCanvasMouseMove(wxMouseEvent& e)
{
	if (!dragging_ || !e.LeftIsDown())
		return;

	// Get cell at mouse position
	unsigned idx;
	if (auto hit = ansi_canvas_->hitTest(e.GetPosition()))
		idx = *hit;
	else
		return;

	// Select a rectangle between the drag anchor and current mouse position
	auto             col_origin  = drag_anchor_ % gfx::ANSIScreen::NUMCOLS;
	auto             row_origin  = drag_anchor_ / gfx::ANSIScreen::NUMCOLS;
	auto             col_current = idx % gfx::ANSIScreen::NUMCOLS;
	auto             row_current = idx / gfx::ANSIScreen::NUMCOLS;
	auto             col_min     = std::min(col_origin, col_current);
	auto             col_max     = std::max(col_origin, col_current);
	auto             row_min     = std::min(row_origin, row_current);
	auto             row_max     = std::max(row_origin, row_current);
	vector<unsigned> selected;
	for (auto r = row_min; r <= row_max; ++r)
		for (auto c = col_min; c <= col_max; ++c)
			selected.push_back(r * gfx::ANSIScreen::NUMCOLS + c);

	// Update selection
	if (!drag_ctrl_)
		ansi_screen_->clearSelection();
	ansi_screen_->select(selected);
	last_selected_ = idx;

	// Update UI
	Refresh();
	updateControls();
}

// -----------------------------------------------------------------------------
// Called when a key is pressed down on the canvas
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onCanvasKeyDown(wxKeyEvent& e)
{
	if (!ansi_screen_->hasSelection())
	{
		e.Skip();
		return;
	}

	// Handle arrow keys for moving selection
	int key = e.GetKeyCode();
	int dx = 0, dy = 0;
	switch (key)
	{
	case WXK_LEFT:  dx = -1; break;
	case WXK_RIGHT: dx = 1; break;
	case WXK_UP:    dy = -1; break;
	case WXK_DOWN:  dy = 1; break;
	default:        e.Skip(); return;
	}

	// Move selection (unless it would end up out of bounds)
	ansi_screen_->moveSelection(dx, dy);

	// Update last selected
	if (last_selected_ != UINT_MAX)
	{
		auto lc = (last_selected_ % gfx::ANSIScreen::NUMCOLS) + dx;
		auto lr = (last_selected_ / gfx::ANSIScreen::NUMCOLS) + dy;
		if (lc < gfx::ANSIScreen::NUMCOLS && lr < gfx::ANSIScreen::NUMROWS)
			last_selected_ = lr * gfx::ANSIScreen::NUMCOLS + lc;
	}

	// Update UI
	Refresh();
	updateControls();
}

// -----------------------------------------------------------------------------
// Called when a character key is pressed on the canvas
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onCanvasChar(wxKeyEvent& e)
{
	if (!ansi_screen_->hasSelection())
	{
		e.Skip();
		return;
	}

	// Accept ASCII/printable characters only
	int uni = e.GetUnicodeKey();
	if (uni == WXK_NONE)
		uni = e.GetKeyCode();
	if (uni < 0 || uni > 255 || !std::isprint(static_cast<unsigned char>(uni)))
	{
		e.Skip();
		return;
	}

	// Apply character to selection
	uint8_t ch = static_cast<uint8_t>(uni & 0xFF);
	ansi_screen_->setSelectionCharacter(ch);

	// Advance selection if single cell selected
	if (ansi_screen_->selectionCount() == 1)
	{
		unsigned idx = ansi_screen_->firstSelectedIndex();
		if (idx < gfx::ANSIScreen::SIZE - 1)
		{
			ansi_screen_->moveSelection(1, 0);
			last_selected_ = idx + 1;
		}

		updateControls();
	}

	setModified();
}

// -----------------------------------------------------------------------------
// Called when a character is picked from the character map
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onCharMapPicked(wxCommandEvent& e)
{
	// Apply to selection
	ansi_screen_->setSelectionCharacter(e.GetInt());
}
