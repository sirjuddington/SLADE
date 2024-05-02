#pragma once

#include "PropsPanelBase.h"
#include "UI/Controls/STabCtrl.h"

namespace slade
{
class NumberTextCtrl;
class MapObjectPropsPanel;
class ArgsPanel;
class ActionSpecialPanel;
class AngleControl;
class SpriteTexCanvas;
class ThingDirCanvas;
namespace game
{
	class ThingType;
}

class ThingPropsPanel : public PropsPanelBase
{
public:
	ThingPropsPanel(wxWindow* parent);
	~ThingPropsPanel() override = default;

	void openObjects(vector<MapObject*>& objects) override;
	void applyChanges() override;

private:
	TabControl*          stc_tabs_         = nullptr;
	MapObjectPropsPanel* mopp_other_props_ = nullptr;
	vector<wxCheckBox*>  cb_flags_;
	vector<wxCheckBox*>  cb_flags_extra_;
	ArgsPanel*           panel_args_    = nullptr;
	SpriteTexCanvas*     gfx_sprite_    = nullptr;
	wxStaticText*        label_type_    = nullptr;
	ActionSpecialPanel*  panel_special_ = nullptr;
	AngleControl*        ac_direction_  = nullptr;
	NumberTextCtrl*      text_id_       = nullptr;
	NumberTextCtrl*      text_height_   = nullptr;
	wxButton*            btn_new_id_    = nullptr;

	vector<wxString> udmf_flags_;
	vector<wxString> udmf_flags_extra_;
	int              type_current_ = 0;

	wxPanel* setupGeneralTab();
	wxPanel* setupExtraFlagsTab();

	void onSpriteClicked(wxMouseEvent& e);
};
} // namespace slade
