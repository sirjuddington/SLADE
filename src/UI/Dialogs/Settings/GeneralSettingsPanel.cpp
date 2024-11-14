
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GeneralSettingsPanel.cpp
// Description: Panel containing general preference controls
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

// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "GeneralSettingsPanel.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;

GeneralSettingsPanel::GeneralSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto lh    = LayoutHelper(this);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Program
	sizer->Add(wxutil::createSectionSeparator(this, "Program"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		sizer,
		{ new wxCheckBox(this, -1, "Show the Start Page on startup"),
		  new wxCheckBox(this, -1, "Show confirmation dialog on exit"),
		  new wxCheckBox(this, -1, "Check for updates on startup"),
		  new wxCheckBox(this, -1, "Include beta versions when checking for updates") },
		lh.sfWithBorder(0, wxLEFT));

	// Archive
	sizer->AddSpacer(lh.padXLarge());
	sizer->Add(wxutil::createSectionSeparator(this, "Archives"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		sizer,
		{ new wxCheckBox(this, -1, "Close archive when its tab is closed"),
		  new wxCheckBox(this, -1, "Automatically open nested WAD archives"),
		  new wxCheckBox(this, -1, "Backup archives before saving"),
		  new wxCheckBox(this, -1, "Ignore hidden files in directories") },
		lh.sfWithBorder(0, wxLEFT));
}

void GeneralSettingsPanel::applySettings() {}
