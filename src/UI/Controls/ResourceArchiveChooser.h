#pragma once

class Archive;
class wxButton;
class wxCheckListBox;

class ResourceArchiveChooser : public wxPanel
{
public:
	ResourceArchiveChooser(wxWindow* parent, Archive* archive);
	~ResourceArchiveChooser() {}

	vector<Archive*> selectedResourceArchives();
	string           selectedResourceList();

private:
	wxCheckListBox*  list_resources_    = nullptr;
	wxButton*        btn_open_resource_ = nullptr;
	wxButton*        btn_recent_        = nullptr;
	vector<Archive*> archives_;

	// Events
	void onBtnOpenResource(wxCommandEvent& e);
	void onBtnRecent(wxCommandEvent& e);
	void onResourceChecked(wxCommandEvent& e);
};
