#pragma once

#include "UI/SDialog.h"

class wxDataViewListCtrl;

namespace slade
{
class Archive;
struct DirEntryChange;

class DirArchiveUpdateDialog : public SDialog
{
public:
	DirArchiveUpdateDialog(wxWindow* parent, Archive* archive, const vector<DirEntryChange>& changes);
	~DirArchiveUpdateDialog() override = default;

	void populateChangeList() const;

	// Events
	void onBtnOKClicked(wxCommandEvent& e);

private:
	Archive*               archive_ = nullptr;
	vector<DirEntryChange> changes_;
	wxDataViewListCtrl*    list_changes_ = nullptr;
};
} // namespace slade
