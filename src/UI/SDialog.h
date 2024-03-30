#pragma once

namespace slade
{
class SDialog : public wxDialog
{
public:
	SDialog(
		wxWindow*       parent,
		const wxString& title,
		const wxString& id,
		int             width  = -1,
		int             height = -1,
		int             x      = -1,
		int             y      = -1);
	~SDialog() override;

	void setSavedSize(int def_width = -1, int def_height = -1);

	void onSize(wxSizeEvent& e);
	void onMove(wxMoveEvent& e);
	void onShow(wxShowEvent& e);

private:
	string id_;
};
} // namespace slade
