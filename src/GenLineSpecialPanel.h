
#ifndef __GEN_LINE_SPECIAL_PANEL_H__
#define __GEN_LINE_SPECIAL_PANEL_H__

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>

class GenLineSpecialPanel : public wxPanel {
private:
	wxChoice*		choice_type;
	wxChoice*		choice_props[7];
	wxStaticText*	label_props[7];
	wxGridBagSizer*	gb_sizer;

public:
	GenLineSpecialPanel(wxWindow* parent);
	~GenLineSpecialPanel();

	void	setupForType(int type);
	void	setProp(int prop, int value);
	bool	loadSpecial(int special);
	int		getSpecial();

	// Events
	void	onChoiceTypeChanged(wxCommandEvent& e);
	void	onChoicePropertyChanged(wxCommandEvent& e);
};

#endif//__GEN_LINE_SPECIAL_PANEL_H__
