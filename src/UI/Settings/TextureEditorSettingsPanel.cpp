
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextureEditorSettingsPanel.cpp
// Description: Panel containing texture editor settings controls
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
#include "TextureEditorSettingsPanel.h"
#include "UI/Controls/SettingsTable.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// TextureEditorSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureEditorSettingsPanel class constructor
// -----------------------------------------------------------------------------
TextureEditorSettingsPanel::TextureEditorSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Create settings table
	settings_general_ = new SettingsTable(this, false);
	sizer->Add(settings_general_, wxSizerFlags(1).Expand());

	// Add controls to settings table
	settings_general_->addCheckBox("Show patch info text on hover", "tx_hover_info");
	settings_general_->addCheckBox("Zoom towards cursor when using mouse wheel", "tx_zoom_towards_cursor");
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void TextureEditorSettingsPanel::loadSettings()
{
	settings_general_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void TextureEditorSettingsPanel::applySettings()
{
	settings_general_->applySettings();
}
