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

	int getSashSize() const { return GetSashSize() * 2; }

	// wxSplitterWindow Overrides
	bool SashHitTest(int x, int y) override;
	void SizeWindows() override;
	void DrawSash(wxDC& dc) override;
	wxSize DoGetBestSize() const override;
};
} // namespace slade::ui
