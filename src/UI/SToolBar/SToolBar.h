#pragma once

class SToolBarButton;
class SToolBar;

class SToolBarGroup : public wxPanel
{
public:
	SToolBarGroup(SToolBar* parent, const string& name, bool force_name = false);
	~SToolBarGroup() = default;

	string name() const { return name_; }
	bool   hidden() const { return hidden_; }
	void   hide(bool hide = true);
	void   redraw();

	SToolBarButton* addActionButton(const string& action, const string& icon = "", bool show_name = false);
	SToolBarButton* addActionButton(
		const string& action_id,
		const string& action_name,
		const string& icon,
		const string& help_text,
		bool          show_name = false);
	void addCustomControl(wxWindow* control);

	void onButtonClicked(wxCommandEvent& e);

private:
	string name_;
	bool   hidden_;
};

class SToolBar : public wxPanel
{
public:
	SToolBar(wxWindow* parent, bool main_toolbar = false);
	~SToolBar() = default;

	const vector<SToolBarGroup*>& groups() const { return groups_; }
	int                           minHeight() const { return min_height_; }
	bool                          mainToolbar() const { return main_toolbar_; }

	SToolBarGroup* group(const string& name);
	void           addGroup(SToolBarGroup* group);
	void           deleteGroup(const string& name);
	void           deleteCustomGroups();
	void           addActionGroup(const string& name, wxArrayString actions);
	void           enableGroup(const string& name, bool enable = true);
	void           populateGroupsMenu(wxMenu* menu, int start_id = 0);
	void           enableContextMenu(bool enable = true) { enable_context_menu_ = enable; }

	void updateLayout(bool force = false, bool generate_event = true);
	int  calculateNumRows(int width);

	void drawBorder(bool draw = true) { draw_border_ = draw; }

	// Static
	static int getBarHeight();

private:
	vector<SToolBarGroup*> groups_;
	vector<wxWindow*>      separators_;
	vector<wxWindow*>      vlines_;
	int                    min_height_          = 0;
	int                    n_rows_              = 0;
	bool                   draw_border_         = true;
	bool                   main_toolbar_        = false;
	bool                   enable_context_menu_ = false;

	// Events
	void onSize(wxSizeEvent& e);
	void onPaint(wxPaintEvent& e);
	void onFocus(wxFocusEvent& e);
	void onMouseEvent(wxMouseEvent& e);
	void onContextMenu(wxCommandEvent& e);
	void onButtonClick(wxCommandEvent& e);
	void onEraseBackground(wxEraseEvent& e);
};

DECLARE_EVENT_TYPE(wxEVT_STOOLBAR_LAYOUT_UPDATED, -1)
