#pragma once

namespace slade::ui
{
class StartPanel : public wxPanel
{
public:
	StartPanel(wxWindow* parent);

private:
	wxPanel*                   recent_files_panel_ = nullptr;
	sigslot::scoped_connection sc_recent_files_updated_;

	void     setupLayout();
	void     updateRecentFilesPanel();
	wxSizer* createRecentFileSizer(string_view full_path, int index) const;
};
} // namespace slade::ui
