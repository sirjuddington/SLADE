#pragma once

namespace slade::ui
{
class NumberSlider : public wxPanel
{
public:
	NumberSlider(wxWindow* parent, int min = 0, int max = 100, int interval = 10, bool decimal = false, int scale = 1);
	~NumberSlider() override = default;

	int    value() const { return slider_->GetValue(); }
	double decimalValue() const { return spin_double_ ? spin_double_->GetValue() : spin_->GetValue(); }
	void   setValue(int value) const;
	void   setDecimalValue(double value) const;

private:
	wxSlider*         slider_      = nullptr;
	wxSpinCtrl*       spin_        = nullptr;
	wxSpinCtrlDouble* spin_double_ = nullptr;
	int               scale_       = 1;
};
} // namespace slade::ui
