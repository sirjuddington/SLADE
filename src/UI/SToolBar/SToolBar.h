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
	void     hide(bool hide = true);
	void     redraw();

	SToolBarButton* addActionButton(const wxString& action, const wxString& icon = "", bool show_name = false);
	SToolBarButton* addActionButton(
		const wxString& action_id,
		const wxString& action_name,
		const wxString& icon,
		const wxString& help_text,
		bool            show_name = false);
	void addCustomControl(wxWindow* control);

	SToolBarButton* findActionButton(const wxString& action) const;
	void            refreshButtons() const;
	void            setAllButtonsEnabled(bool enable = true);

private:
	wxString                name_;
	bool                    hidden_      = false;
	wxOrientation           orientation_ = wxHORIZONTAL;
	vector<SToolBarButton*> buttons_;

	void onButtonClicked(wxCommandEvent& e);
};

class SToolBar : public wxPanel
{
public:
	SToolBar(wxWindow* parent, bool main_toolbar = false, wxOrientation orientation = wxHORIZONTAL);
	~SToolBar() = default;

	const vector<SToolBarGroup*>& groups() const { return groups_; }
	// int                           minHeight() const { return min_height_; }
	bool          mainToolbar() const { return main_toolbar_; }
	wxOrientation orientation() const { return orientation_; }

	SToolBarGroup*  group(const wxString& name);
	void            addGroup(SToolBarGroup* group, bool at_end = false);
	void            deleteGroup(const wxString& name);
	void            deleteCustomGroups();
	void            addActionGroup(const wxString& name, wxArrayString actions);
	void            enableGroup(const wxString& name, bool enable = true);
	void            populateGroupsMenu(wxMenu* menu, int start_id = 0);
	void            enableContextMenu(bool enable = true) { enable_context_menu_ = enable; }
	SToolBarButton* findActionButton(const wxString& action_id) const;

	void updateLayout(bool force = false, bool generate_event = true);
	int  calculateNumRows(int width);

	void drawBorder(bool draw = true) { draw_border_ = draw; }

	// Static
	static int getBarHeight();

private:
	vector<SToolBarGroup*> groups_;
	vector<SToolBarGroup*> groups_end_;
	vector<wxWindow*>      separators_;
	bool                   draw_border_         = true;
	bool                   main_toolbar_        = false;
	bool                   enable_context_menu_ = false;
	wxOrientation          orientation_         = wxHORIZONTAL;

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
