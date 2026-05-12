#pragma once

namespace slade
{
class ResourceArchiveChooser : public wxPanel
{
public:
	ResourceArchiveChooser(wxWindow* parent, const Archive* archive);
	~ResourceArchiveChooser() override = default;

	vector<Archive*> selectedResourceArchives() const;
	string           selectedResourceList() const;

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
} // namespace slade
