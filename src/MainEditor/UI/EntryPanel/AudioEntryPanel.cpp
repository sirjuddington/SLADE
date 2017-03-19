
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
#include "AudioEntryPanel.h"
#include "Audio/MIDIPlayer.h"
#include "Audio/ModMusic.h"
#include "Audio/AudioTags.h"
#include "Graphics/Icons.h"
#include "MainEditor/Conversions.h"

#undef Status


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, snd_volume, 100, CVAR_SAVE)
CVAR(Bool, snd_autoplay, false, CVAR_SAVE)
#ifndef NO_FLUIDSYNTH
EXTERN_CVAR(Bool, snd_midi_usetimidity)
#define usetimidity snd_midi_usetimidity
#else
#define usetimidity true
#endif

/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/

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
	num_tracks = 1;
	subsong = 0;
	song_length = 0;
	sound = new sf::Sound();
	music = new sf::Music();
	mod = new ModMusic();

#ifdef __WXMSW__
	wxRegKey key(wxRegKey::HKLM, "Software\\Microsoft\\Active Setup\\Installed Components\\{6BF52A52-394A-11d3-B153-00C04F79FAA6}");
	long value = 0;
	key.QueryValue("IsInstalled", &value);
	if (value == 0)
	{
		LOG_MESSAGE(1, "Windows Media Player not installed, mp3 playback disabled.");
		media_ctrl = NULL;
	}
	else
	{
		LOG_MESSAGE(3, "Windows Media Player installed, using wxMediaCtrl");
		media_ctrl = new wxMediaCtrl(this, -1);
	}
#else
	// Create wxMediaCtrl
	media_ctrl = new wxMediaCtrl(this, -1);
#endif

	// Setup sizer
	wxGridBagSizer* sizer_gb = new wxGridBagSizer(4, 4);
	sizer_main->AddStretchSpacer();
#ifndef __WXOSX__
	if (media_ctrl) sizer_main->Add(media_ctrl, 0);
#endif
	sizer_main->Add(sizer_gb, 0, wxALIGN_CENTER);
	sizer_main->AddStretchSpacer();

	// Add seekbar
	slider_seek = new wxSlider(this, -1, 0, 0, 100);
	sizer_gb->Add(slider_seek, wxGBPosition(0, 0), wxGBSpan(1, 9), wxEXPAND);

	// Add play controls
	btn_play = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "play"));
	sizer_gb->Add(btn_play, wxGBPosition(1, 0));
	btn_pause = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "pause"));
	sizer_gb->Add(btn_pause, wxGBPosition(1, 1));
	btn_stop = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "stop"));
	sizer_gb->Add(btn_stop, wxGBPosition(1, 2));
	btn_prev = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "prev"));
	sizer_gb->Add(btn_prev, wxGBPosition(1, 3));
	btn_next = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "next"));
	sizer_gb->Add(btn_next, wxGBPosition(1, 4));

	// Add title
	txt_title = new wxStaticText(this, -1, wxEmptyString);
	sizer_gb->Add(txt_title, wxGBPosition(3, 0), wxGBSpan(3, 9));

	// Add info
	txt_info = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_BESTWRAP);
	sizer_gb->Add(txt_info, wxGBPosition(6, 0), wxGBSpan(9, 9), wxEXPAND|wxHORIZONTAL);

	// Add track number
	txt_track = new wxStaticText(this, -1, "1/1");
	sizer_gb->Add(txt_track, wxGBPosition(1, 5), wxDefaultSpan, wxALIGN_CENTER);

	// Separator
	sizer_gb->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), wxGBPosition(1, 6), wxDefaultSpan, wxEXPAND|wxLEFT|wxRIGHT, 8);

	// Add volume slider
	sizer_gb->Add(new wxStaticText(this, -1, "Volume:"), wxGBPosition(1, 7), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_volume = new wxSlider(this, -1, 0, 0, 100, wxDefaultPosition, wxSize(128, -1));
	slider_volume->SetValue(snd_volume);
	sizer_gb->Add(slider_volume, wxGBPosition(1, 8));

	// Set volume
	sound->setVolume(snd_volume);
	music->setVolume(snd_volume);
	theMIDIPlayer->setVolume(snd_volume);
	if (media_ctrl) media_ctrl->SetVolume(snd_volume*0.01);
	mod->setVolume(snd_volume);
	//theGMEPlayer->setVolume(snd_volume);
	//theOPLPlayer->setVolume(snd_volume);

	// Disable general entrypanel buttons
	if (media_ctrl) media_ctrl->Show(false);
	toolbar->Show(false);

	// Bind events
	btn_play->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnPlay, this);
	btn_pause->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnPause, this);
	btn_stop->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnStop, this);
	btn_prev->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnPrev, this);
	btn_next->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnNext, this);
	slider_seek->Bind(wxEVT_SLIDER, &AudioEntryPanel::onSliderSeekChanged, this);
	slider_volume->Bind(wxEVT_SLIDER, &AudioEntryPanel::onSliderVolumeChanged, this);
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
	resetStream();

	// Clean up
	delete sound;
	delete music;
	delete mod;
}

/* AudioEntryPanel::loadEntry
 * Loads an entry into the audio entry panel
 *******************************************************************/
bool AudioEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Are we reopening the same entry? For example having looked at
	// a text file or image or any other non-audio entry, then
	// going back to the original audio entry? Then no need to do
	// abort the current song to restart it.
	if (this->entry == entry)
		return true;

	// Stop anything currently playing
	stopStream();
	resetStream();
	opened = false;

	// Enable all playback controls initially
	slider_seek->Enable();
	btn_play->Enable();
	btn_pause->Enable();

	btn_stop->Enable();
	btn_prev->Enable();
	btn_next->Enable();

	// Reset seek slider
	slider_seek->SetValue(0);

	// Delete previous temp file
	if (wxFileExists(prevfile))
		wxRemoveFile(prevfile);

	// Open new data
	this->entry = entry;
	open();

	// Autoplay if option is on
	if (snd_autoplay)
	{
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

/* AudioEntryPanel::statusString
 * Returns a string with extended editing/entry info for the status
 * bar
 *******************************************************************/
string AudioEntryPanel::statusString()
{
	int hours, minutes, seconds, milliseconds = song_length % 1000;
	seconds = (song_length / 1000) % 60;
	minutes = (song_length / 60000) % 60;
	hours = (song_length / 3600000);
	string ret = wxEmptyString;
	if (hours) ret = S_FMT("%d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
	else if (minutes) ret = S_FMT("%d:%02d.%03d", minutes, seconds, milliseconds);
	else ret = S_FMT("%d:%03d", seconds, milliseconds);

	return ret;
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
	song_length = duration;
}

/* AudioEntryPanel::open
 * Opens the current entry and performs the appropriate conversions
 *******************************************************************/
bool AudioEntryPanel::open()
{
	// Check if already opened
	if (opened)
		return true;

	// Stop if sound currently playing
	resetStream();

	subsong = 0;
	num_tracks = 1;

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
	else if (entry->getType()->getFormat() == "snd_speaker")	// Doom PC Speaker Sound -> WAV
		Conversions::spkSndToWav(mcdata, convdata);
	else if (entry->getType()->getFormat() == "snd_audiot")		// AudioT PC Speaker Sound -> WAV
		Conversions::spkSndToWav(mcdata, convdata, true);
	else if (entry->getType()->getFormat() == "snd_wolf")		// Wolfenstein 3D Sound -> WAV
		Conversions::wolfSndToWav(mcdata, convdata);
	else if (entry->getType()->getFormat() == "snd_voc")		// Creative Voice File -> WAV
		Conversions::vocToWav(mcdata, convdata);
	else if (entry->getType()->getFormat() == "snd_jaguar")		// Jaguar Doom Sound -> WAV
		Conversions::jagSndToWav(mcdata, convdata);
	else if (entry->getType()->getFormat() == "snd_bloodsfx")	// Blood Sound -> WAV
		Conversions::bloodToWav(entry, convdata);
	else if (entry->getType()->getFormat() == "midi_mus")  			// MUS -> MIDI
	{
		Conversions::musToMidi(mcdata, convdata);
		path.SetExt("mid");
	}
	else if (entry->getType()->getFormat() == "midi_xmi" ||  		// HMI/HMP/XMI -> MIDI
			 entry->getType()->getFormat() == "midi_hmi" || entry->getType()->getFormat() == "midi_hmp")
	{
		Conversions::zmusToMidi(mcdata, convdata, 0, &num_tracks);
		path.SetExt("mid");
	}
	else if (entry->getType()->getFormat() == "midi_gmid")  		// GMID -> MIDI
	{
		Conversions::gmidToMidi(mcdata, convdata);
		path.SetExt("mid");
	}
	else
		convdata.importMem(mcdata.getData(), mcdata.getSize());

	// MIDI format
	if (entry->getType()->getFormat().StartsWith("midi_"))
	{
		audio_type = AUTYPE_MIDI;
		openMidi(convdata, path.GetFullPath());
	}

	// MOD format
	else if (entry->getType()->getFormat().StartsWith("mod_"))
		openMod(convdata);

	// Other format
	else
		openAudio(convdata, path.GetFullPath());

	// Keep filename so we can delete it later
	prevfile = path.GetFullPath();

	txt_title->SetLabel(entry->getPath(true));
	txt_track->SetLabel(S_FMT("%d/%d", subsong+1, num_tracks));
	updateInfo();

	// Disable prev/next track buttons if only one track is available
	if (num_tracks < 2)
	{
		btn_prev->Disable();
		btn_next->Disable();
	}

	opened = true;
	return true;
}

/* AudioEntryPanel::openAudio
 * Opens an audio file for playback (SFML 2.x+)
 *******************************************************************/
bool AudioEntryPanel::openAudio(MemChunk& audio, string filename)
{
	// Stop if sound currently playing
	resetStream();

	// (Re)create sound buffer
	if (sound_buffer)
		delete sound_buffer;
	sound_buffer = new sf::SoundBuffer();
	audio_type = AUTYPE_INVALID;

	// Load into buffer
	if (sound_buffer->loadFromMemory((const char*)audio.getData(), audio.getSize()))
	{
		LOG_MESSAGE(3, "opened as sound");
		// Bind to sound
		sound->setBuffer(*sound_buffer);
		audio_type = AUTYPE_SOUND;

		// Enable play controls
#if (SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR < 2)
		// SFML before 2.2 has a bug where it reports an incorrect value for long sounds, so compute it ourselves then
		setAudioDuration((sound_buffer->getSampleCount() / sound_buffer->getSampleRate())*(1000/sound_buffer->getChannelCount()));
#else
		setAudioDuration(sound_buffer->getDuration().asMilliseconds());
#endif
		btn_play->Enable();
		btn_pause->Enable();
		btn_stop->Enable();

		return true;
	}
	else if (music->openFromMemory((const char*)audio.getData(), audio.getSize()))
	{
		LOG_MESSAGE(3, "opened as music");
		// Couldn't open the audio as a sf::SoundBuffer, try sf::Music instead
		audio_type = AUTYPE_MUSIC;

		// Enable play controls
		setAudioDuration(music->getDuration().asMilliseconds());
		btn_play->Enable();
		btn_stop->Enable();

		return true;
	}
	else
	{
		// Couldn't open as sound or music, try the wxMediaCtrl
		LOG_MESSAGE(3, "opened as media");

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
bool AudioEntryPanel::openMidi(MemChunk& data, string filename)
{
	// Enable volume control
	slider_volume->Enable(true);

	// Attempt to open midi
	if (theMIDIPlayer->isReady())
	{
		if (theMIDIPlayer->openData(data))
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

	return false;
}

/* AudioEntryPanel::openMod
* Opens a Module file for playback
*******************************************************************/
bool AudioEntryPanel::openMod(MemChunk& data)
{
	// Attempt to load the mod
	if (mod->loadFromMemory(data.getData(), data.getSize()))
	{
		audio_type = AUTYPE_MOD;

		// Enable playback controls
		slider_volume->Enable();
		btn_play->Enable();
		btn_pause->Enable();
		btn_stop->Enable();
		setAudioDuration(mod->getDuration().asMilliseconds());

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
	return false;
}

/* AudioEntryPanel::openMedia
* Opens audio file [filename] in the wxMediaCtrl
*******************************************************************/
bool AudioEntryPanel::openMedia(string filename)
{
	// Attempt to open with wxMediaCtrl
	if (media_ctrl && media_ctrl->Load(filename))
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
		sound->play(); break;
	case AUTYPE_MUSIC:
		music->play(); break;
	case AUTYPE_MOD:
		mod->play(); break;
	case AUTYPE_MIDI:
		theMIDIPlayer->play(); break;
	case AUTYPE_MEDIA:
		if (media_ctrl) media_ctrl->Play(); break;
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
		sound->pause(); break;
	case AUTYPE_MUSIC:
		music->pause(); break;
	case AUTYPE_MOD:
		mod->pause(); break;
	case AUTYPE_MIDI:
		theMIDIPlayer->pause();	break;
	case AUTYPE_MEDIA:
		if (media_ctrl) media_ctrl->Pause(); break;
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
		sound->stop(); break;
	case AUTYPE_MUSIC:
		music->stop(); break;
	case AUTYPE_MOD:
		mod->stop(); break;
	case AUTYPE_MIDI:
		theMIDIPlayer->stop(); break;
	case AUTYPE_MEDIA:
		if (media_ctrl) media_ctrl->Stop(); break;
	}
}

/* AudioEntryPanel::updateInfo
 * Used to update the info area, returns true if info is non-empty
 *******************************************************************/
bool AudioEntryPanel::updateInfo()
{
	txt_info->Clear();
	string info = entry->getTypeString() + "\n";
	MemChunk& mc = entry->getMCData();
	switch (audio_type)
	{
	case AUTYPE_SOUND:
	case AUTYPE_MUSIC:
	case AUTYPE_MEDIA:
		if (entry->getType() == EntryType::getType("snd_doom"))
		{
			size_t samplerate = READ_L16(mc, 2);
			size_t samples = READ_L16(mc, 4);
			info += S_FMT("%lu samples at %lu Hz", (unsigned long) samples, (unsigned long) samplerate);
		}
		else if (entry->getType() == EntryType::getType("snd_speaker"))
		{
			size_t samples = READ_L16(mc, 2);
			info += S_FMT("%lu samples", (unsigned long) samples);
		}
		else if (entry->getType() == EntryType::getType("snd_audiot"))
		{
			size_t samples = READ_L16(mc, 0);
			info += S_FMT("%lu samples", (unsigned long) samples);
		}
		else if (entry->getType() == EntryType::getType("snd_sun"))
			info += Audio::getSunInfo(mc);
		else if (entry->getType() == EntryType::getType("snd_voc"))
			info += Audio::getVocInfo(mc);
		else if (entry->getType() == EntryType::getType("snd_wav"))
			info += Audio::getWavInfo(mc);
		else if (entry->getType() == EntryType::getType("snd_mp3"))
			info += Audio::getID3Tag(mc);
		else if (entry->getType() == EntryType::getType("snd_ogg"))
			info += Audio::getOggComments(mc);
		else if (entry->getType() == EntryType::getType("snd_flac"))
			info += Audio::getFlacComments(mc);
		else if (entry->getType() == EntryType::getType("snd_aiff"))
			info += Audio::getAiffInfo(mc);
		break;
	case AUTYPE_MOD:
		if (entry->getType() == EntryType::getType("mod_it"))
			info += Audio::getITComments(mc);
		else if (entry->getType() == EntryType::getType("mod_mod"))
			info += Audio::getModComments(mc);
		else if (entry->getType() == EntryType::getType("mod_s3m"))
			info += Audio::getS3MComments(mc);
		else if (entry->getType() == EntryType::getType("mod_xm"))
			info += Audio::getXMComments(mc);
		break;
	case AUTYPE_MIDI:
		info += theMIDIPlayer->getInfo();
		if (entry->getType() == EntryType::getType("midi_rmid"))
			info+= Audio::getRmidInfo(mc);
		break;
	/*case AUTYPE_EMU:
		info += theGMEPlayer->getInfo(subsong);
		break;
	case AUTYPE_OPL:
		if (entry->getType() == EntryType::getType("opl_audiot"))
		{
			size_t samples = READ_L32(mc, 0);
			info += S_FMT("%zu samples", samples);
		}
		info += theOPLPlayer->getInfo();
		break;*/
	}
	txt_info->SetValue(info);
	if (info.length())
		return true;
	return false;
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

/* AudioEntryPanel::onBtnPrev
 * Called when the previous track button is pressed
 *******************************************************************/
void AudioEntryPanel::onBtnPrev(wxCommandEvent& e)
{
	if (subsong > 0)
		subsong--;
	else subsong = num_tracks - 1;

	if (entry->getType()->getFormat() == "xmi")
	{
		MemChunk& mcdata = entry->getMCData();
		MemChunk convdata;
		if (Conversions::zmusToMidi(mcdata, convdata, subsong))
			openMidi(convdata, prevfile);
	}
	//else if (entry->getType()->getFormat().StartsWith("gme"))
	//	theGMEPlayer->play(subsong);
	txt_track->SetLabel(S_FMT("%d/%d", subsong+1, num_tracks));
	updateInfo();
}

/* AudioEntryPanel::onBtnNext
 * Called when the next track button is pressed
 *******************************************************************/
void AudioEntryPanel::onBtnNext(wxCommandEvent& e)
{
	int newsong = (subsong + 1) % num_tracks;
	if (entry->getType()->getFormat() == "xmi")
	{
		MemChunk& mcdata = entry->getMCData();
		MemChunk convdata;
		if (Conversions::zmusToMidi(mcdata, convdata, newsong) && openMidi(convdata, prevfile))
			subsong = newsong;
	}
	/*else if (entry->getType()->getFormat().StartsWith("gme"))
	{
		if (theGMEPlayer->play(newsong))
			subsong = newsong;
	}*/
	txt_track->SetLabel(S_FMT("%d/%d", subsong+1, num_tracks));
	updateInfo();
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
		pos = sound->getPlayingOffset().asMilliseconds(); break;
	case AUTYPE_MUSIC:
		pos = music->getPlayingOffset().asMilliseconds(); break;
	case AUTYPE_MOD:
		pos = mod->getPlayingOffset().asMilliseconds(); break;
	case AUTYPE_MIDI:
		pos = theMIDIPlayer->getPosition(); break;
	case AUTYPE_MEDIA:
		if (media_ctrl) pos = media_ctrl->Tell(); break;
	}

	// Set slider
	slider_seek->SetValue(pos);

	// Stop the timer if playback has reached the end
	if (pos >= slider_seek->GetMax() ||
	        (audio_type == AUTYPE_SOUND && sound->getStatus() == sf::Sound::Stopped) ||
	        (audio_type == AUTYPE_MUSIC && music->getStatus() == sf::Sound::Stopped) ||
			(audio_type == AUTYPE_MOD && mod->getStatus() == sf::Sound::Stopped) ||
			(audio_type == AUTYPE_MEDIA && media_ctrl && media_ctrl->GetState() == wxMEDIASTATE_STOPPED) ||
			(audio_type == AUTYPE_MIDI && theMIDIPlayer && !theMIDIPlayer->isPlaying()))
	{
		timer_seek->Stop();
		slider_seek->SetValue(0);
	}
	
}

/* AudioEntryPanel::onSliderSeekChanged
 * Called when the seek slider position is changed
 *******************************************************************/
void AudioEntryPanel::onSliderSeekChanged(wxCommandEvent& e)
{
	switch (audio_type)
	{
	case AUTYPE_SOUND:
		sound->setPlayingOffset(sf::milliseconds(slider_seek->GetValue())); break;
	case AUTYPE_MUSIC:
		music->setPlayingOffset(sf::milliseconds(slider_seek->GetValue())); break;
	case AUTYPE_MOD:
		mod->setPlayingOffset(sf::milliseconds(slider_seek->GetValue())); break;
	case AUTYPE_MIDI:
		theMIDIPlayer->setPosition(slider_seek->GetValue()); break;
	case AUTYPE_MEDIA:
		if (media_ctrl) media_ctrl->Seek(slider_seek->GetValue()); break;
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
		sound->setVolume(snd_volume); break;
	case AUTYPE_MUSIC:
		music->setVolume(snd_volume); break;
	case AUTYPE_MIDI:
		theMIDIPlayer->setVolume(snd_volume); break;
	case AUTYPE_MEDIA:
		if (media_ctrl) media_ctrl->SetVolume(snd_volume*0.01); break;
	case AUTYPE_MOD:
		mod->setVolume(snd_volume); break;
	}
}
