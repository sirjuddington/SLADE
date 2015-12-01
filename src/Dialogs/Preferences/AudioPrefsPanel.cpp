
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
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
#include <wx/filename.h>
#include <wx/statline.h>


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, snd_autoplay)
#ifndef NO_FLUIDSYNTH
EXTERN_CVAR(String, fs_soundfont_path)
EXTERN_CVAR(String, dir_last)
EXTERN_CVAR(Bool, dmx_padding)
CVAR(String, dir_last_soundfont, "", CVAR_SAVE)
CVAR(Bool, snd_midi_usetimidity, false, CVAR_SAVE)
#endif
CVAR(String, snd_timidity_path, "", CVAR_SAVE)
CVAR(String, snd_timidity_options, "", CVAR_SAVE)


/*******************************************************************
 * AUDIOPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* AudioPrefsPanel::AudioPrefsPanel
 * AudioPrefsPanel class constructor
 *******************************************************************/
AudioPrefsPanel::AudioPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Audio Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Autoplay
	cb_snd_autoplay = new wxCheckBox(this, -1, "Automatically play audio entries when opened");
	sizer->Add(cb_snd_autoplay, 0, wxEXPAND|wxALL, 4);

#ifndef NO_FLUIDSYNTH
	// DMX Padding
	cb_dmx_padding = new wxCheckBox(this, -1, "Use DMX padding when appropriate");
	sizer->Add(cb_dmx_padding, 0, wxEXPAND|wxALL, 4);

	// MIDI Soundfont path
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(this, -1, "Location of MIDI soundfont:"), 0, wxALL, 4);
	text_soundfont_path = new wxTextCtrl(this, -1);
	hbox->Add(text_soundfont_path, 1, wxEXPAND|wxRIGHT, 4);
	btn_browse_soundfont = new wxButton(this, -1, "Browse");
	hbox->Add(btn_browse_soundfont, 0, wxEXPAND);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
#endif

#ifndef NO_FLUIDSYNTH
	sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL), 0, wxEXPAND|wxALL, 8);

	// Timidity checkbox
	cb_snd_timidity = new wxCheckBox(this, -1, "Use timidity instead of fluidsynth:");
	sizer->Add(cb_snd_timidity, 0, wxEXPAND|wxALL, 4);
#endif

	// Timidity exe path
	wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(this, -1, "Location of timidity binary:"), 0, wxALL, 4);
	text_timidity_path = new wxTextCtrl(this, -1);
	hbox2->Add(text_timidity_path, 1, wxEXPAND|wxRIGHT, 4);
	btn_browse_timidityexe = new wxButton(this, -1, "Browse");
	hbox2->Add(btn_browse_timidityexe, 0, wxEXPAND);
	sizer->Add(hbox2, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Timidity options
	wxBoxSizer* hbox3 = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(this, -1, "More timidity options:"), 0, wxALL, 4);
	text_timidity_options = new wxTextCtrl(this, -1);
	hbox3->Add(text_timidity_options, 1, wxEXPAND|wxRIGHT, 4);
	sizer->Add(hbox3, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL), 0, wxEXPAND | wxALL, 8);

	// Reset
	sizer->Add(new wxStaticText(this, -1, "Reset MIDI Player"), 0, wxALL, 4);
	btn_reset_player = new wxButton(this, -1, "Reset");
	sizer->Add(btn_reset_player, 0, wxEXPAND, 4);

	// Bind events
#ifndef NO_FLUIDSYNTH
	btn_browse_soundfont->Bind(wxEVT_BUTTON, &AudioPrefsPanel::onBtnBrowseSoundfont, this);
#endif
	btn_reset_player->Bind(wxEVT_BUTTON, &AudioPrefsPanel::onBtnResetPlayer, this);
	btn_browse_timidityexe->Bind(wxEVT_BUTTON, &AudioPrefsPanel::onBtnBrowseTimidityExe, this);

	Layout();
}

/* AudioPrefsPanel::~AudioPrefsPanel
 * AudioPrefsPanel class destructor
 *******************************************************************/
AudioPrefsPanel::~AudioPrefsPanel()
{
}

/* AudioPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void AudioPrefsPanel::init()
{
	cb_snd_autoplay->SetValue(snd_autoplay);
#ifndef NO_FLUIDSYNTH
	cb_snd_timidity->SetValue(snd_midi_usetimidity);
	text_soundfont_path->SetValue(wxString(fs_soundfont_path));
#endif
	text_timidity_path->SetValue(wxString(snd_timidity_path));
	text_timidity_options->SetValue(wxString(snd_timidity_options));
}

/* AudioPrefsPanel::applyPreferences
 * Applies preferences from the panel controls
 *******************************************************************/
void AudioPrefsPanel::applyPreferences()
{
	snd_autoplay = cb_snd_autoplay->GetValue();
#ifndef NO_FLUIDSYNTH
	snd_midi_usetimidity = cb_snd_timidity->GetValue();
	dmx_padding = cb_dmx_padding->GetValue();
	fs_soundfont_path = text_soundfont_path->GetValue();
	if (!theMIDIPlayer->isSoundfontLoaded())
		theMIDIPlayer->reloadSoundfont();
#endif
	snd_timidity_path = text_timidity_path->GetValue();
	snd_timidity_options = text_timidity_options->GetValue();
}


/*******************************************************************
 * AUDIOPREFSPANEL CLASS EVENTS
 *******************************************************************/

#ifndef NO_FLUIDSYNTH
/* AudioPrefsPanel::onBtnBrowseSoundfont
 * Called when the browse for soundfont button is clicked
 *******************************************************************/
void AudioPrefsPanel::onBtnBrowseSoundfont(wxCommandEvent& e)
{
#ifdef WIN32
	char separator = ';';
#else
	char separator = ':';
#endif

	string dir = dir_last_soundfont;
	if (dir_last_soundfont.value.empty() && fs_soundfont_path.value.size())
	{
		wxArrayString paths = wxSplit(fs_soundfont_path, separator);
		dir = wxFileName(paths[0]).GetPath();
	}

	// Open file dialog
	wxFileDialog fd(this, "Browse for MIDI Soundfont", dir, "", "Soundfont files (*.sf2)|*.sf2", wxFD_MULTIPLE);
	if (fd.ShowModal() == wxID_OK)
	{
		wxArrayString paths;
		string fullpath = "";
		fd.GetPaths(paths);
		for (size_t a = 0; a < paths.size(); ++a)
			fullpath += paths[a] + separator;
		if (paths.size())
			fullpath.RemoveLast(1);
		text_soundfont_path->SetValue(fullpath);
		dir_last_soundfont = fd.GetDirectory();
	}
}
#endif

/* AudioPrefsPanel::onBtnBrowseTimidityExe
 * Called when the browse for timidity exe is clicked
 *******************************************************************/
void AudioPrefsPanel::onBtnBrowseTimidityExe(wxCommandEvent &e)
{
#ifdef WIN32
	char separator = ';';
#else
	char separator = ':';
#endif

	// Open file dialog
#ifdef WIN32
	wxFileDialog fd(this, "Browse for Timidity binary", "", "", "*.exe", wxFD_MULTIPLE);
#else
	wxFileDialog fd(this, "Browse for Timidity binary", "", "", "", wxFD_MULTIPLE);
#endif

	if (fd.ShowModal() == wxID_OK)
	{
		wxArrayString paths;
		string exepath = "";
		fd.GetPaths(paths);
		for (size_t a = 0; a < paths.size(); ++a)
			exepath += paths[a] + separator;
		if (paths.size())
			exepath.RemoveLast(1);
		text_timidity_path->SetValue(exepath);
		snd_timidity_path = exepath;
	}
}

/* AudioPrefsPanel::onBtnResetPlayer
 * Called for resetting the MIDI player
 *******************************************************************/
void AudioPrefsPanel::onBtnResetPlayer(wxCommandEvent& e)
{
	theMIDIPlayer->resetPlayer();
}
