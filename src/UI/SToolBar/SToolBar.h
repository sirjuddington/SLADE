
#ifndef __S_TOOL_BAR_H__
#define __S_TOOL_BAR_H__

#include <wx/panel.h>

class SToolBarButton;
class SToolBar;
class SToolBarGroup : public wxPanel
{
private:
	string	name;
	bool	hidden;

public:
	SToolBarGroup(SToolBar* parent, string name, bool force_name = false);
	~SToolBarGroup();

	string	getName() { return name; }
	bool	isHidden() { return hidden; }
	void	hide(bool hide = true);
	void	redraw();

	SToolBarButton*	addActionButton(string action, string icon = "", bool show_name = false);
	SToolBarButton*	addActionButton(string action_id, string action_name, string icon, string help_text, bool show_name = false);
	void			addCustomControl(wxWindow* control);

	void	onButtonClicked(wxCommandEvent& e);
};

class SToolBar : public wxPanel
{
private:
	vector<SToolBarGroup*>	groups;
	vector<wxWindow*>		separators;
	vector<wxWindow*>		vlines;
	int						min_height;
	int						n_rows;
	bool					draw_border;
	bool					main_toolbar;

public:
	SToolBar(wxWindow* parent, bool main_toolbar = false);
	~SToolBar();

	int		minHeight() { return min_height; }
	void	addGroup(SToolBarGroup* group);
	void	deleteGroup(string name);
	void	deleteCustomGroups();
	void	addActionGroup(string name, wxArrayString actions);
	void	updateLayout(bool force = false, bool generate_event = true);
	void	enableGroup(string name, bool enable = true);
	int		calculateNumRows(int width);
	void	drawBorder(bool draw = true) { draw_border = draw; }
	bool	mainToolbar() { return main_toolbar; }

	// Events
	void	onSize(wxSizeEvent& e);
	void	onPaint(wxPaintEvent& e);
	void	onFocus(wxFocusEvent& e);
	void	onMouseEvent(wxMouseEvent& e);
	void	onContextMenu(wxCommandEvent& e);
	void	onButtonClick(wxCommandEvent& e);
	void	onEraseBackground(wxEraseEvent& e);

	// Static
	static int	getBarHeight();
};

DECLARE_EVENT_TYPE(wxEVT_STOOLBAR_LAYOUT_UPDATED, -1)

#endif//__S_TOOL_BAR_H__
