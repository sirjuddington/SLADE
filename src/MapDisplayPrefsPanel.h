
#ifndef __MAP_DISPLAY_PREFS_PANEL_H__
#define __MAP_DISPLAY_PREFS_PANEL_H__

#include "PrefsPanelBase.h"
#include <wx/notebook.h>

class MapDisplayPrefsPanel : public PrefsPanelBase
{
private:
	wxNotebook*	nb_pages;

	wxCheckBox*	cb_grid_dashed;
	wxCheckBox*	cb_animate_hilight;
	wxCheckBox*	cb_animate_selection;
	wxCheckBox*	cb_animate_tagged;
	wxChoice*	choice_crosshair;

	wxSlider*	slider_vertex_size;
	wxCheckBox*	cb_vertex_round;
	wxChoice*	choice_vertices_always;

	wxSlider*	slider_line_width;
	wxCheckBox*	cb_line_smooth;
	wxCheckBox*	cb_line_tabs_always;
	wxCheckBox*	cb_line_fade;

	wxChoice*	choice_thing_drawtype;
	wxChoice*	choice_things_always;
	wxCheckBox*	cb_thing_force_dir;
	wxCheckBox*	cb_thing_overlay_square;
	wxSlider*	slider_thing_shadow;

	wxSlider*	slider_flat_brightness;
	wxCheckBox*	cb_flat_ignore_light;
	wxCheckBox*	cb_sector_hilight_fill;
	wxCheckBox*	cb_flat_fade;

public:
	MapDisplayPrefsPanel(wxWindow* parent);
	~MapDisplayPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__MAP_DISPLAY_PREFS_PANEL_H__
