#pragma once

#include "Archive/Formats/DirArchive.h"
#include "UI/SDialog.h"

class wxDataViewListCtrl;

namespace slade
{
class DirArchiveUpdateDialog : public SDialog
{
public:
	DirArchiveUpdateDialog(wxWindow* parent, DirArchive* archive, vector<DirEntryChange>& changes);
	~DirArchiveUpdateDialog() = default;

	void populateChangeList();

	// Events
	void onBtnOKClicked(wxCommandEvent& e);

private:
	DirArchive*            archive_ = nullptr;
	vector<DirEntryChange> changes_;
	wxDataViewListCtrl*    list_changes_ = nullptr;
};
} // namespace slade
