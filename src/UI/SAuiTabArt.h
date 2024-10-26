#pragma once

namespace slade
{
// Unused?
// class wxAuiCommandCapture : public wxEvtHandler
//{
// public:
//	wxAuiCommandCapture() { last_id_ = 0; }
//	int GetCommandId() const { return last_id_; }
//
//	bool ProcessEvent(wxEvent& evt) override
//	{
//		if (evt.GetEventType() == wxEVT_MENU)
//		{
//			last_id_ = evt.GetId();
//			return true;
//		}
//
//		if (GetNextHandler())
//			return GetNextHandler()->ProcessEvent(evt);
//
//		return false;
//	}
//
// private:
//	int last_id_;
// };

class SAuiTabArt : public wxAuiGenericTabArt
{
public:
	SAuiTabArt(const wxWindow* window, bool close_buttons = false, bool main_tabs = false);
	~SAuiTabArt() override;

	wxAuiTabArt* Clone() override;
	void         SetSelectedFont(const wxFont& font) override;

	void DrawBorder(wxDC& dc, wxWindow* wnd, const wxRect& rect) override;

	void DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect) override;

	void DrawTab(
		wxDC&                    dc,
		wxWindow*                wnd,
		const wxAuiNotebookPage& pane,
		const wxRect&            inRect,
		int                      closeButtonState,
		wxRect*                  outTabRect,
		wxRect*                  outButtonRect,
		int*                     xExtent) override;

#if wxCHECK_VERSION(3, 3, 0)
	wxSize GetTabSize(
		wxReadOnlyDC&         dc,
		wxWindow*             wnd,
		const wxString&       caption,
		const wxBitmapBundle& bitmap,
		bool                  active,
		int                   closeButtonState,
		int*                  xExtent) override;
#else
	wxSize GetTabSize(
		wxDC&                 dc,
		wxWindow*             wnd,
		const wxString&       caption,
		const wxBitmapBundle& bitmap,
		bool                  active,
		int                   closeButtonState,
		int*                  xExtent) override;
#endif

	int GetIndentSize() override { return -1; }

protected:
	bool     close_buttons_;
	wxColour inactive_tab_colour_;
	bool     main_tabs_;
	int      padding_;

	wxBitmapBundle close_bitmap_white_;
};

class SAuiDockArt : public wxAuiDefaultDockArt
{
public:
	SAuiDockArt(const wxWindow* window);
	~SAuiDockArt() override;

	void DrawCaption(wxDC& dc, wxWindow* window, const wxString& text, const wxRect& rect, wxAuiPaneInfo& pane)
		override;

	void DrawPaneButton(
		wxDC&          dc,
		wxWindow*      window,
		int            button,
		int            buttonState,
		const wxRect&  rect,
		wxAuiPaneInfo& pane) override;

private:
	wxColour caption_back_colour_;
	wxColour caption_accent_colour_;
};
} // namespace slade
