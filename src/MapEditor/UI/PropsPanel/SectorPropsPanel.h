
#ifndef __SECTOR_PROPS_PANEL_H__
#define __SECTOR_PROPS_PANEL_H__

#include "common.h"
#include "PropsPanelBase.h"
#include "UI/Canvas/OGLCanvas.h"
#include "UI/STabCtrl.h"

class GLTexture;
class FlatTexCanvas : public OGLCanvas
{
private:
	GLTexture*	texture;
	string		texname;

public:
	FlatTexCanvas(wxWindow* parent);
	~FlatTexCanvas();

	string	getTexName();
	void	setTexture(string texture);
	void	draw();
};

class FlatComboBox : public wxComboBox
{
private:
	bool list_down;

public:
	FlatComboBox(wxWindow* parent);
	~FlatComboBox() {}

	void onDropDown(wxCommandEvent& e);
	void onCloseUp(wxCommandEvent& e);
	void onKeyDown(wxKeyEvent& e);
};

class SectorSpecialPanel;
class MapObject;
class MapObjectPropsPanel;
class NumberTextCtrl;
class SectorPropsPanel : public PropsPanelBase
{
private:
	TabControl*				stc_tabs;
	SectorSpecialPanel*		panel_special;
	wxCheckBox*				cb_override_special;
	MapObjectPropsPanel*	mopp_all_props;
	FlatTexCanvas*			gfx_floor;
	FlatTexCanvas*			gfx_ceiling;
	FlatComboBox*			fcb_floor;
	FlatComboBox*			fcb_ceiling;
	NumberTextCtrl*			text_height_floor;
	NumberTextCtrl*			text_height_ceiling;
	NumberTextCtrl*			text_light;
	NumberTextCtrl*			text_tag;
	wxButton*				btn_new_tag;

public:
	SectorPropsPanel(wxWindow* parent);
	~SectorPropsPanel();

	wxPanel*	setupGeneralPanel();
	wxPanel*	setupSpecialPanel();
	void		openObjects(vector<MapObject*>& objects);
	void		applyChanges();

	// Events
	void	onTextureChanged(wxCommandEvent& e);
	void	onTextureClicked(wxMouseEvent& e);
	void	onBtnNewTag(wxCommandEvent& e);
};

#endif//__SECTOR_PROPS_PANEL_H__
