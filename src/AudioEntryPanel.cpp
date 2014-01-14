
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    AudioEntryPanel.cpp
 * Description: AudioEntryPanel class. The UI for previewing supported
 *              audio entries
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
#include "AudioEntryPanel.h"
#include "Icons.h"
#include "Conversions.h"
#include "MIDIPlayer.h"
#include <wx/filename.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, snd_volume, 100, CVAR_SAVE)
CVAR(Bool, snd_autoplay, false, CVAR_SAVE)


/*******************************************************************
 * AUDIOENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* AudioEntryPanel::AudioEntryPanel
 * AudioEntryPanel class constructor
 *******************************************************************/
AudioEntryPanel::AudioEntryPanel(wxWindow* parent) : EntryPanel(parent, "audio")
{
	// Init variables
	timer_seek = new wxTimer(this);
	sound_buffer = NULL;
	audio_type = AUTYPE_INVALID;

	// Create wxMediaCtrl
	media_ctrl = new wxMediaCtrl(this, -1);

	// Setup sizer
	wxGridBagSizer* sizer_gb = new wxGridBagSizer(4, 4);
	sizer_main->AddStretchSpacer();
#ifndef __WXOSX__
	sizer_main->Add(media_ctrl, 0);
#endif
	sizer_main->Add(sizer_gb, 0, wxALIGN_CENTER);
	sizer_main->AddStretchSpacer();

	// Add seekbar
	slider_seek = new wxSlider(this, -1, 0, 0, 100);
	sizer_gb->Add(slider_seek, wxGBPosition(0, 0), wxGBSpan(1, 6), wxEXPAND);

	// Add play controls
	btn_play = new wxBitmapButton(this, -1, getIcon("i_play"));
	sizer_gb->Add(btn_play, wxGBPosition(1, 0));
	btn_pause = new wxBitmapButton(this, -1, getIcon("i_pause"));
	sizer_gb->Add(btn_pause, wxGBPosition(1, 1));
	btn_stop = new wxBitmapButton(this, -1, getIcon("i_stop"));
	sizer_gb->Add(btn_stop, wxGBPosition(1, 2));

	// Separator
	sizer_gb->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), wxGBPosition(1, 3), wxDefaultSpan, wxEXPAND|wxLEFT|wxRIGHT, 8);

	// Add volume slider
	sizer_gb->Add(new wxStaticText(this, -1, "Volume:"), wxGBPosition(1, 4), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_volume = new wxSlider(this, -1, 0, 0, 100, wxDefaultPosition, wxSize(128, -1));
	slider_volume->SetValue(snd_volume);
	sizer_gb->Add(slider_volume, wxGBPosition(1, 5));

	// Set volume
	sound.setVolume(snd_volume);
	music.setVolume(snd_volume);
	theMIDIPlayer->setVolume(snd_volume);
	media_ctrl->SetVolume(snd_volume*0.01);
#ifndef NOLIBMODPLUG
	mod.setVolume(snd_volume);
#endif

	// Disable general entrypanel buttons
	media_ctrl->Show(false);
	toolbar->Show(false);

	// Bind events
	btn_play->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AudioEntryPanel::onBtnPlay, this);
	btn_pause->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AudioEntryPanel::onBtnPause, this);
	btn_stop->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &AudioEntryPanel::onBtnStop, this);
	slider_seek->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &AudioEntryPanel::onSliderSeekChanged, this);
	slider_volume->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &AudioEntryPanel::onSliderVolumeChanged, this);
	Bind(wxEVT_TIMER, &AudioEntryPanel::onTimer, this);

	Layout();
}

/* AudioEntryPanel::~AudioEntryPanel
 * AudioEntryPanel class constructor
 *******************************************************************/
AudioEntryPanel::~AudioEntryPanel()
{
	// Stop the timer to avoid crashes
	timer_seek->Stop();
	theMIDIPlayer->stop();
}

/* AudioEntryPanel::loadEntry
 * Loads an entry into the audio entry panel
 *******************************************************************/
bool AudioEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Stop anything currently playing
	stopStream();
	resetStream();
	opened = false;

	// Enable all playback controls initially
	slider_seek->Enable();
	btn_play->Enable();
	btn_pause->Enable();
	btn_stop->Enable();

	// Reset seek slider
	slider_seek->SetValue(0);

	// Delete previous temp file
	if (wxFileExists(prevfile))
		wxRemoveFile(prevfile);

	// Autoplay if option is on
	if (snd_autoplay)
	{
		this->entry = entry;
		startStream();
		timer_seek->Start(10);
	}

	Refresh();
	return true;
}

/* AudioEntryPanel::saveEntry
 * Saves any changes to the entry (does nothing here)
 *******************************************************************/
bool AudioEntryPanel::saveEntry()
{
	return true;
}

/* AudioEntryPanel::setAudioDuration
 * Sets the seek slider control duration
 *******************************************************************/
void AudioEntryPanel::setAudioDuration(int duration)
{
	if (duration == 0)
	{
		slider_seek->Enable(false);
		slider_seek->SetRange(0, 0);
	}
	else
	{
		slider_seek->Enable(true);
		slider_seek->SetRange(0, duration);
		slider_seek->SetPageSize(duration * 0.1);
	}
}

/* AudioEntryPanel::open
 * Opens the current entry and performs the appropriate conversions
 *******************************************************************/
bool AudioEntryPanel::open()
{
	// Check if already opened
	if (opened)
		return true;

	// Get entry data
	MemChunk& mcdata = entry->getMCData();

	// Setup temp filename
	wxFileName path(appPath(entry->getName(), DIR_TEMP));
	// Add extension if missing
	if (path.GetExt().IsEmpty())
		path.SetExt(entry->getType()->getExtension());

	// Convert if necessary, then write to file
	MemChunk convdata;
	if (entry->getType()->getFormat() == "snd_doom" ||			// Doom Sound -> WAV
	        entry->getType()->getFormat() == "snd_doom_mac")
		Conversions::doomSndToWav(mcdata, convdata);
	else if (entry->getType()->getFormat() == "snd_wolf")		// Wolfenstein 3D Sound -> WAV
		Conversions::wolfSndToWav(mcdata, convdata);
	else if (entry->getType()->getFormat() == "snd_voc")		// Creative Voice File -> WAV
		Conversions::vocToWav(mcdata, convdata);
	else if (entry->getType()->getFormat() == "snd_jaguar")		// Jaguar Doom Sound -> WAV
		Conversions::jagSndToWav(mcdata, convdata);
	else if (entry->getType()->getFormat() == "snd_bloodsfx")	// Blood Sound -> WAV
		Conversions::bloodToWav(entry, convdata);
	else if (entry->getType()->getFormat() == "mus")  			// MUS -> MIDI
	{
		Conversions::musToMidi(mcdata, convdata);
		path.SetExt("mid");
	}
	else if (entry->getType()->getFormat() == "gmid")  			// GMID -> MIDI
	{
		Conversions::gmidToMidi(mcdata, convdata);
		path.SetExt("mid");
	}
	else
		convdata.importMem(mcdata.getData(), mcdata.getSize());

	// MIDI format
	if (entry->getType()->getFormat() == "midi" || entry->getType()->getFormat() == "mus" ||
	        entry->getType()->getFormat() == "gmid")
	{
		audio_type = AUTYPE_MIDI;
		convdata.exportFile(path.GetFullPath());
		openMidi(path.GetFullPath());
	}

	// MOD format
	else if (entry->getType()->getFormat().StartsWith("mod_"))
		openMod(convdata);

	// Other format
	else
		openAudio(convdata, path.GetFullPath());

	// Keep filename so we can delete it later
	prevfile = path.GetFullPath();

	opened = true;
	return true;
}

/* AudioEntryPanel::openAudio
 * Opens an audio file for playback (SFML 2.x+)
 *******************************************************************/
bool AudioEntryPanel::openAudio(MemChunk& audio, string filename)
{
	// Stop if sound currently playing
	sound.stop();
	music.stop();
#ifndef NOLIBMODPLUG
	mod.stop();
#endif

	// (Re)create sound buffer
	if (sound_buffer)
		delete sound_buffer;
	sound_buffer = new sf::SoundBuffer();
	audio_type = AUTYPE_INVALID;

	// Load into buffer
	if (sound_buffer->loadFromMemory((const char*)audio.getData(), audio.getSize()))
	{
		wxLogMessage("opened as sound");
		// Bind to sound
		sound.setBuffer(*sound_buffer);
		audio_type = AUTYPE_SOUND;

		// Enable play controls
		setAudioDuration(sound_buffer->getDuration().asMilliseconds());
		btn_play->Enable();
		btn_pause->Enable();
		btn_stop->Enable();

		return true;
	}
	else if (music.openFromMemory((const char*)audio.getData(), audio.getSize()))
	{
		wxLogMessage("opened as music");
		// Couldn't open the audio as a sf::SoundBuffer, try sf::Music instead
		audio_type = AUTYPE_MUSIC;

		// Enable play controls
		setAudioDuration(music.getDuration().asMilliseconds());
		wxLogMessage("duration: %dms", music.getDuration().asMilliseconds());
		btn_play->Enable();
		btn_stop->Enable();

		return true;
	}
	else
	{
		// Couldn't open as sound or music, try the wxMediaCtrl

		// Dump audio to temp file
		audio.exportFile(filename);

		if (openMedia(filename))
			return true;
	}

	// Unable to open audio, disable play controls
	setAudioDuration(0);
	btn_play->Enable(false);
	btn_pause->Enable(false);
	btn_stop->Enable(false);

	return false;
}

/* AudioEntryPanel::openMidi
 * Opens a MIDI file for playback
 *******************************************************************/
bool nosf_warned = false;	// One-time 'no soundfont loaded' warning
bool AudioEntryPanel::openMidi(string filename)
{
	// Enable volume control
	slider_volume->Enable(true);

	// Attempt to open midi
	if (theMIDIPlayer->isInitialised() && theMIDIPlayer->isSoundfontLoaded())
	{
		if (theMIDIPlayer->openFile(filename))
		{
			// Enable play controls
			btn_play->Enable();
			btn_pause->Enable();
			btn_stop->Enable();

			// Setup seekbar
			setAudioDuration(theMIDIPlayer->getLength());

			return true;
		}
	}
	else
	{
		// MIDI Player not initialised (no soundfont set), attempt to open with wxMediaCtrl
		if (openMedia(filename))
			return true;
	}

	// Disable play controls
	btn_play->Enable(false);
	btn_pause->Enable(false);
	btn_stop->Enable(false);
	setAudioDuration(0);

	return false;
}

/* AudioEntryPanel::openMod
* Opens a Module file for playback
*******************************************************************/
bool AudioEntryPanel::openMod(MemChunk& data)
{
#ifndef NOLIBMODPLUG
	// Attempt to load the mod
	if (mod.LoadFromMemory(data.getData(), data.getSize()))
	{
		audio_type = AUTYPE_MOD;

		// Enable playback controls
		slider_volume->Enable();
		btn_play->Enable();
		btn_pause->Enable();
		btn_stop->Enable();
		setAudioDuration(mod.GetLength());

		return true;
	}
	else
	{
		// Disable playback controls
		slider_volume->Enable();
		btn_play->Enable();
		btn_pause->Enable();
		btn_stop->Enable();
		setAudioDuration(0);

		return false;
	}
#endif
	return false;
}

/* AudioEntryPanel::openMedia
* Opens audio file [filename] in the wxMediaCtrl
*******************************************************************/
bool AudioEntryPanel::openMedia(string filename)
{
	// Attempt to open with wxMediaCtrl
	if (media_ctrl->Load(filename))
	{
		// Loaded successfully
		audio_type = AUTYPE_MEDIA;

		// Enable play controls
		setAudioDuration(media_ctrl->Length());
		btn_play->Enable(true);
		btn_pause->Enable(true);
		btn_stop->Enable(true);

		return true;
	}
	
	return false;
}

/* AudioEntryPanel::startStream
 * Begins playback of the current audio or MIDI stream
 *******************************************************************/
void AudioEntryPanel::startStream()
{
	if (!opened)
		open();

	switch (audio_type)
	{
	case AUTYPE_SOUND:
		sound.play(); break;
	case AUTYPE_MUSIC:
		music.play(); break;
#ifndef NOLIBMODPLUG
	case AUTYPE_MOD:
		mod.play(); break;
#endif
	case AUTYPE_MIDI:
		theMIDIPlayer->play(); break;
	case AUTYPE_MEDIA:
		media_ctrl->Play(); break;
	}
}

/* AudioEntryPanel::stopStream
 * Stops playback of the current audio or MIDI stream
 *******************************************************************/
void AudioEntryPanel::stopStream()
{
	switch (audio_type)
	{
	case AUTYPE_SOUND:
		sound.pause(); break;
	case AUTYPE_MUSIC:
		music.pause(); break;
#ifndef NOLIBMODPLUG
	case AUTYPE_MOD:
		mod.pause(); break;
#endif
	case AUTYPE_MIDI:
		theMIDIPlayer->pause(); break;
	case AUTYPE_MEDIA:
		media_ctrl->Pause(); break;
	}
}

/* AudioEntryPanel::resetStream
 * Resets the current audio or MIDI stream to the beginning
 *******************************************************************/
void AudioEntryPanel::resetStream()
{

	switch (audio_type)
	{
	case AUTYPE_SOUND:
		sound.stop(); break;
	case AUTYPE_MUSIC:
		music.stop(); break;
#ifndef NOLIBMODPLUG
	case AUTYPE_MOD:
		mod.stop(); break;
#endif
	case AUTYPE_MIDI:
		theMIDIPlayer->stop(); break;
	case AUTYPE_MEDIA:
		media_ctrl->Stop(); break;
	}
}


/*******************************************************************
 * AUDIOENTRYPANEL CLASS EVENTS
 *******************************************************************/

/* AudioEntryPanel::onBtnPlay
 * Called when the play button is pressed
 *******************************************************************/
void AudioEntryPanel::onBtnPlay(wxCommandEvent& e)
{
	startStream();
	timer_seek->Start(10);
}

/* AudioEntryPanel::onBtnPause
 * Called when the pause button is pressed
 *******************************************************************/
void AudioEntryPanel::onBtnPause(wxCommandEvent& e)
{
	// Stop playing (no reset)
	stopStream();
	timer_seek->Stop();
}

/* AudioEntryPanel::onBtnStop
 * Called when the stop button is pressed
 *******************************************************************/
void AudioEntryPanel::onBtnStop(wxCommandEvent& e)
{
	// Stop playing
	stopStream();
	timer_seek->Stop();

	// Reset
	resetStream();
	slider_seek->SetValue(0);
}

/* AudioEntryPanel::onTimer
 * Called when the playback timer ticks
 *******************************************************************/
void AudioEntryPanel::onTimer(wxTimerEvent& e)
{
	// Get current playback position
	int pos = 0;

	switch (audio_type)
	{
	case AUTYPE_SOUND:
		pos = sound.getPlayingOffset().asMilliseconds(); break;
	case AUTYPE_MUSIC:
		pos = music.getPlayingOffset().asMilliseconds(); break;
#ifndef NOLIBMODPLUG
	case AUTYPE_MOD:
		pos = mod.getPlayingOffset().asMilliseconds(); break;
#endif
	case AUTYPE_MIDI:
		pos = theMIDIPlayer->getPosition(); break;
	case AUTYPE_MEDIA:
		pos = media_ctrl->Tell(); break;
	}

	// Set slider
	slider_seek->SetValue(pos);

	// Stop the timer if playback has reached the end
	if (pos >= slider_seek->GetMax() ||
	        (audio_type == AUTYPE_SOUND && sound.getStatus() == sf::Sound::Stopped) ||
	        (audio_type == AUTYPE_MUSIC && music.getStatus() == sf::Sound::Stopped) ||
#ifndef NOLIBMODPLUG
	        (audio_type == AUTYPE_MOD && mod.getStatus() == sf::Sound::Stopped) ||
#endif
	        (audio_type == AUTYPE_MEDIA && media_ctrl->GetState() == wxMEDIASTATE_STOPPED))
		timer_seek->Stop();
}

/* AudioEntryPanel::onSliderSeekChanged
 * Called when the seek slider position is changed
 *******************************************************************/
void AudioEntryPanel::onSliderSeekChanged(wxCommandEvent& e)
{
	switch (audio_type)
	{
	case AUTYPE_SOUND:
		sound.setPlayingOffset(sf::milliseconds(slider_seek->GetValue())); break;
	case AUTYPE_MUSIC:
		music.setPlayingOffset(sf::milliseconds(slider_seek->GetValue())); break;
#ifndef NOLIBMODPLUG
	case AUTYPE_MOD:
		mod.setPlayingOffset(sf::milliseconds(slider_seek->GetValue())); break;
#endif
	case AUTYPE_MIDI:
		theMIDIPlayer->setPosition(slider_seek->GetValue()); break;
	case AUTYPE_MEDIA:
		media_ctrl->Seek(slider_seek->GetValue()); break;
	}
}

/* AudioEntryPanel::onSliderVolumeChanged
 * Called when the volume slider position is changed
 *******************************************************************/
void AudioEntryPanel::onSliderVolumeChanged(wxCommandEvent& e)
{
	snd_volume = slider_volume->GetValue();

	switch (audio_type)
	{
	case AUTYPE_SOUND:
		sound.setVolume(snd_volume); break;
	case AUTYPE_MUSIC:
		music.setVolume(snd_volume); break;
	case AUTYPE_MIDI:
		theMIDIPlayer->setVolume(snd_volume); break;
	case AUTYPE_MEDIA:
		media_ctrl->SetVolume(snd_volume*0.01); break;
#ifndef NOLIBMODPLUG
	case AUTYPE_MOD:
		mod.setVolume(snd_volume); break;
#endif
	}
}
