#pragma once

namespace slade
{
class SToolBarButton;
class SToolBar;

class SToolBarGroup : public wxPanel
{
public:
	SToolBarGroup(SToolBar* parent, const string& name, bool force_name = false);
	~SToolBarGroup() override = default;

	string name() const { return name_; }
	bool   hidden() const { return hidden_; }
	bool   hasCustomControls() const;

	void hide(bool hide = true);
	void redraw();
	void showGroupSeparator(bool show = true) const { separator_->Show(show); }

	SToolBarButton* addActionButton(const string& action, const string& icon = "", bool show_name = false);
	SToolBarButton* addActionButton(
		const string& action_id,
		const string& action_name,
		const string& icon,
		const string& help_text,
		bool          show_name = false);
	void addCustomControl(wxWindow* control);
	void addSeparator();

	SToolBarButton* findActionButton(const string& action) const;
	void            refreshButtons() const;
	void            setAllButtonsEnabled(bool enable = true);
	void            setAllButtonsChecked(bool check = true);

	void addToMenu(wxMenu& menu) const;

private:
	string        name_;
	bool          hidden_      = false;
	wxOrientation orientation_ = wxHORIZONTAL;
	wxWindow*     separator_   = nullptr;

	struct GroupItem
	{
		enum class Type
		{
			Button,
			Separator,
			CustomControl
		};

		Type      type;
		wxWindow* control;
	};
	vector<GroupItem> items_;

	void onButtonClicked(wxCommandEvent& e);
};

class SToolBar : public wxPanel
{
public:
	SToolBar(wxWindow* parent, bool main_toolbar = false, wxOrientation orientation = wxHORIZONTAL);
	~SToolBar() override = default;

	const vector<SToolBarGroup*>& groups() const { return groups_; }
	bool                          mainToolbar() const { return main_toolbar_; }
	wxOrientation                 orientation() const { return orientation_; }

	SToolBarGroup*  group(const string& name) const;
	void            addGroup(SToolBarGroup* group, bool at_end = false);
	void            deleteGroup(const string& name);
	void            deleteCustomGroups();
	void            addActionGroup(const string& name, const vector<string>& actions, bool at_end = false);
	void            enableGroup(const string& name, bool enable = true);
	void            populateGroupsMenu(wxMenu* menu, int start_id = 0) const;
	void            enableContextMenu(bool enable = true) { enable_context_menu_ = enable; }
	SToolBarButton* findActionButton(const string& action_id) const;

	void updateLayout(bool force = false, bool generate_event = true);
	void hideOverflowGroups();
	int  calculateNumRows(int width) const;

	void drawBorder(bool draw = true) { draw_border_ = draw; }

	static int getBarHeight(const wxWindow* window);
	static int scaledButtonSize(const wxWindow* window);

private:
	vector<SToolBarGroup*> groups_;
	vector<SToolBarGroup*> groups_end_;
	bool                   draw_border_         = true;
	bool                   main_toolbar_        = false;
	bool                   enable_context_menu_ = false;
	wxOrientation          orientation_         = wxHORIZONTAL;
	SToolBarButton*        btn_overflow_        = nullptr;

	// Events
	void onSize(wxSizeEvent& e);
	void onPaint(wxPaintEvent& e);
	void onFocus(wxFocusEvent& e);
	void onMouseEvent(wxMouseEvent& e);
	void onContextMenu(wxCommandEvent& e);
	void onButtonClick(wxCommandEvent& e);
	void onEraseBackground(wxEraseEvent& e);
};
} // namespace slade

DECLARE_EVENT_TYPE(wxEVT_STOOLBAR_LAYOUT_UPDATED, -1)
