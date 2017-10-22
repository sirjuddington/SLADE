#pragma once

#include "common.h"

class wxChoice;
class wxTextCtrl;

class ShowItemDialog : public wxDialog
{
public:
	ShowItemDialog(wxWindow* parent);
	~ShowItemDialog() {}

	int		getType() const;
	int		getIndex() const;
	void	setType(int type) const;

private:
	wxChoice*	choice_type_;
	wxTextCtrl*	text_index_;
};
