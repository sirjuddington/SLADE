#pragma once

namespace slade::ui
{
class RadioButtonPanel : public wxPanel
{
public:
	RadioButtonPanel(
		wxWindow*             parent,
		const vector<string>& choices,
		const string&         label       = "",
		int                   selected    = 0,
		int                   orientation = wxHORIZONTAL);

	int  getSelection() const;
	void setSelection(int index) const;

private:
	vector<wxRadioButton*> radio_buttons_;
};
} // namespace slade::ui
