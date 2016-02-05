
#ifndef __DIR_ARCHIVE_UPDATE_DIALOG__
#define __DIR_ARCHIVE_UPDATE_DIALOG__

#include "Archive/Formats/DirArchive.h"
#include "UI/SDialog.h"

class wxDataViewListCtrl;
class DirArchiveUpdateDialog : public SDialog
{
private:
	DirArchive*					archive;
	vector<dir_entry_change_t>	changes;
	wxDataViewListCtrl*			list_changes;
	wxButton*					btn_ok;

public:
	DirArchiveUpdateDialog(wxWindow* parent, DirArchive* archive, vector<dir_entry_change_t>& changes);
	~DirArchiveUpdateDialog();

	void	populateChangeList();

	// Events
	void	onBtnOKClicked(wxCommandEvent& e);
};

#endif//__DIR_ARCHIVE_UPDATE_DIALOG__
