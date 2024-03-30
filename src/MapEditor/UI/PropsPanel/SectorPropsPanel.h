#pragma once

#include "PropsPanelBase.h"
#include "UI/Controls/STabCtrl.h"

namespace slade
{
class SectorSpecialPanel;
class MapObjectPropsPanel;
class NumberTextCtrl;
class FlatTexCanvas;
class FlatComboBox;

class SectorPropsPanel : public PropsPanelBase
{
public:
	SectorPropsPanel(wxWindow* parent);
	~SectorPropsPanel() override = default;

	void openObjects(vector<MapObject*>& objects) override;
	void applyChanges() override;

private:
	TabControl*          stc_tabs_            = nullptr;
	SectorSpecialPanel*  panel_special_       = nullptr;
	wxCheckBox*          cb_override_special_ = nullptr;
	MapObjectPropsPanel* mopp_all_props_      = nullptr;
	FlatTexCanvas*       gfx_floor_           = nullptr;
	FlatTexCanvas*       gfx_ceiling_         = nullptr;
	FlatComboBox*        fcb_floor_           = nullptr;
	FlatComboBox*        fcb_ceiling_         = nullptr;
	NumberTextCtrl*      text_height_floor_   = nullptr;
	NumberTextCtrl*      text_height_ceiling_ = nullptr;
	NumberTextCtrl*      text_light_          = nullptr;
	NumberTextCtrl*      text_tag_            = nullptr;
	wxButton*            btn_new_tag_         = nullptr;

	wxPanel* setupGeneralPanel();
	wxPanel* setupSpecialPanel();

	// Events
	void onTextureChanged(wxCommandEvent& e);
	void onTextureClicked(wxMouseEvent& e);
	void onBtnNewTag(wxCommandEvent& e);
};
} // namespace slade
