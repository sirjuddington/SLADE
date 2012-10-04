
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    AudioPrefsPanel.cpp
 * Description: Panel containing Audio preference controls
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
#include "WxStuff.h"
#include "AudioPrefsPanel.h"
#include "MIDIPlayer.h"
#include <wx/filedlg.h>


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, snd_autoplay)
EXTERN_CVAR(String, fs_soundfont_path)
EXTERN_CVAR(String, dir_last)


/*******************************************************************
 * AUDIOPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* AudioPrefsPanel::AudioPrefsPanel
 * AudioPrefsPanel class constructor
 *******************************************************************/
AudioPrefsPanel::AudioPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent) {
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox *frame = new wxStaticBox(this, -1, "Audio Preferences");
	wxStaticBoxSizer *sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Autoplay
	cb_snd_autoplay = new wxCheckBox(this, -1, "Automatically play audio entries when opened");
	sizer->Add(cb_snd_autoplay, 0, wxEXPAND|wxALL, 4);

	// MIDI Soundfont path
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(this, -1, "Location of MIDI soundfont:"), 0, wxALL, 4);
	text_soundfont_path = new wxTextCtrl(this, -1);
	hbox->Add(text_soundfont_path, 1, wxEXPAND|wxRIGHT, 4);
	btn_browse_soundfont = new wxButton(this, -1, "Browse");
	hbox->Add(btn_browse_soundfont, 0, wxEXPAND);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Reset
	sizer->Add(new wxStaticText(this, -1, "Reset MIDI Player"), 0, wxALL, 4);
	btn_reset_player = new wxButton(this, -1, "Reset");
	sizer->Add(btn_reset_player, 0, wxEXPAND, 4);

	// Bind events
	btn_browse_soundfont->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AudioPrefsPanel::onBtnBrowseSoundfont, this);
	btn_reset_player->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AudioPrefsPanel::onBtnResetPlayer, this);

	Layout();
}

/* AudioPrefsPanel::~AudioPrefsPanel
 * AudioPrefsPanel class destructor
 *******************************************************************/
AudioPrefsPanel::~AudioPrefsPanel() {
}

/* AudioPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void AudioPrefsPanel::init() {
	cb_snd_autoplay->SetValue(snd_autoplay);
	text_soundfont_path->SetValue(wxString(fs_soundfont_path));
}

/* AudioPrefsPanel::applyPreferences
 * Applies preferences from the panel controls
 *******************************************************************/
void AudioPrefsPanel::applyPreferences() {
	snd_autoplay = cb_snd_autoplay->GetValue();
	fs_soundfont_path = text_soundfont_path->GetValue();
	if (!theMIDIPlayer->isSoundfontLoaded())
		theMIDIPlayer->reloadSoundfont();
}


/*******************************************************************
 * AUDIOPREFSPANEL CLASS EVENTS
 *******************************************************************/

/* AudioPrefsPanel::onBtnBrowseSoundfont
 * Called when the browse for soundfont button is clicked
 *******************************************************************/
void AudioPrefsPanel::onBtnBrowseSoundfont(wxCommandEvent& e) {
	// Open file dialog
	wxFileDialog fd(this, "Browse for MIDI Soundfont", dir_last, "", "Soundfont files (*.sf2)|*.sf2");
	if (fd.ShowModal() == wxID_OK) {
		text_soundfont_path->SetValue(fd.GetPath());
		dir_last = fd.GetDirectory();
	}
}

/* AudioPrefsPanel::onBtnResetPlayer
 * Called for resetting the MIDI player
 *******************************************************************/
void AudioPrefsPanel::onBtnResetPlayer(wxCommandEvent& e) {
	theMIDIPlayer->resetPlayer();
}
