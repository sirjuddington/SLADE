
#ifndef __SHOW_ITEM_DIALOG_H__
#define __SHOW_ITEM_DIALOG_H__

#include "common.h"
#include "MapEditor/SLADEMap/MapObject.h"

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

	MapObject::Type	getType();
	int				getIndex();
	void			setType(MapObject::Type type);
};

#endif//__SHOW_ITEM_DIALOG_H__
