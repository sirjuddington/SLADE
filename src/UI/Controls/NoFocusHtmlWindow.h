#pragma once

#include <wx/html/htmlwin.h>

namespace slade::ui
{
// A wxHtmlWindow that never draws the dotted focus rectangle
// (wxHtmlWindow::OnPaint draws it whenever HasFocus() is true)
class NoFocusHtmlWindow : public wxHtmlWindow
{
public:
	NoFocusHtmlWindow(wxWindow* parent, wxWindowID id, long style) :
		wxHtmlWindow(parent, id, wxDefaultPosition, wxDefaultSize, style)
	{
		SetCanFocus(false);
	}

	bool HasFocus() const override { return false; }
};
} // namespace slade::ui
