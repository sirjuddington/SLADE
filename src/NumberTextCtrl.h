
#ifndef __NUMBER_TEXT_CTRL_H__
#define __NUMBER_TEXT_CTRL_H__

class NumberTextCtrl : public wxTextCtrl
{
private:
	string	last_value;
	int		last_point;

public:
	NumberTextCtrl(wxWindow* parent);
	~NumberTextCtrl() {}

	int		getNumber(int base = 0);
	bool	isIncrement();
	bool	isDecrement();

	// Events
	void onChar(wxKeyEvent& e);
	void onChanged(wxCommandEvent& e);
};

#endif//__NUMBER_TEXT_CTRL_H__
