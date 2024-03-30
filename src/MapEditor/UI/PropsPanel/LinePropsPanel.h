#pragma once

#include "PropsPanelBase.h"
#include "UI/Controls/STabCtrl.h"

namespace slade
{
class MapObjectPropsPanel;
class ActionSpecialPanel;
class ArgsPanel;
class SidePropsPanel;
class NumberTextCtrl;

class LinePropsPanel : public PropsPanelBase
{
public:
	LinePropsPanel(wxWindow* parent);
	~LinePropsPanel() override;

	wxPanel* setupGeneralTab();
	wxPanel* setupSpecialTab();
	void     openObjects(vector<MapObject*>& lines) override;
	void     applyChanges() override;

private:
	TabControl*          stc_tabs_            = nullptr;
	MapObjectPropsPanel* mopp_all_props_      = nullptr;
	wxCheckBox*          cb_override_special_ = nullptr;
	ActionSpecialPanel*  panel_special_       = nullptr;
	ArgsPanel*           panel_args_          = nullptr;
	SidePropsPanel*      panel_side1_         = nullptr;
	SidePropsPanel*      panel_side2_         = nullptr;
	NumberTextCtrl*      text_tag_            = nullptr;
	wxButton*            btn_new_tag_         = nullptr;
	NumberTextCtrl*      text_id_             = nullptr;
	wxButton*            btn_new_id_          = nullptr;

	struct FlagHolder
	{
		wxCheckBox* check_box;
		int         index;
		wxString    udmf;
	};
	vector<FlagHolder> flags_;
};
} // namespace slade
