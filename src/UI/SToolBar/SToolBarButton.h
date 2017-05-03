
#ifndef __S_TOOL_BAR_BUTTON_H__
#define __S_TOOL_BAR_BUTTON_H__

#include "common.h"

class SAction;
class SToolBarButton : public wxControl
{
private:
	SAction*	action;
	wxBitmap	icon;
	int			state;
	bool		show_name;

	// For non-SAction buttons
	string	action_id;
	string	action_name;
	string	help_text;

public:
	SToolBarButton(wxWindow* parent, string action, string icon = "", bool show_name = false);
	SToolBarButton(wxWindow* parent, string action_id, string action_name, string icon, string help_text, bool show_name = false);
	~SToolBarButton();

	void setIcon(string icon);

	enum
	{
	    STATE_NORMAL,
	    STATE_MOUSEOVER,
	    STATE_MOUSEDOWN,
	};

	void	sendClickedEvent();

	// Events
	void	onPaint(wxPaintEvent& e);
	void	onMouseEvent(wxMouseEvent& e);
	void	onFocus(wxFocusEvent& e);
	void	onEraseBackground(wxEraseEvent& e);
};

wxDECLARE_EVENT(wxEVT_STOOLBAR_BUTTON_CLICKED, wxCommandEvent);

#endif//__S_TOOL_BAR_BUTTON_H__
