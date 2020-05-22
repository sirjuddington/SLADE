#pragma once

namespace slade
{
class SToolBarButton;
class SToolBar;

class SToolBarGroup : public wxPanel
{
public:
	SToolBarGroup(SToolBar* parent, const wxString& name, bool force_name = false);
	~SToolBarGroup() = default;

	wxString name() const { return name_; }
	bool     hidden() const { return hidden_; }
	bool     hasCustomControls() const;

	void hide(bool hide = true);
	void redraw();
	void showGroupSeparator(bool show = true) { separator_->Show(show); }

	SToolBarButton* addActionButton(const wxString& action, const wxString& icon = "", bool show_name = false);
	SToolBarButton* addActionButton(
		const wxString& action_id,
		const wxString& action_name,
		const wxString& icon,
		const wxString& help_text,
		bool            show_name = false);
	void addCustomControl(wxWindow* control);
	void addSeparator();

	SToolBarButton* findActionButton(const wxString& action) const;
	void            refreshButtons() const;
	void            setAllButtonsEnabled(bool enable = true);
	void            setAllButtonsChecked(bool check = true);

	void addToMenu(wxMenu& menu);

private:
	wxString      name_;
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
	~SToolBar() = default;

	const vector<SToolBarGroup*>& groups() const { return groups_; }
	bool                          mainToolbar() const { return main_toolbar_; }
	wxOrientation                 orientation() const { return orientation_; }

	SToolBarGroup*  group(const wxString& name);
	void            addGroup(SToolBarGroup* group, bool at_end = false);
	void            deleteGroup(const wxString& name);
	void            deleteCustomGroups();
	void            addActionGroup(const wxString& name, wxArrayString actions, bool at_end = false);
	void            addActionGroup(const wxString& name, const vector<string>& actions, bool at_end = false);
	void            enableGroup(const wxString& name, bool enable = true);
	void            populateGroupsMenu(wxMenu* menu, int start_id = 0);
	void            enableContextMenu(bool enable = true) { enable_context_menu_ = enable; }
	SToolBarButton* findActionButton(const wxString& action_id) const;

	void updateLayout(bool force = false, bool generate_event = true);
	void hideOverflowGroups();
	int  calculateNumRows(int width);

	void drawBorder(bool draw = true) { draw_border_ = draw; }

	// Static
	static int getBarHeight();
	static int scaledButtonSize();

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
