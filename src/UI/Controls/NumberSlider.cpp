
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    NumberSlider.cpp
// Description: Control that combines a slider and a spin control for selecting
//              a numeric value
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
#include "NumberSlider.h"
#include "UI/Layout.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// NumberSlider Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// NumberSlider class constructor
// -----------------------------------------------------------------------------
NumberSlider::NumberSlider(wxWindow* parent, int min, int max, int interval, bool decimal, int scale) :
	wxPanel(parent),
	scale_{ scale }
{
	auto lh    = LayoutHelper(this);
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Slider
	slider_ = new wxSlider(this, -1, 0, min, max, wxDefaultPosition, lh.sliderSize(), wxSL_HORIZONTAL | wxSL_AUTOTICKS);
	slider_->SetTickFreq(interval);
	sizer->Add(slider_, lh.sfWithBorder(1, wxRIGHT).Expand());

	// Spin control
	if (decimal)
	{
		auto scale_double = static_cast<double>(scale);
		spin_double_      = new wxSpinCtrlDouble(
            this,
            -1,
            wxS("0"),
            wxDefaultPosition,
            lh.spinSize(),
            wxSP_ARROW_KEYS,
            min / scale_double,
            max / scale_double,
            0,
            interval / scale_double);
		sizer->Add(spin_double_, wxSizerFlags().CenterVertical());
	}
	else
	{
		spin_ = new wxSpinCtrl(this, -1, wxS("0"), wxDefaultPosition, lh.spinSize(), wxSP_ARROW_KEYS, min, max);
		spin_->SetIncrement(interval);
		sizer->Add(spin_, wxSizerFlags().CenterVertical());
	}

	Bind(
		wxEVT_SLIDER,
		[&](wxCommandEvent&)
		{
			if (spin_double_)
				spin_double_->SetValue(slider_->GetValue() / static_cast<double>(scale_));
			else
				spin_->SetValue(slider_->GetValue());
		});

	Bind(
		wxEVT_SPINCTRL,
		[&](wxCommandEvent&)
		{ slider_->SetValue(spin_double_ ? static_cast<int>(spin_double_->GetValue() * scale_) : spin_->GetValue()); });

	Bind(
		wxEVT_SPINCTRLDOUBLE,
		[&](wxCommandEvent&) { slider_->SetValue(static_cast<int>(spin_double_->GetValue() * scale_)); });
}

// -----------------------------------------------------------------------------
// Sets the value of the slider and spin control
// -----------------------------------------------------------------------------
void NumberSlider::setValue(int value) const
{
	slider_->SetValue(value);

	if (spin_double_)
		spin_double_->SetValue(value / static_cast<double>(scale_));
	else
		spin_->SetValue(value);
}

// -----------------------------------------------------------------------------
// Sets the decimal value of the slider and spin control
// -----------------------------------------------------------------------------
void NumberSlider::setDecimalValue(double value) const
{
	slider_->SetValue(value * scale_);

	if (spin_double_)
		spin_double_->SetValue(value);
	else
		spin_->SetValue(static_cast<int>(value));
}
