#pragma once

#include "PropsPanelBase.h"
#include "UI/Canvas/OGLCanvas.h"
#include "UI/Controls/STabCtrl.h"

class GLTexture;
namespace Game { class ThingType; }
class NumberTextCtrl;
class MapObjectPropsPanel;
class ArgsPanel;
class ActionSpecialPanel;

class SpriteTexCanvas : public OGLCanvas
{
public:
	SpriteTexCanvas(wxWindow* parent);
	~SpriteTexCanvas() {}

	string	texName() const { return texname_; }
	void	setSprite(const Game::ThingType& type);
	void	draw() override;

private:
	GLTexture*	texture_	= nullptr;
	string		texname_;
	rgba_t		colour_		= COL_WHITE;
	bool		icon_		= false;
};

class ThingDirCanvas : public OGLCanvas
{
public:
	ThingDirCanvas(wxWindow* parent);
	~ThingDirCanvas() {}

	int		angle() const { return angle_; }
	void	setAngle(int angle);
	void	draw() override;

	void	onMouseEvent(wxMouseEvent& e);
	
private:
	int					angle_;
	vector<fpoint2_t>	dir_points_;
	rgba_t				col_bg_;
	rgba_t				col_fg_;
	int					point_hl_;
	int					point_sel_;
	long				last_check_;
};


class AngleControl : public wxControl
{
public:
	AngleControl(wxWindow* parent);
	~AngleControl() {}

	int		angle(int base = 0);
	void	setAngle(int angle);
	void	updateAngle();
	bool	angleSet();

private:
	int				angle_;
	wxRadioButton*	rb_angles_[8];
	NumberTextCtrl*	text_angle_;

	void	onAngleButtonClicked(wxCommandEvent& e);
	void	onAngleTextChanged(wxCommandEvent& e);
};


class ThingPropsPanel : public PropsPanelBase
{
public:
	ThingPropsPanel(wxWindow* parent);
	~ThingPropsPanel() {}

	void	openObjects(vector<MapObject*>& objects) override;
	void	applyChanges() override;

private:
	TabControl*				stc_tabs_			= nullptr;
	MapObjectPropsPanel*	mopp_other_props_	= nullptr;
	vector<wxCheckBox*>		cb_flags_;
	vector<wxCheckBox*>		cb_flags_extra_;
	ArgsPanel*				panel_args_			= nullptr;
	SpriteTexCanvas*		gfx_sprite_			= nullptr;
	wxStaticText*			label_type_			= nullptr;
	ActionSpecialPanel*		panel_special_		= nullptr;
	AngleControl*			ac_direction_		= nullptr;
	NumberTextCtrl*			text_id_			= nullptr;
	NumberTextCtrl*			text_height_		= nullptr;

	vector<string>	udmf_flags_;
	vector<string>	udmf_flags_extra_;
	int				type_current_		= 0;

	wxPanel*	setupGeneralTab();
	wxPanel*	setupExtraFlagsTab();

	void	onSpriteClicked(wxMouseEvent& e);
};
