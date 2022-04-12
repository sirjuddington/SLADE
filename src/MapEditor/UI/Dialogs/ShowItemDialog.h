#pragma once

#include "SLADEMap/MapObject/MapObject.h"

class wxChoice;
class wxTextCtrl;

namespace slade
{
class ShowItemDialog : public wxDialog
{
public:
	ShowItemDialog(wxWindow* parent);
	~ShowItemDialog() {}

	MapObject::Type type() const;
	int             index() const;
	void            setType(MapObject::Type type) const;

private:
	wxChoice*   choice_type_ = nullptr;
	wxTextCtrl* text_index_  = nullptr;
};
} // namespace slade
