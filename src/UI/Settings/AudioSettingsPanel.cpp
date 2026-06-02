
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    AudioSettingsPanel.cpp
// Description: Panel containing Audio settings controls
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
#include "AudioSettingsPanel.h"
#include "App.h"
#include "Audio/MIDIPlayer.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/Controls/SettingsTable.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
vector sample_rates = { 22050, 44100, 48000, 96000 };
}

// -----------------------------------------------------------------------------
//
// AudioSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AudioSettingsPanel class constructor
// -----------------------------------------------------------------------------
AudioSettingsPanel::AudioSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	// Create controls
	settings_panel_ = new SettingsTable(this);
	flp_soundfont_  = new FileLocationPanel(
        settings_panel_,
        "",
        app::platform() != app::Windows,
        "Browse for MIDI Soundfont",
        "Soundfont files (*.sf2)|*.sf2");
	flp_timidity_ = new FileLocationPanel(
		settings_panel_,
		"",
		app::platform() != app::Windows,
		"Browse for Timidity Executable",
		filedialog::executableExtensionString());
	btn_reset_player_     = new wxButton(settings_panel_, -1, wxS("Reset MIDI Player"));
	choice_fs_samplerate_ = new wxChoice(settings_panel_, -1);

	// Populate sample rates dropdown
	for (auto rate : sample_rates)
		choice_fs_samplerate_->Append(WX_FMT("{} Hz", rate));

	// Populate settings table
	settings_panel_->addCheckBox("Autoplay audio entries when opened", "snd_autoplay");
	settings_panel_->addCheckBox("Use DMX padding when appropriate", "dmx_padding");
	settings_panel_->addRadioButtons("MIDI player", "snd_midiplayer", { "Fluidsynth", "Timidity (external)" });
	settings_panel_->addCustomControl("", btn_reset_player_);
	settings_panel_->addSectionSeparator("Fluidsynth Options");
	settings_panel_->addCustomControl("Use custom MIDI soundfont", flp_soundfont_);
	settings_panel_->addCustomControl("Sample rate", choice_fs_samplerate_, wxALIGN_CENTER_VERTICAL);
	settings_panel_->addCheckBox("Enable chorus effect", "fs_chorus");
	settings_panel_->addCheckBox("Enable reverb effect", "fs_reverb");
	settings_panel_->addSectionSeparator("Timidity Options");
	settings_panel_->addCustomControl("Timidity executable path", flp_timidity_);
	settings_panel_->addTextBox("Command-line options", "snd_timidity_options");

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	sizer->Add(settings_panel_, 1, wxEXPAND);

	// Bind events
	btn_reset_player_->Bind(
		wxEVT_BUTTON,
		[&](wxCommandEvent&)
		{
			audio::resetMIDIPlayer();
			audio::midiPlayer().setVolume(CVar::getInt("snd_volume"));
		});

	wxPanel::Layout();
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void AudioSettingsPanel::loadSettings()
{
	settings_panel_->loadSettings();
	flp_soundfont_->setLocation(CVar::getString("fs_soundfont_path"));
	flp_timidity_->setLocation(CVar::getString("snd_timidity_path"));
	wxutil::setNumericChoiceFromValue(choice_fs_samplerate_, sample_rates, CVar::getInt("fs_sample_rate"));
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void AudioSettingsPanel::applySettings()
{
	settings_panel_->applySettings();
	CVar::set("fs_soundfont_path", flp_soundfont_->location());
	CVar::set("snd_timidity_path", flp_timidity_->location());
	CVar::setInt("fs_sample_rate", sample_rates[choice_fs_samplerate_->GetSelection()]);

	audio::resetMIDIPlayer();
	audio::midiPlayer().setVolume(CVar::getInt("snd_volume"));
}
