#pragma once

class wxChoice;
class wxStaticText;
class wxGridBagSizer;

class GenLineSpecialPanel : public wxPanel
{
public:
	GenLineSpecialPanel(wxWindow* parent);
	~GenLineSpecialPanel() {}

	void	setupForType(int type);
	void	setProp(int prop, int value);
	bool	loadSpecial(int special);
	int		getSpecial();

private:
	wxChoice*		choice_type_;
	wxChoice*		choice_props_[7];
	wxStaticText*	label_props_[7];
	wxGridBagSizer*	gb_sizer_;

	// Events
	void	onChoiceTypeChanged(wxCommandEvent& e);
	void	onChoicePropertyChanged(wxCommandEvent& e);
};
