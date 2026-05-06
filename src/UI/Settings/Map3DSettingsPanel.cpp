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
#include "UI/Controls/RadioButtonPanel.h"
#include "UI/Layout.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Float, map3d_max_render_dist)
EXTERN_CVAR(Bool, map3d_render_sky)
EXTERN_CVAR(Bool, map3d_crosshair_show_distance)
EXTERN_CVAR(Bool, map3d_fake_contrast)
EXTERN_CVAR(Int, map3d_fov)
EXTERN_CVAR(Int, map3d_lights_max)
EXTERN_CVAR(Bool, map3d_highlight_fill)
EXTERN_CVAR(Bool, map3d_highlight_outline)


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
	auto lh = LayoutHelper(this);

	// Create border sizer
	auto border = new wxBoxSizer(wxVERTICAL);
	SetSizer(border);

	// Create sizer
	auto psizer = new wxBoxSizer(wxVERTICAL);
	border->Add(psizer, lh.sfWithLargeBorder(1).Expand());

	auto gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	psizer->Add(gbsizer, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Render distance
	int row = 0;
	gbsizer->Add(new wxStaticText(this, -1, wxS("Render distance:")), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_max_render_dist_ = new NumberSlider(this, 500, 20 * 500, 500);
	gbsizer->Add(slider_max_render_dist_, { row, 1 }, { 1, 2 }, wxEXPAND);
	cb_distance_unlimited_ = new wxCheckBox(this, -1, wxS("Unlimited"));
	gbsizer->Add(cb_distance_unlimited_, { row, 3 }, { 1, 1 }, wxEXPAND);
	gbsizer->AddGrowableCol(1, 1);

	// FOV
	gbsizer->Add(new wxStaticText(this, -1, wxS("Camera FOV:")), { ++row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_fov_ = new NumberSlider(this, 70, 120, 5);
	gbsizer->Add(slider_fov_, { row, 1 }, { 1, 2 }, wxEXPAND);

	// Max. dynamic lights
	gbsizer->Add(
		new wxStaticText(this, -1, wxS("Max. dynamic lights:")), { ++row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	slider_max_lights_ = new NumberSlider(this, 32, 512, 32);
	gbsizer->Add(slider_max_lights_, { row, 1 }, { 1, 2 }, wxEXPAND);

	// Other rendering options
	lh.layoutVertically(
		psizer,
		{ cb_render_sky_       = new wxCheckBox(this, -1, wxS("Render sky preview")),
		  cb_show_distance_    = new wxCheckBox(this, -1, wxS("Show distance under crosshair")),
		  cb_shade_orthogonal_ = new wxCheckBox(this, -1, wxS("Fake contrast on orthogonal walls")),
		  rbp_highlight_       = new RadioButtonPanel(this, { "Fill", "Outline", "Both" }, "Item highlight:") },
		wxSizerFlags(0).Expand());

	// Bind events
	slider_max_render_dist_->Bind(
		wxEVT_SLIDER,
		[&](wxCommandEvent& e)
		{
			updateDistanceControls();
			e.Skip();
		});
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

	slider_fov_->setValue(map3d_fov);
	cb_render_sky_->SetValue(map3d_render_sky);
	cb_show_distance_->SetValue(map3d_crosshair_show_distance);
	cb_shade_orthogonal_->SetValue(map3d_fake_contrast);
	slider_max_lights_->setValue(map3d_lights_max);

	if (map3d_highlight_fill && map3d_highlight_outline)
		rbp_highlight_->setSelection(2);
	else if (map3d_highlight_outline)
		rbp_highlight_->setSelection(1);
	else
		rbp_highlight_->setSelection(0);

	updateDistanceControls();
}

// -----------------------------------------------------------------------------
// Updates render distance controls (value labels, locking, etc.)
// -----------------------------------------------------------------------------
void Map3DSettingsPanel::updateDistanceControls() const
{
	slider_max_render_dist_->Enable(!cb_distance_unlimited_->GetValue());
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

	// Highlight
	switch (rbp_highlight_->getSelection())
	{
	case 1:
		map3d_highlight_fill    = false;
		map3d_highlight_outline = true;
		break;
	case 2:
		map3d_highlight_fill    = true;
		map3d_highlight_outline = true;
		break;
	default:
		map3d_highlight_fill    = true;
		map3d_highlight_outline = false;
		break;
	}

	// Other
	map3d_render_sky              = cb_render_sky_->GetValue();
	map3d_crosshair_show_distance = cb_show_distance_->GetValue();
	map3d_fov                     = slider_fov_->value();
	map3d_fake_contrast           = cb_shade_orthogonal_->GetValue();
	map3d_lights_max              = slider_max_lights_->value();
}
