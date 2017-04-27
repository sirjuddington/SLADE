
#include "Main.h"
#include "LibraryPanel.h"

LibraryPanel::LibraryPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	auto splitter = new wxSplitterWindow(this);
	splitter->SetSashSize(8);
	sizer->Add(splitter, 1, wxEXPAND | wxALL, 8);

	list_mods_ = new wxDataViewCtrl(splitter, -1);
	
	splitter->SplitVertically(list_mods_, new wxPanel(splitter, -1), -200);
}

LibraryPanel::~LibraryPanel()
{
}
