
#ifndef __S_AUI_TAB_ART_H__
#define __S_AUI_TAB_ART_H__

#include "common.h"

class wxAuiCommandCapture : public wxEvtHandler
{
public:

	wxAuiCommandCapture() {
		m_last_id = 0;
	}
	int GetCommandId() const {
		return m_last_id;
	}

	bool ProcessEvent(wxEvent& evt) override
	{
		if (evt.GetEventType() == wxEVT_MENU) {
			m_last_id = evt.GetId();
			return true;
		}

		if (GetNextHandler())
			return GetNextHandler()->ProcessEvent(evt);

		return false;
	}

private:
	int m_last_id;
};

class SAuiTabArt : public wxAuiGenericTabArt
{
public:
	SAuiTabArt(bool close_buttons = false, bool main_tabs = false);
	virtual ~SAuiTabArt();

	wxAuiTabArt* Clone() override;
	void SetSelectedFont(const wxFont& font) override;

	void DrawBorder(
		wxDC& dc,
		wxWindow* wnd,
		const wxRect& rect) override;

	void DrawBackground(
		wxDC& dc,
		wxWindow* wnd,
		const wxRect& rect) override;

	void DrawTab(wxDC& dc,
		wxWindow* wnd,
		const wxAuiNotebookPage& pane,
		const wxRect& inRect,
		int closeButtonState,
		wxRect* outTabRect,
		wxRect* outButtonRect,
		int* xExtent) override;

	wxSize GetTabSize(
		wxDC& dc,
		wxWindow* wnd,
		const wxString& caption,
		const wxBitmap& bitmap,
		bool active,
		int closeButtonState,
		int* xExtent) override;

	int GetIndentSize() override { return 2; }

protected:
	bool		close_buttons_;
	wxColour	inactive_tab_colour_;
	bool		main_tabs_;
	int			padding_;
};

class SAuiDockArt : public wxAuiDefaultDockArt
{
private:
	wxColour	captionBackColour;
	wxColour	captionAccentColour;

public:
	SAuiDockArt();
	virtual ~SAuiDockArt();

	void DrawCaption(wxDC& dc,
		wxWindow *window,
		const wxString& text,
		const wxRect& rect,
		wxAuiPaneInfo& pane) override;

	void DrawPaneButton(wxDC& dc,
		wxWindow *window,
		int button,
		int buttonState,
		const wxRect& rect,
		wxAuiPaneInfo& pane) override;
};

#endif//__S_AUI_TAB_ART_H__
