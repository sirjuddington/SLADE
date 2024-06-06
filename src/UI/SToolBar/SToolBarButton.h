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
		bool            show_name = false,
		int             icon_size = -1);
	~SToolBarButton() override = default;

	SAction*        action() const { return action_; }
	const wxString& actionId() const { return action_id_; }
	bool            isChecked() const;
	wxMenu*         menu() const { return menu_dropdown_; }

	void setIcon(const wxString& icon);
	void setChecked(bool checked);
	void setMenu(wxMenu* menu, bool delete_existing = false);
	void setFontSize(float scale);
	void setPadding(int inner, int outer = 1);
	void setExactFit(bool fit);

	bool updateState(int mouse_event = 0);

	static int pixelHeight();

private:
	enum class State
	{
		Normal,
		MouseOver,
		MouseDown,
	};

	SAction*       action_ = nullptr;
	wxBitmapBundle icon_;
	State          state_             = State::Normal;
	bool           show_name_         = false;
	wxMenu*        menu_dropdown_     = nullptr;
	bool           last_draw_enabled_ = true; // True if the button was enabled last time it was drawn

	// For non-SAction buttons
	wxString action_id_;
	wxString action_name_;
	wxString help_text_;
	bool     checked_ = false;

	// Layout
	int  pad_outer_  = 3;
	int  pad_inner_  = 1;
	int  icon_size_  = 16;
	int  text_width_ = 0;
	bool exact_fit_  = true;

	void setup(bool show_name, string_view icon);
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
