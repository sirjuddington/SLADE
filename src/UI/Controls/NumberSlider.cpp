
#include "Main.h"
#include "NumberSlider.h"
#include "UI/Layout.h"

using namespace slade;
using namespace ui;


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
            "0",
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
		spin_ = new wxSpinCtrl(this, -1, "0", wxDefaultPosition, lh.spinSize(), wxSP_ARROW_KEYS, min, max);
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

void NumberSlider::setValue(int value) const
{
	slider_->SetValue(value);

	if (spin_double_)
		spin_double_->SetValue(value / static_cast<double>(scale_));
	else
		spin_->SetValue(value);
}

void NumberSlider::setDecimalValue(double value) const
{
	slider_->SetValue(value * scale_);

	if (spin_double_)
		spin_double_->SetValue(value);
	else
		spin_->SetValue(static_cast<int>(value));
}
