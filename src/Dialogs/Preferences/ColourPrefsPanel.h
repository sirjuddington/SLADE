
#ifndef __COL_PREFS_PANEL_H__
#define __COL_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class wxPropertyGrid;
class ColourPrefsPanel : public PrefsPanelBase
{
private:
	wxChoice*			choice_configs;
	wxButton*			btn_saveconfig;
	wxPropertyGrid*		pg_colours;

public:
	ColourPrefsPanel(wxWindow* parent);
	~ColourPrefsPanel();

	void	init();
	void	refreshPropGrid();
	void	applyPreferences();

	// Events
	void	onChoicePresetSelected(wxCommandEvent& e);
};

#endif//__COL_PREFS_PANEL_H__
