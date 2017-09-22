#pragma once

#include "PropsPanelBase.h"

class ColourBox;

class PlanNotePropsPanel : public PropsPanelBase
{
public:
	PlanNotePropsPanel(wxWindow* parent);

	void	openObjects(vector<MapObject*>& objects) override;
	void	applyChanges() override;

private:
	wxTextCtrl*	text_note_;
	wxTextCtrl*	text_detail_;
	ColourBox*	colbox_colour_;
	wxChoice*	choice_icon_;
};
