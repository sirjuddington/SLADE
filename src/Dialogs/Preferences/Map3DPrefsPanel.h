#pragma once

#include "PrefsPanelBase.h"

class Map3DPrefsPanel : public PrefsPanelBase
{
public:
	Map3DPrefsPanel(wxWindow* parent);
	~Map3DPrefsPanel() = default;

	void init() override;
	void updateDistanceControls() const;
	void applyPreferences() override;

	wxString pageTitle() override { return "Map Editor 3D Mode Settings"; }

private:
	wxSlider*     slider_max_render_dist_  = nullptr;
	wxCheckBox*   cb_distance_unlimited_   = nullptr;
	wxSlider*     slider_max_thing_dist_   = nullptr;
	wxCheckBox*   cb_max_thing_dist_lock_  = nullptr;
	wxCheckBox*   cb_render_dist_adaptive_ = nullptr;
	wxSpinCtrl*   spin_adaptive_fps_       = nullptr;
	wxCheckBox*   cb_render_sky_           = nullptr;
	wxStaticText* label_render_dist_       = nullptr;
	wxStaticText* label_thing_dist_        = nullptr;
	wxCheckBox*   cb_show_distance_        = nullptr;
	wxCheckBox*   cb_invert_y_             = nullptr;
	wxCheckBox*   cb_shade_orthogonal_     = nullptr;
};
