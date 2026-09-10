#pragma once

#include <wx/splitter.h>

namespace slade::ui
{
class Splitter : public wxSplitterWindow
{
public:
	Splitter(wxWindow* parent, wxWindowID id = -1, long style = 768L) :
		wxSplitterWindow(parent, id, wxDefaultPosition, wxDefaultSize, style)
	{
	}

	int getSashSize() const;

	bool splitVertically(
		wxWindow*      window1,
		wxWindow*      window2,
		string_view    state_id,
		int            default_pos,
		const Archive* archive = nullptr);
	bool splitHorizontally(
		wxWindow*      window1,
		wxWindow*      window2,
		string_view    state_id,
		int            default_pos,
		const Archive* archive = nullptr);

	// wxSplitterWindow Overrides
	bool   SashHitTest(int x, int y) override;
	void   SizeWindows() override;
	void   DrawSash(wxDC& dc) override;
	wxSize DoGetBestSize() const override;

private:
	string         state_id_;
	const Archive* state_archive_      = nullptr;
	bool           user_dragging_sash_ = false;

	void onSashPosChanging(wxSplitterEvent& e);
	void onSashPosChanged(wxSplitterEvent& e);
	int  gravityAdjustedSashPos(int sash_pos) const;
};
} // namespace slade::ui
