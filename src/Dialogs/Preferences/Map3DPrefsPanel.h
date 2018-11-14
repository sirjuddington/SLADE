#pragma once

#include "PrefsPanelBase.h"

class Map3DPrefsPanel : public PrefsPanelBase
{
public:
	Map3DPrefsPanel(wxWindow* parent);
	~Map3DPrefsPanel();

	void init() override;
	void updateDistanceControls();
	void applyPreferences() override;

	string pageTitle() override { return "Map Editor 3D Mode Settings"; }

private:
	wxSlider*     slider_max_render_dist_;
	wxCheckBox*   cb_distance_unlimited_;
	wxSlider*     slider_max_thing_dist_;
	wxCheckBox*   cb_max_thing_dist_lock_;
	wxCheckBox*   cb_render_dist_adaptive_;
	wxSpinCtrl*   spin_adaptive_fps_;
	wxCheckBox*   cb_render_sky_;
	wxStaticText* label_render_dist_;
	wxStaticText* label_thing_dist_;
	wxCheckBox*   cb_show_distance_;
	wxCheckBox*   cb_invert_y_;
	wxCheckBox*   cb_shade_orthogonal_;

	// Events
	void onSliderMaxRenderDistChanged(wxCommandEvent& e);
	void onSliderMaxThingDistChanged(wxCommandEvent& e);
	void onCBLockThingDistChanged(wxCommandEvent& e);
	void onCBDistUnlimitedChanged(wxCommandEvent& e);
};
