#pragma once

#include "PropsPanelBase.h"
#include "UI/Canvas/OGLCanvas.h"
#include "UI/Controls/STabCtrl.h"

class SectorSpecialPanel;
class MapObject;
class MapObjectPropsPanel;
class NumberTextCtrl;

class FlatTexCanvas : public OGLCanvas
{
public:
	FlatTexCanvas(wxWindow* parent);
	~FlatTexCanvas() = default;

	wxString texName() const { return texname_; }
	void     setTexture(const wxString& texture);
	void     draw() override;

private:
	unsigned texture_ = 0;
	wxString texname_;
};

class FlatComboBox : public wxComboBox
{
public:
	FlatComboBox(wxWindow* parent);
	~FlatComboBox() = default;

private:
	bool list_down_ = false;

	void onDropDown(wxCommandEvent& e);
	void onCloseUp(wxCommandEvent& e);
	void onKeyDown(wxKeyEvent& e);
};


class SectorPropsPanel : public PropsPanelBase
{
public:
	SectorPropsPanel(wxWindow* parent);
	~SectorPropsPanel() = default;

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
