#pragma once

#include <wx/aui/auibar.h>

namespace slade
{
class SAuiToolBar;
class SAuiToolBarArt : public wxAuiGenericToolBarArt
{
public:
	SAuiToolBarArt(SAuiToolBar* toolbar, bool main_toolbar) : toolbar_{ toolbar }, main_toolbar_{ main_toolbar } {}
	~SAuiToolBarArt() override = default;

	int    GetElementSize(int elementId) override;
	wxSize GetToolSize(wxReadOnlyDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item) override;

	void DrawPlainBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect) override;
	void DrawGripper(wxDC& dc, wxWindow* wnd, const wxRect& rect) override {} // No gripper
	void DrawButton(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect) override;
	void DrawDropDownButton(wxDC& dc, wxWindow* wnd, const wxAuiToolBarItem& item, const wxRect& rect) override;

private:
	SAuiToolBar* toolbar_      = nullptr;
	bool         main_toolbar_ = false;
};
} // namespace slade
