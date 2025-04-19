#pragma once

#include "PropsPanelBase.h"
#include "UI/Canvas/OGLCanvas.h"
#include "UI/Controls/STabCtrl.h"

namespace slade
{
class NumberTextCtrl;
class MapObjectPropsPanel;
class ArgsPanel;
class ActionSpecialPanel;
namespace game
{
	class ThingType;
}

class SpriteTexCanvas : public OGLCanvas
{
public:
	SpriteTexCanvas(wxWindow* parent);
	~SpriteTexCanvas() = default;

	string texName() const { return texname_; }
	void   setSprite(const game::ThingType& type);
	void   draw() override;

private:
	unsigned texture_ = 0;
	string   texname_;
	ColRGBA  colour_ = ColRGBA::WHITE;
	bool     icon_   = false;
};

class AngleControl;
class ThingDirCanvas : public OGLCanvas
{
public:
	ThingDirCanvas(AngleControl* parent);
	~ThingDirCanvas() = default;

	void setAngle(int angle);
	void draw() override;

	void onMouseEvent(wxMouseEvent& e);

private:
	AngleControl* parent_ = nullptr;
	vector<Vec2d> dir_points_;
	ColRGBA       col_bg_;
	ColRGBA       col_fg_;
	int           point_hl_   = -1;
	int           point_sel_  = -1;
	long          last_check_ = 0;
};


class AngleControl : public wxControl
{
public:
	AngleControl(wxWindow* parent);
	~AngleControl() = default;

	int  angle(int base = 0) const;
	void setAngle(int angle, bool update_visual = true);
	void updateAngle() const;
	bool angleSet() const;

private:
	int             angle_      = 0;
	ThingDirCanvas* dc_angle_   = nullptr;
	NumberTextCtrl* text_angle_ = nullptr;

	void onAngleTextChanged(wxCommandEvent& e);
};


class ThingPropsPanel : public PropsPanelBase
{
public:
	ThingPropsPanel(wxWindow* parent);
	~ThingPropsPanel() = default;

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

	vector<string> udmf_flags_;
	vector<string> udmf_flags_extra_;
	int            type_current_ = 0;

	wxPanel* setupGeneralTab();
	wxPanel* setupExtraFlagsTab();

	void onSpriteClicked(wxMouseEvent& e);
};
} // namespace slade
