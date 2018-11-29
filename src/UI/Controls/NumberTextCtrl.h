#pragma once

class NumberTextCtrl : public wxTextCtrl
{
public:
	NumberTextCtrl(wxWindow* parent, bool allow_decimal = false);
	~NumberTextCtrl() {}

	int    getNumber(int base = 0);
	double getDecNumber(double base = 0);
	bool   isIncrement();
	bool   isDecrement();
	bool   isFactor();
	bool   isDivisor();

	void setNumber(int num);
	void setDecNumber(double num);
	void allowDecimal(bool allow) { allow_decimal_ = allow; }

private:
	string last_value_;
	int    last_point_;
	bool   allow_decimal_;

	// Events
	void onChar(wxKeyEvent& e);
	void onChanged(wxCommandEvent& e);
};
