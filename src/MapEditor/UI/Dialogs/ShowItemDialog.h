#pragma once

#include "MapEditor/SLADEMap/MapObject.h"

class wxChoice;
class wxTextCtrl;

class ShowItemDialog : public wxDialog
{
public:
	ShowItemDialog(wxWindow* parent);
	~ShowItemDialog() {}

	MapObject::Type getType() const;
	int             getIndex() const;
	void            setType(MapObject::Type type) const;

private:
	wxChoice*   choice_type_;
	wxTextCtrl* text_index_;
};
