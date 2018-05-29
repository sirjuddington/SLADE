
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    AudioPrefsPanel.cpp
// Description: Panel containing Audio preference controls
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
#include "AudioPrefsPanel.h"
#include "Audio/MIDIPlayer.h"
#include "General/UI.h"
#include "UI/Controls/FileLocationPanel.h"


// ----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(Bool, snd_autoplay)
EXTERN_CVAR(Bool, dmx_padding)
EXTERN_CVAR(Int, snd_volume)
EXTERN_CVAR(String, fs_soundfont_path)
EXTERN_CVAR(String, dir_last)
CVAR(String, dir_last_soundfont, "", CVAR_SAVE)
CVAR(Bool, snd_midi_usetimidity, false, CVAR_SAVE)
CVAR(String, snd_timidity_path, "", CVAR_SAVE)
CVAR(String, snd_timidity_options, "", CVAR_SAVE)


// ----------------------------------------------------------------------------
//
// AudioPrefsPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// AudioPrefsPanel::AudioPrefsPanel
//
// AudioPrefsPanel class constructor
// ----------------------------------------------------------------------------
AudioPrefsPanel::AudioPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create controls
	cb_snd_autoplay_ = new wxCheckBox(this, -1, "Automatically play audio entries when opened");
	cb_dmx_padding_ = new wxCheckBox(this, -1, "Use DMX padding when appropriate");
	rb_fluidsynth_ = new wxRadioButton(this, -1, "Use Fluidsynth");
	flp_soundfont_ = new FileLocationPanel(
		this,
		"",
		true,
		"Browse for MIDI Soundfont",
		"Soundfont files (*.sf2)|*.sf2"
	);
	rb_timidity_ = new wxRadioButton(this, -1, "Use Timidity");
	flp_timidity_ = new FileLocationPanel(
		this,
		"",
		true,
		"Browse for Timidity Executable",
		SFileDialog::executableExtensionString()
	);
	text_timidity_options_ = new wxTextCtrl(this, -1);
	btn_reset_player_ = new wxButton(this, -1, "Reset MIDI Player");

	setupLayout();

	// Bind events
	btn_reset_player_->Bind(wxEVT_BUTTON, &AudioPrefsPanel::onBtnResetPlayer, this);
	rb_fluidsynth_->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent& e) { updateControls(); });
	rb_timidity_->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent& e) { updateControls(); });

#ifdef NO_FLUIDSYNTH
	rb_fluidsynth_->Enable(false);
	rb_timidity_->SetValue(true);
#endif

	Layout();
}

// ----------------------------------------------------------------------------
// AudioPrefsPanel::~AudioPrefsPanel
//
// AudioPrefsPanel class destructor
// ----------------------------------------------------------------------------
AudioPrefsPanel::~AudioPrefsPanel()
{
}

// ----------------------------------------------------------------------------
// AudioPrefsPanel::init
//
// Initialises panel controls
// ----------------------------------------------------------------------------
void AudioPrefsPanel::init()
{
	cb_snd_autoplay_->SetValue(snd_autoplay);
	cb_dmx_padding_->SetValue(dmx_padding);
	rb_fluidsynth_->SetValue(!snd_midi_usetimidity);
	flp_soundfont_->setLocation(fs_soundfont_path);
	flp_timidity_->setLocation(snd_timidity_path);
	text_timidity_options_->SetValue(wxString(snd_timidity_options));

	updateControls();
}

// ----------------------------------------------------------------------------
// AudioPrefsPanel::applyPreferences
//
// Applies preferences from the panel controls
// ----------------------------------------------------------------------------
void AudioPrefsPanel::applyPreferences()
{
	snd_autoplay = cb_snd_autoplay_->GetValue();
	dmx_padding = cb_dmx_padding_->GetValue();
	snd_midi_usetimidity = rb_timidity_->GetValue();
	fs_soundfont_path = flp_soundfont_->location();
	if (!theMIDIPlayer->isSoundfontLoaded())
		theMIDIPlayer->reloadSoundfont();
	snd_timidity_path = flp_timidity_->location();
	snd_timidity_options = text_timidity_options_->GetValue();
}

// ----------------------------------------------------------------------------
// AudioPrefsPanel::setupLayout
//
// Lays out the controls on the panel
// ----------------------------------------------------------------------------
void AudioPrefsPanel::setupLayout()
{
	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Autoplay
	sizer->Add(cb_snd_autoplay_, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// DMX Padding
	sizer->Add(cb_dmx_padding_, 0, wxEXPAND);

	sizer->Add(
		new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL),
		0,
		wxEXPAND | wxBOTTOM | wxTOP,
		UI::padLarge()
	);

	// MIDI Playback (fluidsynth/timidity)
	auto gbsizer = new wxGridBagSizer(UI::px(UI::Size::PadMinimum), UI::pad());
	gbsizer->Add(new wxStaticText(this, -1, "MIDI Playback:"), { 0, 0 }, { 1, 2 }, wxEXPAND | wxBOTTOM, UI::pad());
	gbsizer->Add(rb_fluidsynth_, { 1, 0 }, { 1, 1 }, wxEXPAND | wxBOTTOM, UI::pad());
	gbsizer->Add(new wxStaticText(this, -1, "Location of MIDI soundfont:"), { 2, 0 }, { 1, 1 }, wxEXPAND);
	gbsizer->Add(flp_soundfont_, { 3, 0 }, { 1, 1 }, wxEXPAND | wxBOTTOM, UI::pad());
	gbsizer->Add(rb_timidity_, { 1, 1 }, { 1, 1 }, wxEXPAND | wxBOTTOM, UI::pad());
	gbsizer->Add(new wxStaticText(this, -1, "Location of Timidity executable:"), { 2, 1 }, { 1, 1 }, wxEXPAND);
	gbsizer->Add(flp_timidity_, { 3, 1 }, { 1, 1 }, wxEXPAND | wxBOTTOM, UI::pad());
	gbsizer->Add(new wxStaticText(this, -1, "Timidity command line options:"), { 4, 1 }, { 1, 1 }, wxEXPAND);
	gbsizer->Add(text_timidity_options_, { 5, 1 }, { 1, 1 }, wxEXPAND);
	gbsizer->AddGrowableCol(0, 1);
	gbsizer->AddGrowableCol(1, 1);
	sizer->Add(gbsizer, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// Reset MIDI player
	sizer->Add(btn_reset_player_, 0, wxEXPAND);
}

// ----------------------------------------------------------------------------
// AudioPrefsPanel::updateControls
//
// Enables/disables MIDI playback options depending on what playback method is
// currently selected
// ----------------------------------------------------------------------------
void AudioPrefsPanel::updateControls() const
{
	flp_soundfont_->Enable(rb_fluidsynth_->GetValue());
	flp_timidity_->Enable(rb_timidity_->GetValue());
	text_timidity_options_->Enable(rb_timidity_->GetValue());
}


// ----------------------------------------------------------------------------
//
// AudioPrefsPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// AudioPrefsPanel::onBtnResetPlayer
//
// Called for resetting the MIDI player
// ----------------------------------------------------------------------------
void AudioPrefsPanel::onBtnResetPlayer(wxCommandEvent& e)
{
	theMIDIPlayer->resetPlayer();
	theMIDIPlayer->setVolume(snd_volume);
}
