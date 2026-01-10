// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Map3DSettingsPanel.cpp
// Description: Panel containing settings controls for the map editor 3d mode
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Map3DSettingsPanel.h"
#include "UI/Controls/NumberSlider.h"
#include "UI/Layout.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Float, render_max_dist)
EXTERN_CVAR(Float, render_max_thing_dist)
EXTERN_CVAR(Bool, render_max_dist_adaptive)
EXTERN_CVAR(Int, render_adaptive_ms)
EXTERN_CVAR(Bool, render_3d_sky)
EXTERN_CVAR(Bool, camera_3d_show_distance)
EXTERN_CVAR(Bool, mlook_invert_y)
EXTERN_CVAR(Bool, render_shade_orthogonal_lines)
EXTERN_CVAR(Int, render_fov)
EXTERN_CVAR(Bool, map_process_3d_floors)


// -----------------------------------------------------------------------------
//
// Map3DSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Map3DSettingsPanel class constructor
// -----------------------------------------------------------------------------
Map3DSettingsPanel::Map3DSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	// Create sizer
	auto psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	auto lh      = ui::LayoutHelper(this);
	auto gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	psizer->Add(gbsizer, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Render distance
	gbsizer->Add(new wxStaticText(this, -1, wxS("Render distance:")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_max_render_dist_ = new NumberSlider(this, 500, 20 * 500, 500);
	gbsizer->Add(slider_max_render_dist_, { 0, 1 }, { 1, 2 }, wxEXPAND);
	cb_distance_unlimited_ = new wxCheckBox(this, -1, wxS("Unlimited"));
	gbsizer->Add(cb_distance_unlimited_, { 0, 3 }, { 1, 1 }, wxEXPAND);

	// Thing Render distance
	gbsizer->Add(
		new wxStaticText(this, -1, wxS("Thing render distance:")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_max_thing_dist_ = new NumberSlider(this, 500, 20 * 500, 500);
	gbsizer->Add(slider_max_thing_dist_, { 1, 1 }, { 1, 2 }, wxEXPAND);
	cb_max_thing_dist_lock_ = new wxCheckBox(this, -1, wxS("Lock"));
	gbsizer->Add(cb_max_thing_dist_lock_, { 1, 3 }, { 1, 1 }, wxEXPAND);
	gbsizer->AddGrowableCol(1, 1);

	// FOV
	gbsizer->Add(new wxStaticText(this, -1, wxS("FOV:")), { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_fov_ = new NumberSlider(this, 70, 120, 5);
	gbsizer->Add(slider_fov_, { 2, 1 }, { 1, 2 }, wxEXPAND);

	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	psizer->Add(hbox, wxSizerFlags().Expand());

	// Adaptive render distance
	cb_render_dist_adaptive_ = new wxCheckBox(this, -1, wxS("Adaptive render distance"));
	hbox->Add(cb_render_dist_adaptive_, lh.sfWithLargeBorder(0, wxRIGHT).CenterVertical());

	hbox->Add(new wxStaticText(this, -1, wxS("Target framerate:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	spin_adaptive_fps_ = new wxSpinCtrl(
		this, -1, wxS("30"), wxDefaultPosition, lh.spinSize(), wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 10, 100, 30);
	hbox->Add(spin_adaptive_fps_, wxSizerFlags().Expand());

	psizer->Add(new wxStaticLine(this, -1), lh.sfWithLargeBorder(0, wxTOP | wxBOTTOM).Expand());

	lh.layoutVertically(
		psizer,
		{ cb_render_sky_       = new wxCheckBox(this, -1, wxS("Render sky preview")),
		  cb_show_distance_    = new wxCheckBox(this, -1, wxS("Show distance under crosshair")),
		  cb_invert_y_         = new wxCheckBox(this, -1, wxS("Invert mouse Y axis")),
		  cb_shade_orthogonal_ = new wxCheckBox(this, -1, wxS("Shade orthogonal lines")),
		  cb_enable_3d_floors_ = new wxCheckBox(this, -1, wxS("[EXPERIMENTAL] Enable 3d floors preview")) },
		wxSizerFlags(0).Expand());

	// Bind events
	slider_max_render_dist_->Bind(
		wxEVT_SLIDER,
		[&](wxCommandEvent& e)
		{
			if (cb_max_thing_dist_lock_->GetValue())
				slider_max_thing_dist_->setValue(slider_max_render_dist_->value());
			updateDistanceControls();
			e.Skip();
		});
	slider_max_thing_dist_->Bind(
		wxEVT_SLIDER,
		[&](wxCommandEvent& e)
		{
			updateDistanceControls();
			e.Skip();
		});
	cb_max_thing_dist_lock_->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent&) { updateDistanceControls(); });
	cb_distance_unlimited_->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent&) { updateDistanceControls(); });
	slider_fov_->Bind(
		wxEVT_SLIDER,
		[&](wxCommandEvent& e)
		{
			updateDistanceControls();
			e.Skip();
		});
	cb_enable_3d_floors_->Bind(
		wxEVT_CHECKBOX,
		[&](wxCommandEvent&)
		{
			if (cb_enable_3d_floors_->GetValue())
				wxMessageBox(
					wxS("This feature is currently experimental and does not work correctly for all 3d floor types.\n\n"
						"Any currently open map will need to be closed and reopened for the setting to take effect."),
					wxS("Experimental Feature Warning"),
					wxICON_WARNING);
		});
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void Map3DSettingsPanel::loadSettings()
{
	if (render_max_dist < 0)
	{
		cb_distance_unlimited_->SetValue(true);
		slider_max_render_dist_->setValue(6 * 500);
	}
	else
	{
		slider_max_render_dist_->setValue(render_max_dist);
		cb_distance_unlimited_->SetValue(false);
	}

	if (render_max_thing_dist < 0)
		cb_max_thing_dist_lock_->SetValue(true);
	else
	{
		slider_max_thing_dist_->setValue(render_max_thing_dist);
		cb_max_thing_dist_lock_->SetValue(false);
	}

	slider_fov_->setValue(render_fov);
	cb_render_dist_adaptive_->SetValue(render_max_dist_adaptive);
	int fps = 1.0 / (render_adaptive_ms / 1000.0);
	spin_adaptive_fps_->SetValue(fps);
	cb_render_sky_->SetValue(render_3d_sky);
	cb_show_distance_->SetValue(camera_3d_show_distance);
	cb_invert_y_->SetValue(mlook_invert_y);
	cb_shade_orthogonal_->SetValue(render_shade_orthogonal_lines);
	cb_enable_3d_floors_->SetValue(map_process_3d_floors);

	updateDistanceControls();
}

// -----------------------------------------------------------------------------
// Updates render distance controls (value labels, locking, etc.)
// -----------------------------------------------------------------------------
void Map3DSettingsPanel::updateDistanceControls() const
{
	// Render distance
	slider_max_render_dist_->Enable(!cb_distance_unlimited_->GetValue());

	// Thing distance
	if (cb_max_thing_dist_lock_->GetValue())
	{
		slider_max_thing_dist_->Enable(false);
		slider_max_thing_dist_->setValue(slider_max_render_dist_->value());
	}
	else
		slider_max_thing_dist_->Enable();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void Map3DSettingsPanel::applySettings()
{
	// Max render distance
	if (cb_distance_unlimited_->GetValue())
		render_max_dist = -1.0f;
	else
		render_max_dist = slider_max_render_dist_->value();

	// Max thing distance
	if (cb_max_thing_dist_lock_->GetValue())
		render_max_thing_dist = -1.0f;
	else
		render_max_thing_dist = slider_max_thing_dist_->value();

	// Adaptive fps
	render_max_dist_adaptive = cb_render_dist_adaptive_->GetValue();
	render_adaptive_ms       = 1000 / spin_adaptive_fps_->GetValue();

	// Other
	render_3d_sky                 = cb_render_sky_->GetValue();
	camera_3d_show_distance       = cb_show_distance_->GetValue();
	mlook_invert_y                = cb_invert_y_->GetValue();
	render_fov                    = slider_fov_->value();
	render_shade_orthogonal_lines = cb_shade_orthogonal_->GetValue();
	map_process_3d_floors         = cb_enable_3d_floors_->GetValue();
}
