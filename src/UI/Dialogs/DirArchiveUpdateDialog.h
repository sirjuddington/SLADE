#pragma once

#include "UI/SDialog.h"

class wxDataViewListCtrl;

namespace slade
{
class DirArchive;
struct DirEntryChange;

class DirArchiveUpdateDialog : public SDialog
{
public:
	DirArchiveUpdateDialog(wxWindow* parent, DirArchive* archive, const vector<DirEntryChange>& changes);
	~DirArchiveUpdateDialog() override = default;

	void populateChangeList() const;

	// Events
	void onBtnOKClicked(wxCommandEvent& e);

private:
	DirArchive*            archive_ = nullptr;
	vector<DirEntryChange> changes_;
	wxDataViewListCtrl*    list_changes_ = nullptr;
};
} // namespace slade
