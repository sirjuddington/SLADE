
#ifndef __SHOW_ITEM_DIALOG_H__
#define __SHOW_ITEM_DIALOG_H__

#include <wx/dialog.h>

class wxChoice;
class wxTextCtrl;
class ShowItemDialog : public wxDialog
{
private:
	wxChoice*	choice_type;
	wxTextCtrl*	text_index;

public:
	ShowItemDialog(wxWindow* parent);
	~ShowItemDialog();

	int		getType();
	int		getIndex();
	void	setType(int type);
};

#endif//__SHOW_ITEM_DIALOG_H__
