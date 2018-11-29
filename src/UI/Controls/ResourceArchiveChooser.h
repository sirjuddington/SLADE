#pragma once

class Archive;
class wxButton;
class wxCheckListBox;

class ResourceArchiveChooser : public wxPanel
{
public:
	ResourceArchiveChooser(wxWindow* parent, Archive* archive);
	~ResourceArchiveChooser() {}

	vector<Archive*> getSelectedResourceArchives();
	string           getSelectedResourceList();

private:
	wxCheckListBox*  list_resources_;
	wxButton*        btn_open_resource_;
	wxButton*        btn_recent_;
	vector<Archive*> archives_;

	// Events
	void onBtnOpenResource(wxCommandEvent& e);
	void onBtnRecent(wxCommandEvent& e);
	void onResourceChecked(wxCommandEvent& e);
};
