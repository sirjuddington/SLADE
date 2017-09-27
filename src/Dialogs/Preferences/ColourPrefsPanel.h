#pragma once

#include "PrefsPanelBase.h"

class wxPropertyGrid;

class ColourPrefsPanel : public PrefsPanelBase
{
public:
	ColourPrefsPanel(wxWindow* parent);
	~ColourPrefsPanel();

	void	init() override;
	void	refreshPropGrid();
	void	applyPreferences() override;
	string	pageTitle() override { return "Colours && Theme"; }

private:
	wxChoice*		choice_configs_;
	wxButton*		btn_saveconfig_;
	wxPropertyGrid*	pg_colours_;

	// Events
	void	onChoicePresetSelected(wxCommandEvent& e);
};
