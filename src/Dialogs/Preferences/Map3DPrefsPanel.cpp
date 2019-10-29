
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Map3DPrefsPanel.cpp
// Description: Panel containing preference controls for the map editor 3d mode
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "Map3DPrefsPanel.h"
#include "General/UI.h"
#include "UI/WxUtils.h"


// ----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(Float, render_max_dist)
EXTERN_CVAR(Float, render_max_thing_dist)
EXTERN_CVAR(Bool, render_max_dist_adaptive)
EXTERN_CVAR(Int, render_adaptive_ms)
EXTERN_CVAR(Bool, render_3d_sky)
EXTERN_CVAR(Bool, camera_3d_show_distance)
EXTERN_CVAR(Bool, mlook_invert_y)
EXTERN_CVAR(Bool, render_shade_orthogonal_lines)
EXTERN_CVAR(Int, render_fov)


// ----------------------------------------------------------------------------
//
// Map3DPrefsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Map3DPrefsPanel::Map3DPrefsPanel
//
// Map3DPrefsPanel class constructor
// ----------------------------------------------------------------------------
Map3DPrefsPanel::Map3DPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	wxGridBagSizer* gbsizer = new wxGridBagSizer(UI::pad(), UI::pad());
	psizer->Add(gbsizer, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// Render distance
	gbsizer->Add(new wxStaticText(this, -1, "Render distance:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_max_render_dist_ = new wxSlider(this, -1, 1, 1, 20, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gbsizer->Add(slider_max_render_dist_, { 0, 1 }, { 1, 1 }, wxEXPAND);
	label_render_dist_ = new wxStaticText(this, -1, "00000");
	label_render_dist_->SetInitialSize(wxSize(label_render_dist_->GetSize().x, -1));
	gbsizer->Add(label_render_dist_, { 0, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	cb_distance_unlimited_ = new wxCheckBox(this, -1, "Unlimited");
	gbsizer->Add(cb_distance_unlimited_, { 0, 3 }, { 1, 1 }, wxEXPAND);

	// Thing Render distance
	gbsizer->Add(new wxStaticText(this, -1, "Thing render distance:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_max_thing_dist_ = new wxSlider(this, -1, 1, 1, 20, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gbsizer->Add(slider_max_thing_dist_, { 1, 1 }, { 1, 1 }, wxEXPAND);
	label_thing_dist_ = new wxStaticText(this, -1, "00000");
	gbsizer->Add(label_thing_dist_, { 1, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	cb_max_thing_dist_lock_ = new wxCheckBox(this, -1, "Lock");
	gbsizer->Add(cb_max_thing_dist_lock_, { 1, 3 }, { 1, 1 }, wxEXPAND);
	gbsizer->AddGrowableCol(1, 1);

	// FOV
	gbsizer->Add(new wxStaticText(this, -1, "FOV:"), { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_fov_ = new wxSlider(this, -1, 1, 7, 12, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS);
	gbsizer->Add(slider_fov_, { 2, 1 }, { 1, 1 }, wxEXPAND);
	label_fov_ = new wxStaticText(this, -1, "00000");
	gbsizer->Add(label_fov_, { 2, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	psizer->Add(hbox, 0, wxEXPAND);

	// Adaptive render distance
	cb_render_dist_adaptive_ = new wxCheckBox(this, -1, "Adaptive render distance");
	hbox->Add(cb_render_dist_adaptive_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::padLarge());

	hbox->Add(new wxStaticText(this, -1, "Target framerate:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	spin_adaptive_fps_ = new wxSpinCtrl(
		this,
		-1,
		"30",
		wxDefaultPosition,
		{ UI::px(UI::Size::SpinCtrlWidth), -1 },
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		10,
		100,
		30
	);
	hbox->Add(spin_adaptive_fps_, 0, wxEXPAND);

	psizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND | wxTOP | wxBOTTOM, UI::padLarge());

	WxUtils::layoutVertically(
		psizer,
		{
			cb_render_sky_ = new wxCheckBox(this, -1, "Render sky preview"),
			cb_show_distance_ = new wxCheckBox(this, -1, "Show distance under crosshair"),
			cb_invert_y_ = new wxCheckBox(this, -1, "Invert mouse Y axis"),
			cb_shade_orthogonal_ = new wxCheckBox(this, -1, "Shade orthogonal lines")
		},
		wxSizerFlags(0).Expand()
	);

	// Bind events
	slider_max_render_dist_->Bind(wxEVT_SLIDER, &Map3DPrefsPanel::onSliderMaxRenderDistChanged, this);
	slider_max_thing_dist_->Bind(wxEVT_SLIDER, &Map3DPrefsPanel::onSliderMaxThingDistChanged, this);
	cb_max_thing_dist_lock_->Bind(wxEVT_CHECKBOX, &Map3DPrefsPanel::onCBLockThingDistChanged, this);
	cb_distance_unlimited_->Bind(wxEVT_CHECKBOX, &Map3DPrefsPanel::onCBDistUnlimitedChanged, this);
	slider_fov_->Bind(wxEVT_SLIDER, [&](wxCommandEvent&){ updateDistanceControls(); });
}

// ----------------------------------------------------------------------------
// Map3DPrefsPanel::~Map3DPrefsPanel
//
// Map3DPrefsPanel class destructor
// ----------------------------------------------------------------------------
Map3DPrefsPanel::~Map3DPrefsPanel()
{
}

// ----------------------------------------------------------------------------
// Map3DPrefsPanel::init
//
// Initialises panel controls
// ----------------------------------------------------------------------------
void Map3DPrefsPanel::init()
{
	if (render_max_dist < 0)
	{
		cb_distance_unlimited_->SetValue(true);
		slider_max_render_dist_->SetValue(6);
	}
	else
	{
		slider_max_render_dist_->SetValue(render_max_dist / 500);
		cb_distance_unlimited_->SetValue(false);
	}

	if (render_max_thing_dist < 0)
		cb_max_thing_dist_lock_->SetValue(true);
	else
	{
		slider_max_thing_dist_->SetValue(render_max_thing_dist / 500);
		cb_max_thing_dist_lock_->SetValue(false);
	}

	slider_fov_->SetValue(render_fov / 10);
	cb_render_dist_adaptive_->SetValue(render_max_dist_adaptive);
	int fps = 1.0 / (render_adaptive_ms/1000.0);
	spin_adaptive_fps_->SetValue(fps);
	cb_render_sky_->SetValue(render_3d_sky);
	cb_show_distance_->SetValue(camera_3d_show_distance);
	cb_invert_y_->SetValue(mlook_invert_y);
	cb_shade_orthogonal_->SetValue(render_shade_orthogonal_lines);

	updateDistanceControls();
}

// ----------------------------------------------------------------------------
// Map3DPrefsPanel::updateDistanceControls
//
// Updates render distance controls (value labels, locking, etc.)
// ----------------------------------------------------------------------------
void Map3DPrefsPanel::updateDistanceControls()
{
	// Render distance
	if (cb_distance_unlimited_->GetValue())
	{
		label_render_dist_->SetLabel("");
		slider_max_render_dist_->Enable(false);
	}
	else
	{
		label_render_dist_->SetLabel(S_FMT("%d", slider_max_render_dist_->GetValue() * 500));
		slider_max_render_dist_->Enable();
	}

	// Thing distance
	if (cb_max_thing_dist_lock_->GetValue())
	{
		label_thing_dist_->SetLabel("");
		slider_max_thing_dist_->Enable(false);
		slider_max_thing_dist_->SetValue(slider_max_render_dist_->GetValue());
	}
	else
	{
		label_thing_dist_->SetLabel(S_FMT("%d", slider_max_thing_dist_->GetValue() * 500));
		slider_max_thing_dist_->Enable();
	}

	// FOV
	label_fov_->SetLabel(S_FMT("%d", slider_fov_->GetValue() * 10));
}

// ----------------------------------------------------------------------------
// Map3DPrefsPanel::applyPreferences
//
// Applies preference values from the controls to CVARs
// ----------------------------------------------------------------------------
void Map3DPrefsPanel::applyPreferences()
{
	// Max render distance
	if (cb_distance_unlimited_->GetValue())
		render_max_dist = -1.0f;
	else
		render_max_dist = slider_max_render_dist_->GetValue() * 500.0f;

	// Max thing distance
	if (cb_max_thing_dist_lock_->GetValue())
		render_max_thing_dist = -1.0f;
	else
		render_max_thing_dist = slider_max_thing_dist_->GetValue() * 500.0f;

	// Adaptive fps
	render_max_dist_adaptive = cb_render_dist_adaptive_->GetValue();
	render_adaptive_ms = 1000 / spin_adaptive_fps_->GetValue();

	// Other
	render_fov = slider_fov_->GetValue() * 10;
	render_3d_sky = cb_render_sky_->GetValue();
	camera_3d_show_distance = cb_show_distance_->GetValue();
	mlook_invert_y = cb_invert_y_->GetValue();
	render_shade_orthogonal_lines = cb_shade_orthogonal_->GetValue();
}


// ----------------------------------------------------------------------------
//
// Map3DPrefsPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Map3DPrefsPanel::onSliderMaxRenderDistChanged
//
// Called when the render distance slider is changed
// ----------------------------------------------------------------------------
void Map3DPrefsPanel::onSliderMaxRenderDistChanged(wxCommandEvent& e)
{
	if (cb_max_thing_dist_lock_->GetValue())
		slider_max_thing_dist_->SetValue(slider_max_render_dist_->GetValue());

	updateDistanceControls();
}

// ----------------------------------------------------------------------------
// Map3DPrefsPanel::onSliderMaxThingDistChanged
//
// Called when the thing render distance slider is changed
// ----------------------------------------------------------------------------
void Map3DPrefsPanel::onSliderMaxThingDistChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}

// ----------------------------------------------------------------------------
// Map3DPrefsPanel::onCBDistUnlimitedChanged
//
// Called when the 'Unlimited' render distance checkbox is clicked
// ----------------------------------------------------------------------------
void Map3DPrefsPanel::onCBDistUnlimitedChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}

// ----------------------------------------------------------------------------
// Map3DPrefsPanel::onCBLockThingDistChanged
//
// Called when the 'Lock' thing render distance checkbox is clicked
// ----------------------------------------------------------------------------
void Map3DPrefsPanel::onCBLockThingDistChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}
