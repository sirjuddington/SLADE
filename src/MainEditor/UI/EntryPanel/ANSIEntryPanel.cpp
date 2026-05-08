
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Graphics/WxGfx.h"
#include "UI/Canvas/AnsiCanvas.h"
#include "UI/SAuiToolBar.h"
#include "UI/State.h"
#include "Utility/CodePages.h"
#include "Utility/Colour.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Constants
//
// -----------------------------------------------------------------------------
wxDEFINE_EVENT(EVT_CHARMAP_PICKED, wxCommandEvent);
wxDEFINE_EVENT(EVT_COLOUR_PICKED, wxCommandEvent);


namespace slade
{
// -----------------------------------------------------------------------------
// CharMapPanel Class
//
// Simple panel showing VGA font glyphs in a 16x16 grid.
// Clicking on a character sends an EVT_CHARMAP_PICKED event.
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

	i32  selectedChar() const { return selected_ch_; }
	void setSelectedChar(i32 c)
	{
		selected_ch_ = c;
		Refresh(false);
	}

private:
	const uint8_t* fontdata_    = nullptr;
	u8             char_h_      = 8;
	u8             char_w_      = 8;
	u8             scale_       = 2;
	i32            hover_ch_    = -1;
	i32            selected_ch_ = -1;

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

			// Background
			dc.SetBrush(bg_brush);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(x0, y0, char_w_ * scale_, char_h_ * scale_);

			// Glyph
			const uint8_t* fnt = fontdata_ + (char_h_ * ch);
			dc.SetBrush(hover_ch_ == ch ? hover_brush : glyph_brush);
			for (u8 y = 0; y < char_h_; ++y)
				for (u8 x = 0; x < char_w_; ++x)
					if (fnt[y] & (1 << (char_w_ - 1 - x)))
						dc.DrawRectangle(x0 + x * scale_, y0 + y * scale_, scale_, scale_);

			// Selected outline
			if (selected_ch_ == ch)
			{
				dc.SetPen(app::isDarkTheme() ? *wxWHITE_PEN : *wxBLACK_PEN);
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.DrawRectangle(x0, y0, char_w_ * scale_, char_h_ * scale_);
				dc.SetPen(*wxTRANSPARENT_PEN);
			}
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
// ColourPickerPanel Class
//
// Displays ANSI colour swatches in a grid
// (8 or 16 colours depending on [full_range] and a 'no colour' option).
// Clicking on a swatch sends an EVT_COLOUR_PICKED event.
// -----------------------------------------------------------------------------
class ColourPickerPanel : public wxPanel
{
public:
	ColourPickerPanel(wxWindow* parent, bool full_range = true) : wxPanel(parent), full_range_{ full_range }
	{
		SetDoubleBuffered(true);
		SetMinSize(FromDIP(wxSize(24 * 9, full_range_ ? 48 : 24)));
		Bind(wxEVT_PAINT, &ColourPickerPanel::onPaint, this);
		Bind(wxEVT_LEFT_DOWN, &ColourPickerPanel::onMouseDown, this);
		Bind(wxEVT_MOTION, &ColourPickerPanel::onMouseMove, this);
		Bind(wxEVT_LEAVE_WINDOW, &ColourPickerPanel::onMouseLeave, this);
	}

	void setSelected(int colour)
	{
		if (selected_ != colour)
		{
			selected_ = colour;
			Refresh();
		}
	}
	int selected() const { return selected_; }

private:
	bool     full_range_ = true;
	int      selected_   = -2; // -2 = nothing selected, -1 = 'no colour', 0-15 = ANSI colour
	int      hover_      = -2;
	wxBitmap bmp_checkered;

	// -------------------------------------------------------------------------
	// Returns the swatch index at [px,py]
	// -------------------------------------------------------------------------
	int swatchAt(int px, int py) const
	{
		auto cell_width  = GetSize().x / 9;
		auto cell_height = full_range_ ? GetSize().y / 2 : GetSize().y;

		auto x = px / cell_width;
		auto y = py / cell_height;

		if (x < 0 || x > 8 || y < 0 || y > 1 || (!full_range_ && y > 0))
			return -2;

		if (x > 7)
			return -1;

		return y * 8 + x;
	}

	// -------------------------------------------------------------------------
	// Called when the panel needs to be repainted
	// -------------------------------------------------------------------------
	void onPaint(wxPaintEvent&)
	{
		wxPaintDC dc(this);

		auto cell_width  = GetSize().x / 9;
		auto cell_height = full_range_ ? GetSize().y / 2 : GetSize().y;

		// Colour swatches
		auto max = full_range_ ? 16 : 8;
		for (int i = 0; i < max; ++i)
		{
			auto c = codepages::ansiColor(static_cast<uint8_t>(i));
			dc.SetBrush(wxBrush(wxColour(c.r, c.g, c.b)));
			dc.SetPen(*wxTRANSPARENT_PEN);
			int x0 = (i % 8) * cell_width;
			int y0 = (i / 8) * cell_height;
			dc.DrawRectangle(x0 + 1, y0 + 1, cell_width - 1, cell_height - 1);

			if (i == selected_ || i == hover_)
			{
				// Selection outline
				if (i == selected_)
				{
					dc.SetBrush(*wxTRANSPARENT_BRUSH);
					dc.SetPen(*wxWHITE_PEN);
					dc.DrawRectangle(x0 + 1, y0 + 1, cell_width - 1, cell_height - 1);
					dc.SetPen(*wxBLACK_PEN);
					dc.DrawRectangle(x0 + 2, y0 + 2, cell_width - 3, cell_height - 3);
				}

				// Selection/Hover - draw index number
				dc.SetFont(GetFont().Bold());
				if (colour::greyscale(c).r > 128)
					dc.SetTextForeground(*wxBLACK);
				else
					dc.SetTextForeground(*wxWHITE);
				auto text     = WX_FMT("{}", i);
				auto [tw, th] = dc.GetTextExtent(text);
				dc.DrawText(text, x0 + (cell_width - tw) / 2, y0 + (cell_height - th) / 2);
			}
		}

		// 'No colour' swatch
		auto nc_height = full_range_ ? cell_height * 2 : cell_height;
		wxgfx::generateCheckeredBackground(bmp_checkered, cell_width, nc_height);
		dc.DrawBitmap(bmp_checkered, 8 * cell_width + 1, 1);
		if (selected_ == -1)
		{
			// Selection outline
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.SetPen(*wxWHITE_PEN);
			dc.DrawRectangle(8 * cell_width + 1, 1, cell_width - 1, nc_height - 1);
			dc.SetPen(*wxBLACK_PEN);
			dc.DrawRectangle(8 * cell_width + 2, 2, cell_width - 3, nc_height - 3);
		}
	}

	// -------------------------------------------------------------------------
	// Called when the mouse is clicked on the panel
	// -------------------------------------------------------------------------
	void onMouseDown(wxMouseEvent& e)
	{
		int idx = swatchAt(e.GetX(), e.GetY());
		if (idx < -1)
			return;
		setSelected(idx);
		wxCommandEvent evt(EVT_COLOUR_PICKED, GetId());
		evt.SetEventObject(this);
		evt.SetInt(idx);
		GetEventHandler()->ProcessEvent(evt);
	}

	// -------------------------------------------------------------------------
	// Called when the mouse is moved over the panel
	// -------------------------------------------------------------------------
	void onMouseMove(wxMouseEvent& e)
	{
		int idx = swatchAt(e.GetX(), e.GetY());
		if (idx < -1)
			idx = -2;
		if (idx != hover_)
		{
			hover_ = idx;
			Refresh();
		}
	}

	// -------------------------------------------------------------------------
	// Called when the mouse leaves the panel
	// -------------------------------------------------------------------------
	void onMouseLeave(wxMouseEvent&)
	{
		if (hover_ != -2)
		{
			hover_ = -2;
			Refresh();
		}
	}
};
} // namespace slade


// -----------------------------------------------------------------------------
//
// ANSIEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ANSIEntryPanel class constructor
// -----------------------------------------------------------------------------
ANSIEntryPanel::ANSIEntryPanel(wxWindow* parent) :
	EntryPanel(parent, "ansi", true),
	ansi_screen_{ new gfx::ANSIScreen() }
{
	auto* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer_main_->Add(sizer, wxSizerFlags(1).Expand());

	// Left toolbar
	toolbar_left_->loadLayoutFromResource("entry_ansi_left");
	toolbar_left_->setItemChecked("tool_view", true);

	// Canvas
	ansi_canvas_ = new ui::ANSICanvas(this);
	sizer->Add(ansi_canvas_, wxSizerFlags(1).Expand());

	// Sidebar
	sidebar_         = new wxPanel(this);
	auto* side_sizer = new wxBoxSizer(wxVERTICAL);
	sidebar_->SetSizer(side_sizer);

	// Foreground colour chooser
	side_sizer->Add(new wxStaticText(sidebar_, wxID_ANY, wxS("Foreground")), wxSizerFlags().Border(wxALL, 5));
	fg_picker_ = new ColourPickerPanel(sidebar_);
	side_sizer->Add(fg_picker_, wxSizerFlags().Border(wxALL, 5).Expand());

	// Background colour chooser
	side_sizer->Add(new wxStaticText(sidebar_, wxID_ANY, wxS("Background")), wxSizerFlags().Border(wxALL, 5));
	bg_picker_ = new ColourPickerPanel(sidebar_, false);
	side_sizer->Add(bg_picker_, wxSizerFlags().Border(wxALL, 5).Expand());

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
	label_character_ = new wxStaticText(sidebar_, wxID_ANY, wxS("Character"));
	side_sizer->Add(label_character_, wxSizerFlags().Border(wxALL, 5));
	char_map_ = new CharMapPanel(sidebar_, fontdata, char_h);
	side_sizer->Add(char_map_, wxSizerFlags(1).Border(wxALL, 5).Expand());

	sizer->Add(sidebar_, wxSizerFlags().Border(wxALL, 5).Expand());

	// Scale dropdown
	auto scale_choice = new wxChoice(toolbar_, wxID_ANY);
	scale_choice->Append(wxS("1x"));
	scale_choice->Append(wxS("2x"));
	scale_choice->Append(wxS("3x"));
	scale_choice->Append(wxS("4x"));
	toolbar_->registerCustomControl("scale", scale_choice);
	toolbar_->loadLayoutFromResource("entry_ansi_top");

	// Apply scale from saved state
	auto scale = ui::hasSavedState("ansi_scale") ? ui::getStateInt("ansi_scale") : 1;
	ansi_canvas_->setScale(scale);
	scale_choice->SetSelection(scale - 1);

	// Bind events
	Bind(EVT_CHARMAP_PICKED, &ANSIEntryPanel::onCharMapPicked, this, char_map_->GetId());
	ansi_canvas_->Bind(wxEVT_LEFT_DOWN, &ANSIEntryPanel::onCanvasMouseDown, this);
	ansi_canvas_->Bind(wxEVT_MOTION, &ANSIEntryPanel::onCanvasMouseMove, this);
	ansi_canvas_->Bind(wxEVT_LEFT_UP, &ANSIEntryPanel::onCanvasMouseUp, this);
	ansi_canvas_->Bind(wxEVT_KEY_DOWN, &ANSIEntryPanel::onCanvasKeyDown, this);
	ansi_canvas_->Bind(wxEVT_CHAR_HOOK, &ANSIEntryPanel::onCanvasKeyDown, this);
	ansi_canvas_->Bind(wxEVT_CHAR, &ANSIEntryPanel::onCanvasChar, this);
	fg_picker_->Bind(EVT_COLOUR_PICKED, &ANSIEntryPanel::onForegroundColourPicked, this);
	bg_picker_->Bind(EVT_COLOUR_PICKED, &ANSIEntryPanel::onBackgroundColourPicked, this);
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
			ui::saveStateInt("ansi_scale", scale);
			ansi_canvas_->setScale(scale);
		});

	sidebar_->Hide(); // Hidden by default since the default mode is 'View'
	updateControlsForSelection();
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
// Called when a (EntryPanel) toolbar button is clicked
// -----------------------------------------------------------------------------
void ANSIEntryPanel::toolbarButtonClick(const string& action_id)
{
	toolbar_left_->setItemChecked("tool_view", false);
	toolbar_left_->setItemChecked("tool_edit", false);
	toolbar_left_->setItemChecked("tool_paint", false);
	toolbar_left_->setItemChecked("tool_text", false);

	// View mode
	if (action_id == "tool_view")
	{
		mode_ = Mode::View;
		toolbar_left_->setItemChecked("tool_view", true);
		ansi_screen_->clearSelection();
		sidebar_->Hide();
	}

	// Edit mode
	else if (action_id == "tool_edit")
	{
		mode_ = Mode::Edit;
		toolbar_left_->setItemChecked("tool_edit", true);
		sidebar_->Show();
	}

	// Paint mode
	else if (action_id == "tool_paint")
	{
		mode_ = Mode::Paint;
		toolbar_left_->setItemChecked("tool_paint", true);
		fg_picker_->Enable();
		fg_picker_->setSelected(paint_fg_);
		bg_picker_->Enable();
		bg_picker_->setSelected(paint_bg_);
		char_map_->setSelectedChar(paint_char_);
		ansi_screen_->clearSelection();
		sidebar_->Show();
	}

	// Text mode
	else if (action_id == "tool_text")
	{
		mode_ = Mode::Text;
		toolbar_left_->setItemChecked("tool_text", true);
		fg_picker_->Enable();
		fg_picker_->setSelected(text_fg_);
		bg_picker_->Enable();
		bg_picker_->setSelected(text_bg_);
		ansi_screen_->clearSelection();
		sidebar_->Show();
	}

	// Update UI
	updateControlsForSelection();
	Layout();
	Refresh();
}

// -----------------------------------------------------------------------------
// Updates editing controls based on the current selection
// -----------------------------------------------------------------------------
void ANSIEntryPanel::updateControlsForSelection()
{
	if (mode_ == Mode::Edit)
	{
		bool has_sel = ansi_screen_->hasSelection();
		fg_picker_->Enable(has_sel);
		bg_picker_->Enable(has_sel);
		updating_ = true;
		if (!has_sel)
		{
			fg_picker_->setSelected(-2);
			bg_picker_->setSelected(-2);
			char_map_->setSelectedChar(-1);
			updating_ = false;
			return;
		}

		// Determine common fg/bg/char
		u8   fg_first = 255, bg_first = 255, char_first = 255;
		bool multi_fg = false, multi_bg = false, multi_char = false;
		for (u8 col = 0u; col < gfx::ANSIScreen::NUMCOLS; ++col)
		{
			for (u8 row = 0u; row < gfx::ANSIScreen::NUMROWS; ++row)
			{
				if (!ansi_screen_->isSelected(col, row))
					continue;

				auto fg = ansi_screen_->foregroundAt(col, row);
				auto bg = ansi_screen_->backgroundAt(col, row);
				auto ch = ansi_screen_->characterAt(col, row);
				if (fg_first == 255)
				{
					fg_first   = fg;
					bg_first   = bg;
					char_first = ch;
				}
				else
				{
					if (fg != fg_first)
						multi_fg = true;
					if (bg != bg_first)
						multi_bg = true;
					if (ch != char_first)
						multi_char = true;
				}
			}
		}

		fg_picker_->setSelected(multi_fg ? -1 : static_cast<int>(fg_first));
		bg_picker_->setSelected(multi_bg ? -1 : static_cast<int>(bg_first));
		char_map_->setSelectedChar(multi_char ? -1 : static_cast<int>(char_first));
		updating_ = false;
	}
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

	bool ctrl  = e.ControlDown();
	bool shift = e.ShiftDown();

	// Edit mode
	if (mode_ == Mode::Edit)
	{
		// Clear selection if no modifiers held
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

		updateControlsForSelection();
	}

	// Paint mode
	else if (mode_ == Mode::Paint)
	{
		// Apply selected foreground/background/character to clicked cell
		if (fg_picker_->selected() >= 0)
			ansi_screen_->setForeground(idx, static_cast<u8>(fg_picker_->selected()));
		if (bg_picker_->selected() >= 0)
			ansi_screen_->setBackground(idx, static_cast<u8>(bg_picker_->selected()));
		if (char_map_->selectedChar() >= 0)
			ansi_screen_->setCharacter(idx, static_cast<uint8_t>(char_map_->selectedChar()));
		setModified();
	}

	// Text mode
	else if (mode_ == Mode::Text)
	{
		// Select clicked cell
		ansi_screen_->clearSelection();
		ansi_screen_->toggleSelection(idx);
		last_selected_ = idx;
	}

	// Update UI
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when the mouse is moved on the canvas
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onCanvasMouseMove(wxMouseEvent& e)
{
	// Edit mode
	if (mode_ == Mode::Edit)
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
		updateControlsForSelection();
	}

	// Paint mode
	else if (mode_ == Mode::Paint)
	{
		// Get cell at mouse position
		unsigned idx;
		if (auto hit = ansi_canvas_->hitTest(e.GetPosition()))
			idx = *hit;
		else
			return;

		// Ignore if no change to the selected cell
		if (ansi_screen_->isSelected(idx))
			return;

		// Select cell at mouse position
		ansi_screen_->clearSelection();
		ansi_screen_->select(idx);

		// Apply selected foreground/background/character to hovered cell if
		// left mouse button is down
		if (e.LeftIsDown())
		{
			if (fg_picker_->selected() >= 0)
				ansi_screen_->setForeground(idx, static_cast<u8>(fg_picker_->selected()));
			if (bg_picker_->selected() >= 0)
				ansi_screen_->setBackground(idx, static_cast<u8>(bg_picker_->selected()));
			if (char_map_->selectedChar() >= 0)
				ansi_screen_->setCharacter(idx, static_cast<uint8_t>(char_map_->selectedChar()));

			setModified();
		}

		// Update UI
		Refresh();
	}
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

	int key = e.GetKeyCode();
	int dx = 0, dy = 0;
	switch (key)
	{
	case WXK_LEFT:  dx = -1; break;
	case WXK_RIGHT: dx = 1; break;
	case WXK_UP:    dy = -1; break;
	case WXK_DOWN:  dy = 1; break;
	case WXK_ESCAPE:
		ansi_screen_->clearSelection();
		last_selected_ = UINT_MAX;
		break;
	default: e.Skip(); return;
	}

	// Move selection if an arrow key was pressed
	if (dx != 0 || dy != 0)
	{
		ansi_screen_->moveSelection(dx, dy);

		// Update last selected
		if (last_selected_ != UINT_MAX)
		{
			auto lc = (last_selected_ % gfx::ANSIScreen::NUMCOLS) + dx;
			auto lr = (last_selected_ / gfx::ANSIScreen::NUMCOLS) + dy;
			if (lc < gfx::ANSIScreen::NUMCOLS && lr < gfx::ANSIScreen::NUMROWS)
				last_selected_ = lr * gfx::ANSIScreen::NUMCOLS + lc;
		}
	}

	// Update UI
	Refresh();
	updateControlsForSelection();
}

// -----------------------------------------------------------------------------
// Called when a character key is pressed on the canvas
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onCanvasChar(wxKeyEvent& e)
{
	// Ignore if no selection or not text mode
	if (!ansi_screen_->hasSelection())
	{
		e.Skip();
		return;
	}

	// Get key code
	int uni = e.GetUnicodeKey();
	if (uni == WXK_NONE)
		uni = e.GetKeyCode();

	// Backspace in text mode moves selection back and clears character
	if (mode_ == Mode::Text && uni == WXK_BACK)
	{
		ansi_screen_->moveSelection(-1, 0);
		ansi_screen_->setSelectionCharacter(0);
		setModified();
		return;
	}

	// Accept ASCII/printable characters only
	if (uni < 0 || uni > 255 || !std::isprint(static_cast<unsigned char>(uni)))
	{
		e.Skip();
		return;
	}

	// Apply character to selection
	uint8_t ch = static_cast<uint8_t>(uni & 0xFF);
	ansi_screen_->setSelectionCharacter(ch);

	// Text mode
	if (mode_ == Mode::Text)
	{
		// Apply selected foreground/background to selected cell
		if (fg_picker_->selected() >= 0)
			ansi_screen_->setSelectionForeground(static_cast<u8>(fg_picker_->selected()));
		if (bg_picker_->selected() >= 0)
			ansi_screen_->setSelectionBackground(static_cast<u8>(bg_picker_->selected()));

		// Move selection to next cell (right, then down)
		unsigned idx = ansi_screen_->firstSelectedIndex();
		if (idx < gfx::ANSIScreen::SIZE - 1)
		{
			ansi_screen_->moveSelection(1, 0);
			last_selected_ = idx + 1;
		}

		updateControlsForSelection();
	}

	setModified();
}

// -----------------------------------------------------------------------------
// Called when a character is picked from the character map
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onCharMapPicked(wxCommandEvent& e)
{
	switch (mode_)
	{
	case Mode::Edit: ansi_screen_->setSelectionCharacter(e.GetInt()); break;

	case Mode::Paint:
		char_map_->setSelectedChar(e.GetInt());
		paint_char_ = e.GetInt();
		break;

	case Mode::Text:
		ansi_screen_->setSelectionCharacter(e.GetInt());
		ansi_screen_->moveSelection(1, 0);
		break;

	default: break;
	}
}

// -----------------------------------------------------------------------------
// Called when a colour is picked from the foreground colour picker
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onForegroundColourPicked(wxCommandEvent& e)
{
	if (updating_)
		return;

	int colour = e.GetInt();

	switch (mode_)
	{
	case Mode::Edit:
		if (colour < 0)
			return;
		ansi_screen_->setSelectionForeground(static_cast<u8>(colour));
		setModified();
		break;

	case Mode::Paint:
		fg_picker_->setSelected(colour);
		paint_fg_ = colour;
		break;

	case Mode::Text:
		fg_picker_->setSelected(colour);
		text_fg_ = colour;
		break;

	default: break;
	}
}

// -----------------------------------------------------------------------------
// Called when a colour is picked from the background colour picker
// -----------------------------------------------------------------------------
void ANSIEntryPanel::onBackgroundColourPicked(wxCommandEvent& e)
{
	if (updating_)
		return;

	int colour = e.GetInt();

	switch (mode_)
	{
	case Mode::Edit:
		if (colour < 0)
			return;
		ansi_screen_->setSelectionBackground(static_cast<u8>(colour));
		setModified();
		break;

	case Mode::Paint:
		bg_picker_->setSelected(colour);
		paint_bg_ = colour;
		break;

	case Mode::Text:
		bg_picker_->setSelected(colour);
		text_bg_ = colour;
		break;

	default: break;
	}
}
