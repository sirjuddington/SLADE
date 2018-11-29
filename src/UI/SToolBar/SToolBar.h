#pragma once

class SToolBarButton;
class SToolBar;

class SToolBarGroup : public wxPanel
{
public:
	SToolBarGroup(SToolBar* parent, string name, bool force_name = false);
	~SToolBarGroup();

	string name() const { return name_; }
	bool   hidden() const { return hidden_; }
	void   hide(bool hide = true);
	void   redraw();

	SToolBarButton* addActionButton(string action, string icon = "", bool show_name = false);
	SToolBarButton* addActionButton(
		string action_id,
		string action_name,
		string icon,
		string help_text,
		bool   show_name = false);
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
	~SToolBar();

	const vector<SToolBarGroup*>& groups() const { return groups_; }
	int                           minHeight() const { return min_height_; }
	bool                          mainToolbar() const { return main_toolbar_; }

	SToolBarGroup* group(const string& name);
	void           addGroup(SToolBarGroup* group);
	void           deleteGroup(string name);
	void           deleteCustomGroups();
	void           addActionGroup(string name, wxArrayString actions);
	void           enableGroup(string name, bool enable = true);
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
	int                    min_height_;
	int                    n_rows_;
	bool                   draw_border_;
	bool                   main_toolbar_;
	bool                   enable_context_menu_;

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
