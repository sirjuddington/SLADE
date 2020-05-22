#pragma once

namespace slade
{
class SAction;

class SToolBarButton : public wxControl
{
public:
	SToolBarButton(wxWindow* parent, const wxString& action, const wxString& icon = "", bool show_name = false);
	SToolBarButton(
		wxWindow*       parent,
		const wxString& action_id,
		const wxString& action_name,
		const wxString& icon,
		const wxString& help_text,
		bool            show_name = false);
	~SToolBarButton() = default;

	SAction*        action() const { return action_; }
	const wxString& actionId() const { return action_id_; }
	bool            isChecked() const;
	wxMenu*         menu() const { return menu_dropdown_; }

	void setIcon(const wxString& icon);
	void setChecked(bool checked);
	void setMenu(wxMenu* menu, bool delete_existing = false);

	bool updateState();

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
	State    state_         = State::Normal;
	bool     show_name_     = false;
	wxMenu*  menu_dropdown_ = nullptr;

	// For non-SAction buttons
	wxString action_id_;
	wxString action_name_;
	wxString help_text_;
	bool     checked_ = false;

	// Layout
	int pad_outer_  = 3;
	int pad_inner_  = 1;
	int icon_size_  = 16;
	int text_width_ = 0;

	void sendClickedEvent();
	void updateSize();

	// Events
	void onPaint(wxPaintEvent& e);
	void onMouseEvent(wxMouseEvent& e);
	void onFocus(wxFocusEvent& e);
	void onEraseBackground(wxEraseEvent& e);
};
} // namespace slade

wxDECLARE_EVENT(wxEVT_STOOLBAR_BUTTON_CLICKED, wxCommandEvent);
