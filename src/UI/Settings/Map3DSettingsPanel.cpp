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
EXTERN_CVAR(Float, map3d_max_render_dist)
EXTERN_CVAR(Float, map3d_max_thing_dist)
EXTERN_CVAR(Bool, map3d_render_sky)
EXTERN_CVAR(Bool, map3d_crosshair_show_distance)
EXTERN_CVAR(Bool, map3d_mlook_invert_y)
EXTERN_CVAR(Bool, map3d_fake_contrast)
EXTERN_CVAR(Int, map3d_fov)


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

	psizer->Add(new wxStaticLine(this, -1), lh.sfWithLargeBorder(0, wxTOP | wxBOTTOM).Expand());

	lh.layoutVertically(
		psizer,
		{ cb_render_sky_       = new wxCheckBox(this, -1, wxS("Render sky preview")),
		  cb_show_distance_    = new wxCheckBox(this, -1, wxS("Show distance under crosshair")),
		  cb_invert_y_         = new wxCheckBox(this, -1, wxS("Invert mouse Y axis")),
		  cb_shade_orthogonal_ = new wxCheckBox(this, -1, wxS("Fake contrast on orthogonal walls")) },
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
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void Map3DSettingsPanel::loadSettings()
{
	if (map3d_max_render_dist <= 0)
	{
		cb_distance_unlimited_->SetValue(true);
		slider_max_render_dist_->setValue(6 * 500);
	}
	else
	{
		slider_max_render_dist_->setValue(map3d_max_render_dist);
		cb_distance_unlimited_->SetValue(false);
	}

	if (map3d_max_thing_dist <= 0)
		cb_max_thing_dist_lock_->SetValue(true);
	else
	{
		slider_max_thing_dist_->setValue(map3d_max_thing_dist);
		cb_max_thing_dist_lock_->SetValue(false);
	}

	slider_fov_->setValue(map3d_fov);
	cb_render_sky_->SetValue(map3d_render_sky);
	cb_show_distance_->SetValue(map3d_crosshair_show_distance);
	cb_invert_y_->SetValue(map3d_mlook_invert_y);
	cb_shade_orthogonal_->SetValue(map3d_fake_contrast);

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
		map3d_max_render_dist = 0.0f;
	else
		map3d_max_render_dist = slider_max_render_dist_->value();

	// Max thing distance
	if (cb_max_thing_dist_lock_->GetValue())
		map3d_max_thing_dist = 0.0f;
	else
		map3d_max_thing_dist = slider_max_thing_dist_->value();

	// Other
	map3d_render_sky              = cb_render_sky_->GetValue();
	map3d_crosshair_show_distance = cb_show_distance_->GetValue();
	map3d_mlook_invert_y          = cb_invert_y_->GetValue();
	map3d_fov                     = slider_fov_->value();
	map3d_fake_contrast           = cb_shade_orthogonal_->GetValue();
}
