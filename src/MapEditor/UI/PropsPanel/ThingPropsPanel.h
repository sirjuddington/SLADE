
#ifndef __THING_PROPS_PANEL_H__
#define __THING_PROPS_PANEL_H__

#include "common.h"
#include "PropsPanelBase.h"
#include "UI/Canvas/OGLCanvas.h"
#include "UI/STabCtrl.h"

class GLTexture;
namespace Game { class ThingType; }

class SpriteTexCanvas : public OGLCanvas
{
private:
	GLTexture*	texture;
	string		texname;
	rgba_t		col;
	bool		icon;

public:
	SpriteTexCanvas(wxWindow* parent);
	~SpriteTexCanvas();

	string	getTexName();
	void	setSprite(const Game::ThingType& type);
	void	draw();
};

class ThingDirCanvas : public OGLCanvas
{
private:
	int					angle;
	vector<fpoint2_t>	dir_points;
	rgba_t				col_bg;
	rgba_t				col_fg;
	int					point_hl;
	int					point_sel;
	long				last_check;

public:
	ThingDirCanvas(wxWindow* parent);
	~ThingDirCanvas() {}

	int		getAngle() { return angle; }
	void	setAngle(int angle);
	void	draw();

	void	onMouseEvent(wxMouseEvent& e);
};

class NumberTextCtrl;
class AngleControl : public wxControl
{
private:
	int				angle;
	wxRadioButton*	rb_angles[8];
	NumberTextCtrl*	text_angle;

public:
	AngleControl(wxWindow* parent);
	~AngleControl();

	int		getAngle(int base = 0);
	void	setAngle(int angle);
	void	updateAngle();
	bool	angleSet();

	void	onAngleButtonClicked(wxCommandEvent& e);
	void	onAngleTextChanged(wxCommandEvent& e);
};

class MapObjectPropsPanel;
class ArgsPanel;
class ActionSpecialPanel;
class ThingPropsPanel : public PropsPanelBase
{
private:
	TabControl*				stc_tabs;
	MapObjectPropsPanel*	mopp_other_props;
	vector<wxCheckBox*>		cb_flags;
	vector<wxCheckBox*>		cb_flags_extra;
	ArgsPanel*				panel_args;
	SpriteTexCanvas*		gfx_sprite;
	wxStaticText*			label_type;
	ActionSpecialPanel*		panel_special;
	AngleControl*			ac_direction;
	NumberTextCtrl*			text_id;
	NumberTextCtrl*			text_height;

	vector<string>	udmf_flags;
	vector<string>	udmf_flags_extra;
	int				type_current;

public:
	ThingPropsPanel(wxWindow* parent);
	~ThingPropsPanel();

	wxPanel*	setupGeneralTab();
	wxPanel*	setupExtraFlagsTab();

	void	openObjects(vector<MapObject*>& objects);
	void	applyChanges();

	void	onSpriteClicked(wxMouseEvent& e);
};

#endif//__THING_PROPS_PANEL_H__
