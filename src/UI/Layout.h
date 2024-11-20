#pragma once

namespace slade::ui
{
struct LayoutHelper
{
	const wxWindow* window;

	LayoutHelper(const wxWindow* window) : window(window) {}

	wxSizer* layoutHorizontally(const vector<wxObject*>& widgets, int expand_col = -1) const;
	void     layoutHorizontally(
			wxSizer*                 sizer,
			const vector<wxObject*>& widgets,
			wxSizerFlags             flags      = {},
			int                      expand_col = -1) const;
	wxSizer* layoutVertically(const vector<wxObject*>& widgets, int expand_row = -1) const;
	void     layoutVertically(
			wxSizer*                 sizer,
			const vector<wxObject*>& widgets,
			wxSizerFlags             flags      = {},
			int                      expand_row = -1) const;

	wxSizerFlags sfWithBorder(int proportion = 0, int direction = wxALL, int size = -1) const;
	wxSizerFlags sfWithLargeBorder(int proportion = 0, int direction = wxALL) const;
	wxSizerFlags sfWithXLargeBorder(int proportion = 0, int direction = wxALL) const;
	wxSizerFlags sfWithSmallBorder(int proportion = 0, int direction = wxALL) const;

	int     px(int size) const;
	wxSize  size(int width, int height) const;
	wxPoint point(int x, int y) const;
	wxRect  rect(int x, int y, int width, int height) const;

	int    pad() const;
	int    padLarge() const;
	int    padXLarge() const;
	int    padSmall() const;
	wxSize spinSize() const;
	wxSize sliderSize() const;
};

} // namespace slade::ui
