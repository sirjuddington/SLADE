
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ANSIEntryPanel.cpp
// Description: ANSIEntryPanel class. Views ANSI entry data content
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
#include "UI/SToolBar/SToolBar.h"
#include "Utility/CodePages.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Constants
//
// -----------------------------------------------------------------------------
namespace
{
constexpr int NUMROWS = 25;
constexpr int NUMCOLS = 80;
} // namespace


// -----------------------------------------------------------------------------
// AnsiCanvas Class
//
// A canvas that displays an ANSI screen using a VGA font
// -----------------------------------------------------------------------------
namespace slade
{
class AnsiCanvas : public wxPanel
{
public:
	AnsiCanvas(wxWindow* parent) : wxPanel(parent)
	{
		// Get the all-important font data
		auto res_archive = app::archiveManager().programResourceArchive();
		if (!res_archive)
			return;
		auto ansi_font = res_archive->entryAtPath("vga-rom-font.16");
		if (!ansi_font || ansi_font->size() % 256)
			return;

		fontdata_ = ansi_font->rawData();

		// Init variables
		char_width_  = 8;
		char_height_ = ansi_font->size() / 256;
		width_       = NUMCOLS * char_width_;
		height_      = NUMROWS * char_height_;
		picdata_     = new uint8_t[width_ * height_];

		// Bind Events
		Bind(wxEVT_PAINT, &AnsiCanvas::onPaint, this);
	}

	~AnsiCanvas() override { delete[] picdata_; }

	// Loads ANSI data
	void loadData(uint8_t* data)
	{
		ansidata_ = data;
		image_.reset();
	}

	// Draws a single character. This is called from the parent ANSIPanel
	void drawCharacter(size_t index) const
	{
		if (!ansidata_)
			return;

		// Setup some variables
		uint8_t  chara = ansidata_[index << 1];       // Character
		uint8_t  color = ansidata_[(index << 1) + 1]; // Colour
		uint8_t* pic   = picdata_ + ((index / NUMCOLS) * width_ * char_height_)
					   + ((index % NUMCOLS) * char_width_);      // Position on canvas to draw
		const uint8_t* fnt = fontdata_ + (char_height_ * chara); // Position of character in font image

		// Draw character (including background)
		for (int y = 0; y < char_height_; ++y)
			for (int x = 0; x < char_width_; ++x)
				pic[x + (y * width_)] = (fnt[y] & (1 << (char_width_ - 1 - x))) ? (color & 15) : ((color & 112) >> 4);
	}

private:
	size_t               width_    = 0;
	size_t               height_   = 0;
	uint8_t*             picdata_  = nullptr;
	const uint8_t*       fontdata_ = nullptr;
	uint8_t*             ansidata_ = nullptr;
	unique_ptr<wxBitmap> image_;
	int                  char_width_  = 8;
	int                  char_height_ = 8;

	void onPaint(wxPaintEvent& e)
	{
		wxPaintDC dc(this);

		// Check image is valid
		if (!picdata_)
			return;

		// Load wx image
		if (!image_)
		{
			vector<uint8_t> rgb_data(width_ * height_ * 3);
			auto            i = 0;
			for (size_t p = 0; p < width_ * height_; ++p)
			{
				auto c        = codepages::ansiColor(picdata_[p]);
				rgb_data[i++] = c.r;
				rgb_data[i++] = c.g;
				rgb_data[i++] = c.b;
			}

			wxImage img(width_, height_, rgb_data.data(), true);
			image_ = std::make_unique<wxBitmap>(img);
		}

		dc.DrawBitmap(*image_, GetSize().x / 2 - image_->GetSize().x / 2, GetSize().y / 2 - image_->GetSize().y / 2);
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
ANSIEntryPanel::ANSIEntryPanel(wxWindow* parent) : EntryPanel(parent, "ansi")
{
	// Get the VGA font
	ansi_chardata_.assign(DATASIZE, 0);
	ansi_canvas_ = new AnsiCanvas(this);
	sizer_main_->Add(ansi_canvas_, wxSizerFlags(1).Expand());

	// Hide toolbar (no reason for it on this panel, yet)
	toolbar_->Show(false);

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
		ansi_chardata_.assign(entry->rawData(), entry->rawData() + DATASIZE);
		ansi_canvas_->loadData(ansi_chardata_.data());
		for (size_t i = 0; i < DATASIZE / 2; i++)
			ansi_canvas_->drawCharacter(i);
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
	return entry.importMem(ansi_chardata_.data(), DATASIZE);
}
