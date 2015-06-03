
#ifndef __S_AUI_TAB_ART_H__
#define __S_AUI_TAB_ART_H__

#include <wx/aui/auibook.h>
#include <wx/aui/dockart.h>

class wxAuiCommandCapture : public wxEvtHandler
{
public:

	wxAuiCommandCapture() {
		m_last_id = 0;
	}
	int GetCommandId() const {
		return m_last_id;
	}

	bool ProcessEvent(wxEvent& evt) {
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
	SAuiTabArt(bool close_buttons = false);
	virtual ~SAuiTabArt();

	//wxBitmap bitmapFromBits(const unsigned char bits[], int w, int h, const wxColour& color);

	wxAuiTabArt* Clone();
	void SetSelectedFont(const wxFont& font);

	void DrawBorder(
		wxDC& dc,
		wxWindow* wnd,
		const wxRect& rect);

	void DrawBackground(
		wxDC& dc,
		wxWindow* wnd,
		const wxRect& rect);

	void DrawTab(wxDC& dc,
		wxWindow* wnd,
		const wxAuiNotebookPage& pane,
		const wxRect& inRect,
		int closeButtonState,
		wxRect* outTabRect,
		wxRect* outButtonRect,
		int* xExtent);

	wxSize GetTabSize(
		wxDC& dc,
		wxWindow* wnd,
		const wxString& caption,
		const wxBitmap& bitmap,
		bool active,
		int closeButtonState,
		int* xExtent);

protected:
	bool m_closeButtons;
	wxColour m_inactiveTabColour;
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
		wxAuiPaneInfo& pane);

	void DrawPaneButton(wxDC& dc,
		wxWindow *window,
		int button,
		int buttonState,
		const wxRect& rect,
		wxAuiPaneInfo& pane);
};

#endif//__S_AUI_TAB_ART_H__
