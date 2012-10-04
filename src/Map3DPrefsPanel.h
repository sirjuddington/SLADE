
#ifndef __MAP_3D_PREFS_PANEL_H__
#define __MAP_3D_PREFS_PANEL_H__

#include "PrefsPanelBase.h"
#include <wx/spinctrl.h>

class Map3DPrefsPanel : public PrefsPanelBase {
private:
	wxSlider*		slider_max_render_dist;
	wxCheckBox*		cb_distance_unlimited;
	wxSlider*		slider_max_thing_dist;
	wxCheckBox*		cb_max_thing_dist_lock;
	wxCheckBox*		cb_render_dist_adaptive;
	wxSpinCtrl*		spin_adaptive_fps;
	wxCheckBox*		cb_render_sky;
	wxStaticText*	label_render_dist;
	wxStaticText*	label_thing_dist;

public:
	Map3DPrefsPanel(wxWindow* parent);
	~Map3DPrefsPanel();

	void	init();
	void	updateDistanceControls();
	void	applyPreferences();

	// Events
	void	onSliderMaxRenderDistChanged(wxCommandEvent& e);
	void	onSliderMaxThingDistChanged(wxCommandEvent& e);
	void	onCBLockThingDistChanged(wxCommandEvent& e);
	void	onCBDistUnlimitedChanged(wxCommandEvent& e);
};

#endif//__MAP_3D_PREFS_PANEL_H__
