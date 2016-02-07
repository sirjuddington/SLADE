
#ifndef __RESOURCE_ARCHIVE_CHOOSER_H__
#define __RESOURCE_ARCHIVE_CHOOSER_H__

#include <wx/panel.h>

class Archive;
class wxButton;
class wxCheckListBox;
class ResourceArchiveChooser : public wxPanel
{
private:
	wxCheckListBox*		list_resources;
	wxButton*			btn_open_resource;
	wxButton*			btn_recent;
	vector<Archive*>	archives;

public:
	ResourceArchiveChooser(wxWindow* parent, Archive* archive);
	~ResourceArchiveChooser();

	vector<Archive*>	getSelectedResourceArchives();
	string				getSelectedResourceList();

	// Events
	void	onBtnOpenResource(wxCommandEvent& e);
	void	onBtnRecent(wxCommandEvent& e);
	void	onResourceChecked(wxCommandEvent& e);
};

#endif//__RESOURCE_ARCHIVE_CHOOSER_H__
