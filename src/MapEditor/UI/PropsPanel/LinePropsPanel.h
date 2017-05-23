
#ifndef __LINE_PROPS_PANEL_H__
#define __LINE_PROPS_PANEL_H__

#include "PropsPanelBase.h"
#include "UI/STabCtrl.h"

class MapObject;
class MapObjectPropsPanel;
class ActionSpecialPanel;
class ArgsPanel;
class SidePropsPanel;
class NumberTextCtrl;
class LinePropsPanel : public PropsPanelBase
{
public:
	LinePropsPanel(wxWindow* parent);
	~LinePropsPanel();

	wxPanel*	setupGeneralTab();
	wxPanel*	setupSpecialTab();
	void		openObjects(vector<MapObject*>& objects) override;
	void		applyChanges() override;

private:
	TabControl*				stc_tabs;
	MapObjectPropsPanel*	mopp_all_props;
	wxCheckBox*				cb_override_special;
	ActionSpecialPanel*		panel_special;
	ArgsPanel*				panel_args;
	SidePropsPanel*			panel_side1;
	SidePropsPanel*			panel_side2;
	NumberTextCtrl*			text_tag;
	wxButton*				btn_new_tag;
	NumberTextCtrl*			text_id;
	wxButton*				btn_new_id;

	struct FlagHolder
	{
		wxCheckBox*	check_box;
		int			index;
		string		udmf;
	};
	vector<FlagHolder>	flags;
};

#endif//__LINE_PROPS_PANEL_H__
