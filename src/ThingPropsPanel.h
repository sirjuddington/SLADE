
#ifndef __THING_PROPS_PANEL_H__
#define __THING_PROPS_PANEL_H__

#include "PropsPanelBase.h"
#include <OGLCanvas.h>

class GLTexture;
class ThingType;
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
	void	setSprite(ThingType* type);
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

class wxNotebook;
class MapObjectPropsPanel;
class ArgsPanel;
class ActionSpecialPanel;
class NumberTextCtrl;
class ThingPropsPanel : public PropsPanelBase
{
private:
	wxNotebook*				nb_tabs;
	MapObjectPropsPanel*	mopp_other_props;
	vector<wxCheckBox*>		cb_flags;
	vector<wxCheckBox*>		cb_flags_extra;
	ArgsPanel*				panel_args;
	SpriteTexCanvas*		gfx_sprite;
	wxStaticText*			label_type;
	ActionSpecialPanel*		panel_special;
	ThingDirCanvas*			gfx_direction;
	NumberTextCtrl*			text_direction;
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
	void	onDirectionClicked(wxMouseEvent& e);
	void	onDirectionTextChanged(wxCommandEvent& e);
};

#endif//__THING_PROPS_PANEL_H__
