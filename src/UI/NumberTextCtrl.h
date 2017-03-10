
#ifndef __NUMBER_TEXT_CTRL_H__
#define __NUMBER_TEXT_CTRL_H__

#include "common.h"

class NumberTextCtrl : public wxTextCtrl
{
private:
	string	last_value;
	int		last_point;
	bool	allow_decimal;

public:
	NumberTextCtrl(wxWindow* parent, bool allow_decimal = false);
	~NumberTextCtrl() {}

	int		getNumber(int base = 0);
	double	getDecNumber(double base = 0);
	bool	isIncrement();
	bool	isDecrement();
	bool	isFactor();
	bool	isDivisor();

	void	setNumber(int num);
	void	setDecNumber(double num);
	void	allowDecimal(bool allow) { allow_decimal = allow; }

	// Events
	void onChar(wxKeyEvent& e);
	void onChanged(wxCommandEvent& e);
};

#endif//__NUMBER_TEXT_CTRL_H__
