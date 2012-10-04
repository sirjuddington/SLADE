
#ifndef __S_TOOL_BAR_BUTTON_H__
#define __S_TOOL_BAR_BUTTON_H__

class SAction;
class SToolBarButton : public wxControl {
private:
	//string		action;
	SAction*	action;
	wxBitmap	icon;
	int			state;

public:
	SToolBarButton(wxWindow* parent, string action, string icon = "");
	~SToolBarButton();

	enum {
		STATE_NORMAL,
		STATE_MOUSEOVER,
		STATE_MOUSEDOWN,
	};

	// Events
	void	onPaint(wxPaintEvent& e);
	void	onMouseEvent(wxMouseEvent& e);
	void	onFocus(wxFocusEvent& e);
	void	onEraseBackground(wxEraseEvent& e);
};

#endif//__S_TOOL_BAR_BUTTON_H__
