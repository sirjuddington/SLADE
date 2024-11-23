#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class NumberSlider;

class Map3DSettingsPanel : public SettingsPanel
{
public:
	Map3DSettingsPanel(wxWindow* parent);
	~Map3DSettingsPanel() override = default;

	void loadSettings() override;
	void applySettings() override;

	string title() const override { return "Map Editor 3D Mode Settings"; }

private:
	NumberSlider* slider_max_render_dist_  = nullptr;
	wxCheckBox*   cb_distance_unlimited_   = nullptr;
	NumberSlider* slider_max_thing_dist_   = nullptr;
	wxCheckBox*   cb_max_thing_dist_lock_  = nullptr;
	wxCheckBox*   cb_render_dist_adaptive_ = nullptr;
	wxSpinCtrl*   spin_adaptive_fps_       = nullptr;
	wxCheckBox*   cb_render_sky_           = nullptr;
	wxCheckBox*   cb_show_distance_        = nullptr;
	wxCheckBox*   cb_invert_y_             = nullptr;
	wxCheckBox*   cb_shade_orthogonal_     = nullptr;
	NumberSlider* slider_fov_              = nullptr;
	wxCheckBox*   cb_enable_3d_floors_     = nullptr;

	void updateDistanceControls() const;
};
} // namespace slade::ui
