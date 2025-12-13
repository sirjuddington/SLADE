
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ANSIScreen.cpp
// Description: Handles an ANSI screen buffer including selection
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
#include "ANSIScreen.h"
#include <utility>

using namespace slade;
using namespace gfx;


// -----------------------------------------------------------------------------
// Opens ANSI screen data from MemChunk [mc]
// -----------------------------------------------------------------------------
bool ANSIScreen::open(const MemChunk& mc)
{
	if (mc.size() != DATASIZE)
		return false;

	data_.importMem(mc);

	return true;
}

// -----------------------------------------------------------------------------
// Returns the colour byte at [index]
// -----------------------------------------------------------------------------
u8 ANSIScreen::colourAt(unsigned index) const
{
	if (data_.empty() || index >= SIZE)
		return 0;

	return data_[(index << 1) + 1];
}

// -----------------------------------------------------------------------------
// Returns the colour byte at [x, y]
// -----------------------------------------------------------------------------
u8 ANSIScreen::colourAt(u8 x, u8 y) const
{
	return colourAt((y * NUMCOLS) + x);
}

// -----------------------------------------------------------------------------
// Returns the foreground color at [index]
// -----------------------------------------------------------------------------
u8 ANSIScreen::foregroundAt(unsigned index) const
{
	return colourAt(index) & 0x0F;
}

// -----------------------------------------------------------------------------
// Returns the foreground color at [x, y]
// -----------------------------------------------------------------------------
u8 ANSIScreen::foregroundAt(u8 x, u8 y) const
{
	return foregroundAt((y * NUMCOLS) + x);
}

// -----------------------------------------------------------------------------
// Returns the background color at [index]
// -----------------------------------------------------------------------------
u8 ANSIScreen::backgroundAt(unsigned index) const
{
	return colourAt(index) >> 4 & 0x07;
}

// -----------------------------------------------------------------------------
// Returns the background color at [x, y]
// -----------------------------------------------------------------------------
u8 ANSIScreen::backgroundAt(u8 x, u8 y) const
{
	return backgroundAt((y * NUMCOLS) + x);
}

// -----------------------------------------------------------------------------
// Returns the character at [index]
// -----------------------------------------------------------------------------
u8 ANSIScreen::characterAt(unsigned index) const
{
	if (data_.empty() || index >= SIZE)
		return 0;

	return data_[index << 1];
}

// -----------------------------------------------------------------------------
// Returns the character at [x, y]
// -----------------------------------------------------------------------------
u8 ANSIScreen::characterAt(u8 x, u8 y) const
{
	return characterAt((y * NUMCOLS) + x);
}

// -----------------------------------------------------------------------------
// Sets foreground color of character at [index] to [fg]
// -----------------------------------------------------------------------------
void ANSIScreen::setForeground(unsigned index, uint8_t fg)
{
	if (data_.empty() || index >= SIZE)
		return;

	// Apply foreground colour
	uint8_t& color = data_[(index << 1) + 1];
	color          = (color & 0xF0) | (fg & 0x0F);

	signals_.char_changed(index);
}

// -----------------------------------------------------------------------------
// Sets foreground color of character at [x, y] to [fg]
// -----------------------------------------------------------------------------
void ANSIScreen::setForeground(u8 x, u8 y, u8 fg)
{
	setForeground((y * NUMCOLS) + x, fg);
}

// -----------------------------------------------------------------------------
// Sets foreground color of all selected characters to [fg]
// -----------------------------------------------------------------------------
void ANSIScreen::setSelectionForeground(u8 fg)
{
	vector<unsigned> changed;
	signals_.char_changed.block();

	for (unsigned i = 0u; i < SIZE; ++i)
		if (selection_[i])
		{
			setForeground(i, fg);
			changed.push_back(i);
		}

	signals_.char_changed.unblock();
	signals_.chars_changed(changed);
}

// -----------------------------------------------------------------------------
// Sets background color of character at [index] to [bg]
// -----------------------------------------------------------------------------
void ANSIScreen::setBackground(unsigned index, uint8_t bg)
{
	if (data_.empty() || index >= SIZE)
		return;

	// Apply background color
	auto& color = data_[(index << 1) + 1];
	color       = ((bg & 0x07) << 4) | (color & 0x0F);

	signals_.char_changed(index);
}

// -----------------------------------------------------------------------------
// Sets background color of character at [x, y] to [bg]
// -----------------------------------------------------------------------------
void ANSIScreen::setBackground(u8 x, u8 y, u8 bg)
{
	setBackground((y * NUMCOLS) + x, bg);
}

// -----------------------------------------------------------------------------
// Sets background color of all selected characters to [bg]
// -----------------------------------------------------------------------------
void ANSIScreen::setSelectionBackground(u8 bg)
{
	vector<unsigned> changed;
	signals_.char_changed.block();

	for (unsigned i = 0u; i < SIZE; ++i)
		if (selection_[i])
		{
			setBackground(i, bg);
			changed.push_back(i);
		}

	signals_.char_changed.unblock();
	signals_.chars_changed(changed);
}

// -----------------------------------------------------------------------------
// Sets character at [index] to [ch]
// -----------------------------------------------------------------------------
void ANSIScreen::setCharacter(unsigned index, uint8_t ch)
{
	if (data_.empty() || index >= SIZE)
		return;

	// Apply character
	data_[index << 1] = ch;

	signals_.char_changed(index);
}

// -----------------------------------------------------------------------------
// Sets character at [x, y] to [ch]
// -----------------------------------------------------------------------------
void ANSIScreen::setCharacter(u8 x, u8 y, u8 ch)
{
	setCharacter((y * NUMCOLS) + x, ch);
}

// -----------------------------------------------------------------------------
// Sets character of all selected characters to [ch]
// -----------------------------------------------------------------------------
void ANSIScreen::setSelectionCharacter(u8 ch)
{
	for (unsigned i = 0u; i < SIZE; ++i)
		if (selection_[i])
			setCharacter(i, ch);
}

// -----------------------------------------------------------------------------
// Returns true if the character at [index] is selected
// -----------------------------------------------------------------------------
bool ANSIScreen::isSelected(unsigned index) const
{
	return index < SIZE && selection_[index];
}

// -----------------------------------------------------------------------------
// Returns true if the character at [x, y] is selected
// -----------------------------------------------------------------------------
bool ANSIScreen::isSelected(u8 x, u8 y) const
{
	return isSelected((y * NUMCOLS) + x);
}

// -----------------------------------------------------------------------------
// Returns true if there is any selection
// -----------------------------------------------------------------------------
bool ANSIScreen::hasSelection() const
{
	return std::ranges::any_of(selection_, std::identity{});
}

// -----------------------------------------------------------------------------
// Returns the number of selected characters
// -----------------------------------------------------------------------------
unsigned ANSIScreen::selectionCount() const
{
	return static_cast<unsigned>(std::ranges::count(selection_, true));
}

// -----------------------------------------------------------------------------
// Returns the index of the first selected character
// -----------------------------------------------------------------------------
unsigned ANSIScreen::firstSelectedIndex() const
{
	return static_cast<unsigned>(std::ranges::find(selection_, true) - selection_.begin());
}

// -----------------------------------------------------------------------------
// Sets selection state of character at [index] to [set]
// -----------------------------------------------------------------------------
void ANSIScreen::select(unsigned index, bool set)
{
	if (index < SIZE)
	{
		selection_[index] = set;
		signals_.selection_changed();
	}
}

// -----------------------------------------------------------------------------
// Sets selection state of character at [x, y] to [set]
// -----------------------------------------------------------------------------
void ANSIScreen::select(u8 x, u8 y, bool set)
{
	select((y * NUMCOLS) + x, set);
}

// -----------------------------------------------------------------------------
// Sets selection state of characters at [indices] to [set]
// -----------------------------------------------------------------------------
void ANSIScreen::select(const vector<unsigned>& indices, bool set)
{
	signals_.selection_changed.block();
	for (auto idx : indices)
		select(idx, set);
	signals_.selection_changed.unblock();
	signals_.selection_changed();
}

// -----------------------------------------------------------------------------
// Toggles selection state of character at [index]
// -----------------------------------------------------------------------------
void ANSIScreen::toggleSelection(unsigned index)
{
	if (index < SIZE)
	{
		selection_[index] = !selection_[index];
		signals_.selection_changed();
	}
}

// -----------------------------------------------------------------------------
// Toggles selection state of character at [x, y]
// -----------------------------------------------------------------------------
void ANSIScreen::toggleSelection(u8 x, u8 y)
{
	toggleSelection((y * NUMCOLS) + x);
}

// -----------------------------------------------------------------------------
// Toggles selection state of characters at [indices]
// -----------------------------------------------------------------------------
void ANSIScreen::toggleSelection(const vector<unsigned>& indices)
{
	for (auto idx : indices)
		toggleSelection(idx);
}

// -----------------------------------------------------------------------------
// Moves the current selection by [x_offset] and [y_offset], if possible
// without going out of bounds
// -----------------------------------------------------------------------------
void ANSIScreen::moveSelection(i8 x_offset, i8 y_offset)
{
	Selection new_selection;
	new_selection.fill(false);
	for (unsigned i = 0u; i < SIZE; ++i)
	{
		if (selection_[i])
		{
			unsigned x     = i % NUMCOLS;
			unsigned y     = i / NUMCOLS;
			int      new_x = x + x_offset;
			int      new_y = y + y_offset;
			if (new_x >= 0 && std::cmp_less(new_x, NUMCOLS) && new_y >= 0 && std::cmp_less(new_y, NUMROWS))
				new_selection[new_y * NUMCOLS + new_x] = true;
			else
				return;
		}
	}
	selection_ = new_selection;
	signals_.selection_changed();
}
