#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
struct LayoutHelper;
class RadioButtonPanel;
class NumberSlider;

class MapDisplaySettingsPanel : public SettingsPanel
{
public:
	MapDisplaySettingsPanel(wxWindow* parent);
	~MapDisplaySettingsPanel() override = default;

	string title() const override { return "Map Editor Display Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	wxCheckBox*       cb_grid_dashed_       = nullptr;
	RadioButtonPanel* rbp_grid_64_          = nullptr;
	wxCheckBox*       cb_grid_show_origin_  = nullptr;
	wxCheckBox*       cb_animate_hilight_   = nullptr;
	wxCheckBox*       cb_animate_selection_ = nullptr;
	wxCheckBox*       cb_animate_tagged_    = nullptr;
	RadioButtonPanel* rbp_crosshair_        = nullptr;
	wxCheckBox*       cb_action_lines_      = nullptr;
	wxCheckBox*       cb_show_help_         = nullptr;
	wxChoice*         choice_tex_filter_    = nullptr;

	NumberSlider*     slider_vertex_size_  = nullptr;
	wxCheckBox*       cb_vertex_round_     = nullptr;
	RadioButtonPanel* rbp_vertices_always_ = nullptr;

	NumberSlider* slider_line_width_   = nullptr;
	wxCheckBox*   cb_line_smooth_      = nullptr;
	wxCheckBox*   cb_line_tabs_always_ = nullptr;
	wxCheckBox*   cb_line_fade_        = nullptr;

	RadioButtonPanel* rbp_thing_shape_         = nullptr;
	RadioButtonPanel* rbp_things_always_       = nullptr;
	wxCheckBox*       cb_thing_sprites_        = nullptr;
	wxCheckBox*       cb_thing_force_dir_      = nullptr;
	wxCheckBox*       cb_thing_overlay_square_ = nullptr;
	NumberSlider*     slider_thing_shadow_     = nullptr;
	wxCheckBox*       cb_use_zeth_icons_       = nullptr;
	wxSlider*         slider_halo_width_       = nullptr;
	NumberSlider*     slider_light_intensity_  = nullptr;

	NumberSlider* slider_flat_brightness_  = nullptr;
	wxCheckBox*   cb_flat_ignore_light_    = nullptr;
	wxCheckBox*   cb_sector_hilight_fill_  = nullptr;
	wxCheckBox*   cb_flat_fade_            = nullptr;
	wxCheckBox*   cb_sector_selected_fill_ = nullptr;

	wxPanel* createGeneralPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* createVerticesPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* createLinesPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* createThingsPanel(wxWindow* parent, const LayoutHelper& lh);
	wxPanel* createSectorsPanel(wxWindow* parent, const LayoutHelper& lh);
};
} // namespace slade::ui
