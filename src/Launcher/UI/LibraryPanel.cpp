
#include "Main.h"
#include "LibraryPanel.h"

LibraryPanel::LibraryPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	wxSplitterWindow* splitter = new wxSplitterWindow(this);
	splitter->SetSashSize(8);
	sizer->Add(splitter, 1, wxEXPAND | wxALL, 8);

	list_mods = new wxDataViewCtrl(splitter, -1);
	
	splitter->SplitVertically(list_mods, new wxPanel(splitter, -1), -200);
}

LibraryPanel::~LibraryPanel()
{
}
