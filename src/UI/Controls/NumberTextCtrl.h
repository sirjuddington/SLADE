#pragma once

namespace slade
{
class NumberTextCtrl : public wxTextCtrl
{
public:
	NumberTextCtrl(wxWindow* parent, bool allow_decimal = false);
	~NumberTextCtrl() override {}

	int    number(int base = 0) const;
	double decNumber(double base = 0) const;
	bool   isIncrement() const;
	bool   isDecrement() const;
	bool   isFactor() const;
	bool   isDivisor() const;

	void setNumber(int num);
	void setDecNumber(double num);
	void allowDecimal(bool allow) { allow_decimal_ = allow; }

private:
	wxString last_value_;
	int      last_point_    = 0;
	bool     allow_decimal_ = false;

	// Events
	void onChar(wxKeyEvent& e);
	void onChanged(wxCommandEvent& e);
};
} // namespace slade
