
#ifndef __RESOURCE_ARCHIVE_CHOOSER_H__
#define __RESOURCE_ARCHIVE_CHOOSER_H__

class Archive;
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
};

#endif//__RESOURCE_ARCHIVE_CHOOSER_H__
