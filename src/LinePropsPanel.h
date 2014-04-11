
#ifndef __LINE_PROPS_PANEL_H__
#define __LINE_PROPS_PANEL_H__

#include <wx/notebook.h>

class MapObject;
class MapObjectPropsPanel;
class ActionSpecialPanel;
class ArgsPanel;
class SidePropsPanel;
class LinePropsPanel : public wxPanel
{
private:
	wxNotebook*				nb_tabs;
	vector<wxCheckBox*>		cb_flags;
	MapObjectPropsPanel*	mopp_all_props;
	wxCheckBox*				cb_override_special;
	ActionSpecialPanel*		panel_special;
	ArgsPanel*				panel_args;
	SidePropsPanel*			panel_side1;
	SidePropsPanel*			panel_side2;
	wxTextCtrl*				text_tag;
	wxButton*				btn_new_tag;

	vector<string>	udmf_flags;

public:
	LinePropsPanel(wxWindow* parent);
	~LinePropsPanel();

	wxPanel*	setupGeneralTab();
	wxPanel*	setupSpecialTab();
	void		openLines(vector<MapObject*>& lines);
	void		applyChanges(vector<MapObject*>& lines);

	void	onOverrideSpecialChecked(wxCommandEvent& e);
	void	onBtnNewTag(wxCommandEvent& e);
};

#endif//__LINE_PROPS_PANEL_H__
