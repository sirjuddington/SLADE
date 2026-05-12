#pragma once

namespace slade
{
class SAction;

class SToolButton : public wxControl
{
public:
	SToolButton(wxWindow* parent, const string& action, const string& icon = "", bool show_name = false);
	SToolButton(
		wxWindow*     parent,
		const string& action_id,
		const string& action_name,
		const string& icon,
		const string& help_text,
		bool          show_name = false,
		int           icon_size = -1);
	~SToolButton() override = default;

	SAction*      action() const { return action_; }
	const string& actionId() const { return action_id_; }
	const string& actionName() const { return action_name_; }
	bool          isChecked() const;
	wxMenu*       menu() const { return menu_dropdown_; }

	void setIcon(const string& icon);
	void setChecked(bool checked);
	void setMenu(wxMenu* menu, bool delete_existing = false);
	void setFontSize(float scale);
	void setPadding(int inner, int outer = 1);
	void setExactFit(bool fit);
	void setTextOffset(int offset);
	void setFillChecked(bool fill) { fill_checked_ = fill; }

	static int pixelHeight();

protected:
	SAction*       action_ = nullptr;
	wxBitmapBundle icon_;
	bool           pressed_             = false;
	bool           show_name_           = false;
	wxMenu*        menu_dropdown_       = nullptr;
	bool           menu_open_           = false;
	int            action_wx_id_offset_ = 0;
	bool           fill_checked_        = false;

	// For non-SAction buttons
	string action_id_;
	string action_name_;
	string help_text_;
	bool   checked_ = false;

	// Layout
	int  pad_outer_   = 3;
	int  pad_inner_   = 1;
	int  icon_size_   = 16;
	int  text_width_  = 0;
	int  text_offset_ = 0; // Space between icon and text
	bool exact_fit_   = true;

	// Recent file buttons on the start page can potentially be deleted after
	// being clicked (as the recent file list is updated). This is needed to
	// avoid crashes due to the button being deleted while still executing the
	// mouse event handler.
	bool click_can_delete_ = false;

	void         setup(bool show_name, string_view icon);
	void         sendClickedEvent();
	void         updateSize();
	virtual void drawContent(wxGraphicsContext* gc, bool mouse_over);

	// Events
	void onPaint(wxPaintEvent& e);
	void onMouseEvent(wxMouseEvent& e);
	void onEraseBackground(wxEraseEvent& e);
};
} // namespace slade

wxDECLARE_EVENT(wxEVT_STOOLBAR_BUTTON_CLICKED, wxCommandEvent);
