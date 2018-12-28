#pragma once

class SAction;

class SToolBarButton : public wxControl
{
public:
	SToolBarButton(wxWindow* parent, const string& action, const string& icon = "", bool show_name = false);
	SToolBarButton(
		wxWindow*     parent,
		const string& action_id,
		const string& action_name,
		const string& icon,
		const string& help_text,
		bool          show_name = false);
	~SToolBarButton() = default;

	void setIcon(const string& icon);

	static int pixelHeight();

private:
	enum class State
	{
		Normal,
		MouseOver,
		MouseDown,
	};

	SAction* action_ = nullptr;
	wxBitmap icon_;
	State    state_     = State::Normal;
	bool     show_name_ = false;

	// For non-SAction buttons
	string action_id_;
	string action_name_;
	string help_text_;

	// Layout
	int pad_outer_  = 3;
	int pad_inner_  = 1;
	int icon_size_  = 16;
	int text_width_ = 0;

	void sendClickedEvent();

	// Events
	void onPaint(wxPaintEvent& e);
	void onMouseEvent(wxMouseEvent& e);
	void onFocus(wxFocusEvent& e);
	void onEraseBackground(wxEraseEvent& e);
};

wxDECLARE_EVENT(wxEVT_STOOLBAR_BUTTON_CLICKED, wxCommandEvent);
