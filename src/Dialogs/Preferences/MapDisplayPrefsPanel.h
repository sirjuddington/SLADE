#pragma once

#include "PrefsPanelBase.h"
#include "UI/Controls/STabCtrl.h"

class STabCtrl;
class MapDisplayPrefsPanel : public PrefsPanelBase
{
public:
	MapDisplayPrefsPanel(wxWindow* parent);
	~MapDisplayPrefsPanel();

	void	setupGeneralTab();
	void	setupVerticesTab();
	void	setupLinesTab();
	void	setupThingsTab();
	void	setupFlatsTab();
	void	init() override;
	void	applyPreferences() override;

	string pageTitle() override { return "Map Editor Display Settings"; }

private:
	TabControl*	stc_pages_;

	wxCheckBox*	cb_grid_dashed_;
	wxChoice*	choice_grid_64_;
	wxCheckBox* cb_grid_show_origin_;
	wxCheckBox*	cb_animate_hilight_;
	wxCheckBox*	cb_animate_selection_;
	wxCheckBox*	cb_animate_tagged_;
	wxChoice*	choice_crosshair_;
	wxCheckBox*	cb_action_lines_;
	wxCheckBox*	cb_show_help_;
	wxChoice*	choice_tex_filter_;

	wxSlider*	slider_vertex_size_;
	wxCheckBox*	cb_vertex_round_;
	wxChoice*	choice_vertices_always_;

	wxSlider*	slider_line_width_;
	wxCheckBox*	cb_line_smooth_;
	wxCheckBox*	cb_line_tabs_always_;
	wxCheckBox*	cb_line_fade_;

	wxChoice*	choice_thing_drawtype_;
	wxChoice*	choice_things_always_;
	wxCheckBox*	cb_thing_force_dir_;
	wxCheckBox*	cb_thing_overlay_square_;
	wxCheckBox*	cb_thing_arrow_colour_;
	wxSlider*	slider_thing_shadow_;
	wxSlider*	slider_thing_arrow_alpha_;
	wxCheckBox*	cb_use_zeth_icons_;
	wxSlider*	slider_halo_width_;

	wxSlider*	slider_flat_brightness_;
	wxCheckBox*	cb_flat_ignore_light_;
	wxCheckBox*	cb_sector_hilight_fill_;
	wxCheckBox*	cb_flat_fade_;
	wxCheckBox*	cb_sector_selected_fill_;
};
