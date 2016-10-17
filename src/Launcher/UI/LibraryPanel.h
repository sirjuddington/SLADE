#pragma once

#include <wx/splitter.h>

class LibraryPanel : public wxPanel
{
public:
	LibraryPanel(wxWindow* parent);
	~LibraryPanel();

private:
	wxDataViewCtrl*	list_mods;
};
