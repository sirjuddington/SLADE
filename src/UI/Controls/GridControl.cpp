
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GridControl.cpp
// Description: A simple control for selecting a grid size and
//              enabling/disabling grid snapping
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
#include "GridControl.h"
#include "UI/Layout.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
double grid_sizes[] = { 0.05, 0.1, 0.25, 0.5,  1,    2,    4,    8,     16,    32,   64,
						128,  256, 512,  1024, 2048, 4096, 8192, 16384, 32768, 65536 };
}


// -----------------------------------------------------------------------------
//
// GridControl Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GridControl class constructor
// -----------------------------------------------------------------------------
GridControl::GridControl(wxWindow* parent, bool allow_decimal, double min, double max) :
	wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
	min_{ min },
	max_{ max }
{
	auto lh    = LayoutHelper(this);
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	combo_grid_size_ = new wxComboBox(this, wxID_ANY);
	for (auto i : grid_sizes)
		if (i >= min_ && i <= max_)
			combo_grid_size_->Append(WX_FMT("{}", i));
	if (!allow_decimal)
		combo_grid_size_->SetValidator(wxIntegerValidator<int>());
	else
		combo_grid_size_->SetValidator(wxFloatingPointValidator<double>(3, nullptr, wxNUM_VAL_NO_TRAILING_ZEROES));
	sizer->Add(combo_grid_size_, wxSizerFlags(0).Expand());
	sizer->Add(new wxStaticText(this, -1, wxS(" px")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());

	cb_snap_ = new wxCheckBox(this, wxID_ANY, wxS("Snap"));
	sizer->Add(cb_snap_, wxSizerFlags(0).Expand());
}

// -----------------------------------------------------------------------------
// Returns the currently selected grid size
// -----------------------------------------------------------------------------
double GridControl::gridSize() const
{
	double value;
	if (combo_grid_size_->GetValue().ToDouble(&value))
		return value;

	return -1.0;
}

// -----------------------------------------------------------------------------
// Returns whether grid snapping is enabled
// -----------------------------------------------------------------------------
bool GridControl::snapEnabled() const
{
	return cb_snap_->GetValue();
}

// -----------------------------------------------------------------------------
// Sets the selected grid size to [size]
// -----------------------------------------------------------------------------
void GridControl::setGridSize(double size) const
{
	combo_grid_size_->SetValue(WX_FMT("{}", size));
}

// -----------------------------------------------------------------------------
// Increments the grid size to the next available value
// -----------------------------------------------------------------------------
void GridControl::incrementGridSize() const
{
	double size = gridSize();
	for (size_t i = 0; i < std::size(grid_sizes); ++i)
	{
		if (i >= min_ && i <= max_ && grid_sizes[i] > size)
		{
			setGridSize(grid_sizes[i]);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Decrements the grid size to the next available value
// -----------------------------------------------------------------------------
void GridControl::decrementGridSize() const
{
	double size = gridSize();
	for (int i = std::size(grid_sizes) - 1; i > 0; --i)
	{
		if (i >= min_ && i <= max_ && grid_sizes[i] < size)
		{
			setGridSize(grid_sizes[i]);
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Enables/disables grid snapping
// -----------------------------------------------------------------------------
void GridControl::snapToGrid(bool snap) const
{
	cb_snap_->SetValue(snap);
}
