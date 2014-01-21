
#include "Main.h"
#include "WxStuff.h"
#include "Map3DPrefsPanel.h"
#include <wx/gbsizer.h>
#include <wx/statline.h>


EXTERN_CVAR(Float, render_max_dist)
EXTERN_CVAR(Float, render_max_thing_dist)
//CVAR(Int, render_thing_icon_size, 16, CVAR_SAVE)
//CVAR(Bool, render_fog_quality, true, CVAR_SAVE)
EXTERN_CVAR(Bool, render_max_dist_adaptive)
EXTERN_CVAR(Int, render_adaptive_ms)
EXTERN_CVAR(Bool, render_3d_sky)
//CVAR(Int, render_3d_things, 1, CVAR_SAVE)
//CVAR(Int, render_3d_hilight, 1, CVAR_SAVE)

Map3DPrefsPanel::Map3DPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Map Editor 3D Mode Preferences");
	wxStaticBoxSizer* fsizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(fsizer, 1, wxEXPAND|wxALL, 4);

	wxGridBagSizer* gbsizer = new wxGridBagSizer(4, 4);
	fsizer->Add(gbsizer, 0, wxEXPAND|wxALL, 4);

	// Render distance
	gbsizer->Add(new wxStaticText(this, -1, "Render distance:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_max_render_dist = new wxSlider(this, -1, 1, 1, 20, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gbsizer->Add(slider_max_render_dist, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);
	label_render_dist = new wxStaticText(this, -1, "00000");
	label_render_dist->SetInitialSize(wxSize(label_render_dist->GetSize().x, -1));
	gbsizer->Add(label_render_dist, wxGBPosition(0, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	cb_distance_unlimited = new wxCheckBox(this, -1, "Unlimited");
	gbsizer->Add(cb_distance_unlimited, wxGBPosition(0, 3), wxDefaultSpan, wxEXPAND);

	// Thing Render distance
	gbsizer->Add(new wxStaticText(this, -1, "Thing render distance:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_max_thing_dist = new wxSlider(this, -1, 1, 1, 20, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gbsizer->Add(slider_max_thing_dist, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	label_thing_dist = new wxStaticText(this, -1, "00000");
	gbsizer->Add(label_thing_dist, wxGBPosition(1, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	cb_max_thing_dist_lock = new wxCheckBox(this, -1, "Lock");
	gbsizer->Add(cb_max_thing_dist_lock, wxGBPosition(1, 3), wxDefaultSpan, wxEXPAND);
	gbsizer->AddGrowableCol(1, 1);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	fsizer->Add(hbox, 0, wxEXPAND|wxALL, 4);

	// Adaptive render distance
	cb_render_dist_adaptive = new wxCheckBox(this, -1, "Adaptive render distance");
	hbox->Add(cb_render_dist_adaptive, 0, wxEXPAND|wxRIGHT, 10);

	hbox->Add(new wxStaticText(this, -1, "Target framerate:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	spin_adaptive_fps = new wxSpinCtrl(this, -1, "30", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, 10, 100, 30);
	hbox->Add(spin_adaptive_fps, 0, wxEXPAND);

	fsizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL), 0, wxEXPAND|wxTOP|wxBOTTOM, 4);

	// Render sky preview
	cb_render_sky = new wxCheckBox(this, -1, "Render sky preview");
	fsizer->Add(cb_render_sky, 0, wxEXPAND|wxALL, 4);


	// Bind events
	slider_max_render_dist->Bind(wxEVT_SLIDER, &Map3DPrefsPanel::onSliderMaxRenderDistChanged, this);
	slider_max_thing_dist->Bind(wxEVT_SLIDER, &Map3DPrefsPanel::onSliderMaxThingDistChanged, this);
	cb_max_thing_dist_lock->Bind(wxEVT_CHECKBOX, &Map3DPrefsPanel::onCBLockThingDistChanged, this);
	cb_distance_unlimited->Bind(wxEVT_CHECKBOX, &Map3DPrefsPanel::onCBDistUnlimitedChanged, this);
}

Map3DPrefsPanel::~Map3DPrefsPanel()
{
}

void Map3DPrefsPanel::init()
{
	if (render_max_dist < 0)
	{
		cb_distance_unlimited->SetValue(true);
		slider_max_render_dist->SetValue(6);
	}
	else
	{
		slider_max_render_dist->SetValue(render_max_dist / 500);
		cb_distance_unlimited->SetValue(false);
	}

	if (render_max_thing_dist < 0)
		cb_max_thing_dist_lock->SetValue(true);
	else
	{
		slider_max_thing_dist->SetValue(render_max_thing_dist / 500);
		cb_max_thing_dist_lock->SetValue(false);
	}

	cb_render_dist_adaptive->SetValue(render_max_dist_adaptive);
	int fps = 1.0 / (render_adaptive_ms/1000.0);
	spin_adaptive_fps->SetValue(fps);
	cb_render_sky->SetValue(render_3d_sky);

	updateDistanceControls();
}

void Map3DPrefsPanel::updateDistanceControls()
{
	// Render distance
	if (cb_distance_unlimited->GetValue())
	{
		label_render_dist->SetLabel("");
		slider_max_render_dist->Enable(false);
	}
	else
	{
		label_render_dist->SetLabel(S_FMT("%d", slider_max_render_dist->GetValue() * 500));
		slider_max_render_dist->Enable();
	}

	// Thing distance
	if (cb_max_thing_dist_lock->GetValue())
	{
		label_thing_dist->SetLabel("");
		slider_max_thing_dist->Enable(false);
		slider_max_thing_dist->SetValue(slider_max_render_dist->GetValue());
	}
	else
	{
		label_thing_dist->SetLabel(S_FMT("%d", slider_max_thing_dist->GetValue() * 500));
		slider_max_thing_dist->Enable();
	}
}

void Map3DPrefsPanel::applyPreferences()
{
	// Max render distance
	if (cb_distance_unlimited->GetValue())
		render_max_dist = -1.0f;
	else
		render_max_dist = slider_max_render_dist->GetValue() * 500.0f;

	// Max thing distance
	if (cb_max_thing_dist_lock->GetValue())
		render_max_thing_dist = -1.0f;
	else
		render_max_thing_dist = slider_max_thing_dist->GetValue() * 500.0f;

	// Adaptive fps
	render_max_dist_adaptive = cb_render_dist_adaptive->GetValue();
	render_adaptive_ms = 1000 / spin_adaptive_fps->GetValue();

	// Other
	render_3d_sky = cb_render_sky->GetValue();
}

void Map3DPrefsPanel::onSliderMaxRenderDistChanged(wxCommandEvent& e)
{
	if (cb_max_thing_dist_lock->GetValue())
		slider_max_thing_dist->SetValue(slider_max_render_dist->GetValue());

	updateDistanceControls();
}

void Map3DPrefsPanel::onSliderMaxThingDistChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}

void Map3DPrefsPanel::onCBDistUnlimitedChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}

void Map3DPrefsPanel::onCBLockThingDistChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}
