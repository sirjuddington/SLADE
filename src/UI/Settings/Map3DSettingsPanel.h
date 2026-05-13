#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class RadioButtonPanel;
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
	NumberSlider*     slider_max_render_dist_ = nullptr;
	wxCheckBox*       cb_distance_unlimited_  = nullptr;
	wxCheckBox*       cb_show_distance_       = nullptr;
	wxCheckBox*       cb_shade_orthogonal_    = nullptr;
	NumberSlider*     slider_fov_             = nullptr;
	NumberSlider*     slider_max_lights_      = nullptr;
	RadioButtonPanel* rbp_highlight_          = nullptr;

	void updateDistanceControls() const;
};
} // namespace slade::ui
