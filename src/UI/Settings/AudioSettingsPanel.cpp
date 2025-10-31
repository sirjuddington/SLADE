
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Audio/MIDIPlayer.h"
#include "UI/Controls/FileLocationPanel.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, snd_autoplay)
EXTERN_CVAR(Bool, dmx_padding)
EXTERN_CVAR(Int, snd_volume)
EXTERN_CVAR(String, fs_soundfont_path)
EXTERN_CVAR(String, dir_last)
EXTERN_CVAR(String, snd_timidity_path)
EXTERN_CVAR(String, snd_timidity_options)
EXTERN_CVAR(String, snd_midi_player)


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
	cb_snd_autoplay_ = new wxCheckBox(this, -1, wxS("Automatically play audio entries when opened"));
	cb_dmx_padding_  = new wxCheckBox(this, -1, wxS("Use DMX padding when appropriate"));
	rb_fluidsynth_   = new wxRadioButton(this, -1, wxS("Use Fluidsynth"));
	flp_soundfont_   = new FileLocationPanel(
        this, "", true, "Browse for MIDI Soundfont", "Soundfont files (*.sf2)|*.sf2");
	rb_timidity_  = new wxRadioButton(this, -1, wxS("Use Timidity"));
	flp_timidity_ = new FileLocationPanel(
		this, "", true, "Browse for Timidity Executable", filedialog::executableExtensionString());
	text_timidity_options_ = new wxTextCtrl(this, -1);
	btn_reset_player_      = new wxButton(this, -1, wxS("Reset MIDI Player"));

	setupLayout();

	// Bind events
	btn_reset_player_->Bind(
		wxEVT_BUTTON,
		[&](wxCommandEvent&)
		{
			audio::resetMIDIPlayer();
			audio::midiPlayer().setVolume(snd_volume);
		});
	rb_fluidsynth_->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) { updateControls(); });
	rb_timidity_->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) { updateControls(); });

#ifdef NO_FLUIDSYNTH
	rb_fluidsynth_->Enable(false);
	rb_timidity_->SetValue(true);
#endif

	wxPanel::Layout();
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void AudioSettingsPanel::loadSettings()
{
	bool midi_fsynth = strutil::equalCI(snd_midi_player, "fluidsynth");

	cb_snd_autoplay_->SetValue(snd_autoplay);
	cb_dmx_padding_->SetValue(dmx_padding);
	rb_fluidsynth_->SetValue(midi_fsynth);
	rb_timidity_->SetValue(!midi_fsynth);
	flp_soundfont_->setLocation(fs_soundfont_path);
	flp_timidity_->setLocation(snd_timidity_path);
	text_timidity_options_->SetValue(snd_timidity_options);

	updateControls();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void AudioSettingsPanel::applySettings()
{
	snd_autoplay         = cb_snd_autoplay_->GetValue();
	dmx_padding          = cb_dmx_padding_->GetValue();
	snd_midi_player      = rb_timidity_->GetValue() ? "timidity" : "fluidsynth";
	fs_soundfont_path    = flp_soundfont_->location();
	snd_timidity_path    = flp_timidity_->location();
	snd_timidity_options = text_timidity_options_->GetValue().utf8_string();

	audio::resetMIDIPlayer();
	audio::midiPlayer().setVolume(snd_volume);
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void AudioSettingsPanel::setupLayout()
{
	auto lh = ui::LayoutHelper(this);

	// Create sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Autoplay
	sizer->Add(cb_snd_autoplay_, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// DMX Padding
	sizer->Add(cb_dmx_padding_, wxSizerFlags().Expand());

	sizer->AddSpacer(lh.padXLarge());
	sizer->Add(wxutil::createSectionSeparator(this, "MIDI Playback"), lh.sfWithBorder(0, wxBOTTOM).Expand());

	// MIDI Playback (fluidsynth/timidity)
	auto gbsizer = new wxGridBagSizer(lh.padSmall(), lh.pad());
	gbsizer->Add(rb_fluidsynth_, { 0, 0 }, { 1, 1 }, wxEXPAND | wxBOTTOM, lh.pad());
	gbsizer->Add(new wxStaticText(this, -1, wxS("Use custom MIDI soundfont:")), { 1, 0 }, { 1, 1 }, wxEXPAND);
	gbsizer->Add(flp_soundfont_, { 2, 0 }, { 1, 1 }, wxEXPAND | wxBOTTOM, lh.pad());
	gbsizer->Add(rb_timidity_, { 0, 1 }, { 1, 1 }, wxEXPAND | wxBOTTOM, lh.pad());
	gbsizer->Add(new wxStaticText(this, -1, wxS("Location of Timidity executable:")), { 1, 1 }, { 1, 1 }, wxEXPAND);
	gbsizer->Add(flp_timidity_, { 2, 1 }, { 1, 1 }, wxEXPAND | wxBOTTOM, lh.pad());
	gbsizer->Add(new wxStaticText(this, -1, wxS("Timidity command line options:")), { 3, 1 }, { 1, 1 }, wxEXPAND);
	gbsizer->Add(text_timidity_options_, { 4, 1 }, { 1, 1 }, wxEXPAND);
	gbsizer->AddGrowableCol(0, 1);
	gbsizer->AddGrowableCol(1, 1);
	sizer->Add(gbsizer, lh.sfWithBorder(0, wxLEFT).Expand());

	// Reset MIDI player
	sizer->Add(btn_reset_player_, lh.sfWithBorder(0, wxTOP | wxLEFT));
}

// -----------------------------------------------------------------------------
// Enables/disables MIDI playback options depending on what playback method is
// currently selected
// -----------------------------------------------------------------------------
void AudioSettingsPanel::updateControls() const
{
	flp_soundfont_->Enable(rb_fluidsynth_->GetValue());
	flp_timidity_->Enable(rb_timidity_->GetValue());
	text_timidity_options_->Enable(rb_timidity_->GetValue());
}
