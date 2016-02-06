
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Map3DPrefsPanel.cpp
 * Description: Panel containing preference controls for the map
 *              editor 3d mode
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "Map3DPrefsPanel.h"
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Float, render_max_dist)
EXTERN_CVAR(Float, render_max_thing_dist)
EXTERN_CVAR(Bool, render_max_dist_adaptive)
EXTERN_CVAR(Int, render_adaptive_ms)
EXTERN_CVAR(Bool, render_3d_sky)
EXTERN_CVAR(Bool, camera_3d_show_distance)
EXTERN_CVAR(Bool, mlook_invert_y)
EXTERN_CVAR(Bool, render_shade_orthogonal_lines)


/*******************************************************************
 * MAP3DPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* Map3DPrefsPanel::Map3DPrefsPanel
 * Map3DPrefsPanel class constructor
 *******************************************************************/
Map3DPrefsPanel::Map3DPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Map Editor 3D Mode Preferences");
	wxStaticBoxSizer* fsizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(fsizer, 1, wxEXPAND|wxALL, 4);

	wxGridBagSizer* gbsizer = new wxGridBagSizer(8, 8);
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

	// Show distance
	cb_show_distance = new wxCheckBox(this, -1, "Show distance under crosshair");
	fsizer->Add(cb_show_distance, 0, wxEXPAND|wxALL, 4);

	// Invert mouse y
	cb_invert_y = new wxCheckBox(this, -1, "Invert mouse Y axis");
	fsizer->Add(cb_invert_y, 0, wxEXPAND|wxALL, 4);

	// Shade orthogonal lines
	cb_shade_orthogonal = new wxCheckBox(this, -1, "Shade orthogonal lines");
	fsizer->Add(cb_shade_orthogonal, 0, wxEXPAND | wxALL, 4);


	// Bind events
	slider_max_render_dist->Bind(wxEVT_SLIDER, &Map3DPrefsPanel::onSliderMaxRenderDistChanged, this);
	slider_max_thing_dist->Bind(wxEVT_SLIDER, &Map3DPrefsPanel::onSliderMaxThingDistChanged, this);
	cb_max_thing_dist_lock->Bind(wxEVT_CHECKBOX, &Map3DPrefsPanel::onCBLockThingDistChanged, this);
	cb_distance_unlimited->Bind(wxEVT_CHECKBOX, &Map3DPrefsPanel::onCBDistUnlimitedChanged, this);
}

/* Map3DPrefsPanel::~Map3DPrefsPanel
 * Map3DPrefsPanel class destructor
 *******************************************************************/
Map3DPrefsPanel::~Map3DPrefsPanel()
{
}

/* Map3DPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
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
	cb_show_distance->SetValue(camera_3d_show_distance);
	cb_invert_y->SetValue(mlook_invert_y);
	cb_shade_orthogonal->SetValue(render_shade_orthogonal_lines);

	updateDistanceControls();
}

/* Map3DPrefsPanel::updateDistanceControls
 * Updates render distance controls (value labels, locking, etc.)
 *******************************************************************/
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

/* Map3DPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
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
	camera_3d_show_distance = cb_show_distance->GetValue();
	mlook_invert_y = cb_invert_y->GetValue();
	render_shade_orthogonal_lines = cb_shade_orthogonal->GetValue();
}


/*******************************************************************
 * MAP3DPREFSPANEL CLASS EVENTS
 *******************************************************************/

/* Map3DPrefsPanel::onSliderMaxRenderDistChanged
 * Called when the render distance slider is changed
 *******************************************************************/
void Map3DPrefsPanel::onSliderMaxRenderDistChanged(wxCommandEvent& e)
{
	if (cb_max_thing_dist_lock->GetValue())
		slider_max_thing_dist->SetValue(slider_max_render_dist->GetValue());

	updateDistanceControls();
}

/* Map3DPrefsPanel::onSliderMaxThingDistChanged
 * Called when the thing render distance slider is changed
 *******************************************************************/
void Map3DPrefsPanel::onSliderMaxThingDistChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}

/* Map3DPrefsPanel::onCBDistUnlimitedChanged
 * Called when the 'Unlimited' render distance checkbox is clicked
 *******************************************************************/
void Map3DPrefsPanel::onCBDistUnlimitedChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}

/* Map3DPrefsPanel::onCBLockThingDistChanged
 * Called when the 'Lock' thing render distance checkbox is clicked
 *******************************************************************/
void Map3DPrefsPanel::onCBLockThingDistChanged(wxCommandEvent& e)
{
	updateDistanceControls();
}
