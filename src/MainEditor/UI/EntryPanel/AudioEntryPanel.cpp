
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    AudioEntryPanel.cpp
// Description: The UI for previewing supported audio entries
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
#include "App.h"
#include "AudioEntryPanel.h"
#include "Audio/MIDIPlayer.h"
#include "Audio/ModMusic.h"
#include "Audio/AudioTags.h"
#include "MainEditor/Conversions.h"
#include "UI/WxUtils.h"
#include "UI/Controls/SIconButton.h"

#undef Status


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
CVAR(Int, snd_volume, 100, CVAR_SAVE)
CVAR(Bool, snd_autoplay, false, CVAR_SAVE)
#ifndef NO_FLUIDSYNTH
EXTERN_CVAR(Bool, snd_midi_usetimidity)
#define usetimidity snd_midi_usetimidity
#else
#define usetimidity true
#endif


// ----------------------------------------------------------------------------
//
// AudioEntryPanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// AudioEntryPanel::AudioEntryPanel
//
// AudioEntryPanel class constructor
// ----------------------------------------------------------------------------
AudioEntryPanel::AudioEntryPanel(wxWindow* parent) :
	EntryPanel(parent, "audio"),
	timer_seek_{ new wxTimer(this) },
	sound_{ new sf::Sound() },
	music_{ new sf::Music() },
	mod_{ new ModMusic() }
{
#ifdef __WXMSW__
	wxRegKey key(
		wxRegKey::HKLM,
		"Software\\Microsoft\\Active Setup\\Installed Components\\{6BF52A52-394A-11d3-B153-00C04F79FAA6}"
	);
	long value = 0;
	key.QueryValue("IsInstalled", &value);
	if (value == 0)
	{
		LOG_MESSAGE(1, "Windows Media Player not installed, mp3 playback disabled.");
		media_ctrl_ = nullptr;
	}
	else
	{
		LOG_MESSAGE(3, "Windows Media Player installed, using wxMediaCtrl");
		media_ctrl_ = new wxMediaCtrl(this, -1);
	}
#else
	// Create wxMediaCtrl
	media_ctrl_ = new wxMediaCtrl(this, -1);
#endif

	// Setup sizer
	wxGridBagSizer* sizer_gb = new wxGridBagSizer(UI::pad(), UI::pad());
	sizer_main_->AddStretchSpacer();
#ifndef __WXOSX__
	if (media_ctrl_) sizer_main_->Add(media_ctrl_, 0);
#endif
	sizer_main_->Add(sizer_gb, 0, wxALIGN_CENTER);
	sizer_main_->AddStretchSpacer();

	// Add seekbar
	slider_seek_ = new wxSlider(this, -1, 0, 0, 100);
	sizer_gb->Add(slider_seek_, wxGBPosition(0, 0), wxGBSpan(1, 9), wxEXPAND);

	// Add play controls
	btn_play_ = new SIconButton(this, "play");
	sizer_gb->Add(btn_play_, wxGBPosition(1, 0));
	btn_pause_ = new SIconButton(this, "pause");
	sizer_gb->Add(btn_pause_, wxGBPosition(1, 1));
	btn_stop_ = new SIconButton(this, "stop");
	sizer_gb->Add(btn_stop_, wxGBPosition(1, 2));
	btn_prev_ = new SIconButton(this, "prev");
	sizer_gb->Add(btn_prev_, wxGBPosition(1, 3));
	btn_next_ = new SIconButton(this, "next");
	sizer_gb->Add(btn_next_, wxGBPosition(1, 4));

	// Separator
	sizer_gb->Add(new wxStaticLine(this), { 2, 0 }, { 1, 9 }, wxEXPAND | wxTOP | wxBOTTOM, UI::pad());

	// Add title
	txt_title_ = new wxStaticText(this, -1, wxEmptyString);
	sizer_gb->Add(txt_title_, wxGBPosition(3, 0), wxGBSpan(1, 9));

	// Add info
	txt_info_ = new wxTextCtrl(
		this,
		-1,
		wxEmptyString,
		wxDefaultPosition,
		{ -1, UI::scalePx(200) },
		wxTE_MULTILINE | wxTE_READONLY | wxTE_BESTWRAP
	);
	sizer_gb->Add(txt_info_, wxGBPosition(4, 0), wxGBSpan(1, 9), wxEXPAND|wxHORIZONTAL);

	// Add track number
	txt_track_ = new wxStaticText(this, -1, "1/1");
	sizer_gb->Add(txt_track_, wxGBPosition(1, 5), wxDefaultSpan, wxALIGN_CENTER);

	// Separator
	sizer_gb->Add(
		new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL),
		wxGBPosition(1, 6),
		wxDefaultSpan,
		wxEXPAND
	);

	// Add volume slider
	sizer_gb->Add(new wxStaticText(this, -1, "Volume:"), wxGBPosition(1, 7), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_volume_ = new wxSlider(this, -1, 0, 0, 100, wxDefaultPosition, wxSize(UI::scalePx(128), -1));
	slider_volume_->SetValue(snd_volume);
	sizer_gb->Add(slider_volume_, wxGBPosition(1, 8));

	// Set volume
	sound_->setVolume(snd_volume);
	music_->setVolume(snd_volume);
	theMIDIPlayer->setVolume(snd_volume);
	if (media_ctrl_) media_ctrl_->SetVolume(snd_volume*0.01);
	mod_->setVolume(snd_volume);
	//theGMEPlayer->setVolume(snd_volume);
	//theOPLPlayer->setVolume(snd_volume);

	// Disable general entrypanel buttons
	if (media_ctrl_) media_ctrl_->Show(false);
	toolbar_->Show(false);

	// Bind events
	btn_play_->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnPlay, this);
	btn_pause_->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnPause, this);
	btn_stop_->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnStop, this);
	btn_prev_->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnPrev, this);
	btn_next_->Bind(wxEVT_BUTTON, &AudioEntryPanel::onBtnNext, this);
	slider_seek_->Bind(wxEVT_SLIDER, &AudioEntryPanel::onSliderSeekChanged, this);
	slider_volume_->Bind(wxEVT_SLIDER, &AudioEntryPanel::onSliderVolumeChanged, this);
	Bind(wxEVT_TIMER, &AudioEntryPanel::onTimer, this);

	Layout();
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::~AudioEntryPanel
//
// AudioEntryPanel class destructor
// ----------------------------------------------------------------------------
AudioEntryPanel::~AudioEntryPanel()
{
	// Stop the timer to avoid crashes
	timer_seek_->Stop();
	resetStream();
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::loadEntry
//
// Loads an entry into the audio entry panel
// ----------------------------------------------------------------------------
bool AudioEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Are we reopening the same entry? For example having looked at
	// a text file or image or any other non-audio entry, then
	// going back to the original audio entry? Then no need to do
	// abort the current song to restart it.
	if (this->entry_ == entry)
		return true;

	// Stop anything currently playing
	stopStream();
	resetStream();
	opened_ = false;

	// Enable all playback controls initially
	slider_seek_->Enable();
	btn_play_->Enable();
	btn_pause_->Enable();

	btn_stop_->Enable();
	btn_prev_->Enable();
	btn_next_->Enable();

	// Reset seek slider
	slider_seek_->SetValue(0);

	// Delete previous temp file
	if (wxFileExists(prevfile_))
		wxRemoveFile(prevfile_);

	// Open new data
	this->entry_ = entry;
	open();

	// Autoplay if option is on
	if (snd_autoplay)
	{
		startStream();
		timer_seek_->Start(10);
	}

	Refresh();
	return true;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::saveEntry
//
// Saves any changes to the entry (does nothing here)
// ----------------------------------------------------------------------------
bool AudioEntryPanel::saveEntry()
{
	return true;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::statusString
//
// Returns a string with extended editing/entry info for the status bar
// ----------------------------------------------------------------------------
string AudioEntryPanel::statusString()
{
	int hours, minutes, seconds, milliseconds = song_length_ % 1000;
	seconds = (song_length_ / 1000) % 60;
	minutes = (song_length_ / 60000) % 60;
	hours = (song_length_ / 3600000);
	string ret = wxEmptyString;
	if (hours) ret = S_FMT("%d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
	else if (minutes) ret = S_FMT("%d:%02d.%03d", minutes, seconds, milliseconds);
	else ret = S_FMT("%d.%03d", seconds, milliseconds);

	return ret;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::setAudioDuration
//
// Sets the seek slider control duration
// ----------------------------------------------------------------------------
void AudioEntryPanel::setAudioDuration(int duration)
{
	if (duration == 0)
	{
		slider_seek_->Enable(false);
		slider_seek_->SetRange(0, 0);
	}
	else
	{
		slider_seek_->Enable(true);
		slider_seek_->SetRange(0, duration);
		slider_seek_->SetPageSize(duration * 0.1);
	}
	song_length_ = duration;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::open
//
// Opens the current entry and performs the appropriate conversions
// ----------------------------------------------------------------------------
bool AudioEntryPanel::open()
{
	// Check if already opened
	if (opened_)
		return true;

	// Stop if sound currently playing
	resetStream();

	subsong_ = 0;
	num_tracks_ = 1;

	// Get entry data
	MemChunk& mcdata = entry_->getMCData();

	// Setup temp filename
	wxFileName path(App::path(entry_->getName(), App::Dir::Temp));
	// Add extension if missing
	if (path.GetExt().IsEmpty())
		path.SetExt(entry_->getType()->extension());

	// Convert if necessary, then write to file
	MemChunk convdata;
	if (entry_->getType()->formatId() == "snd_doom" ||			// Doom Sound -> WAV
	        entry_->getType()->formatId() == "snd_doom_mac")
		Conversions::doomSndToWav(mcdata, convdata);
	else if (entry_->getType()->formatId() == "snd_speaker")	// Doom PC Speaker Sound -> WAV
		Conversions::spkSndToWav(mcdata, convdata);
	else if (entry_->getType()->formatId() == "snd_audiot")		// AudioT PC Speaker Sound -> WAV
		Conversions::spkSndToWav(mcdata, convdata, true);
	else if (entry_->getType()->formatId() == "snd_wolf")		// Wolfenstein 3D Sound -> WAV
		Conversions::wolfSndToWav(mcdata, convdata);
	else if (entry_->getType()->formatId() == "snd_voc")		// Creative Voice File -> WAV
		Conversions::vocToWav(mcdata, convdata);
	else if (entry_->getType()->formatId() == "snd_jaguar")		// Jaguar Doom Sound -> WAV
		Conversions::jagSndToWav(mcdata, convdata);
	else if (entry_->getType()->formatId() == "snd_bloodsfx")	// Blood Sound -> WAV
		Conversions::bloodToWav(entry_, convdata);
	else if (entry_->getType()->formatId() == "midi_mus")  			// MUS -> MIDI
	{
		Conversions::musToMidi(mcdata, convdata);
		path.SetExt("mid");
	}
	else if (entry_->getType()->formatId() == "midi_xmi" ||  		// HMI/HMP/XMI -> MIDI
			 entry_->getType()->formatId() == "midi_hmi" || entry_->getType()->formatId() == "midi_hmp")
	{
		Conversions::zmusToMidi(mcdata, convdata, 0, &num_tracks_);
		path.SetExt("mid");
	}
	else if (entry_->getType()->formatId() == "midi_gmid")  		// GMID -> MIDI
	{
		Conversions::gmidToMidi(mcdata, convdata);
		path.SetExt("mid");
	}
	else
		convdata.importMem(mcdata.getData(), mcdata.getSize());

	// MIDI format
	if (entry_->getType()->formatId().StartsWith("midi_"))
	{
		audio_type_ = MIDI;
		openMidi(convdata, path.GetFullPath());
	}

	// MOD format
	else if (entry_->getType()->formatId().StartsWith("mod_"))
		openMod(convdata);

	// Other format
	else
		openAudio(convdata, path.GetFullPath());

	// Keep filename so we can delete it later
	prevfile_ = path.GetFullPath();

	txt_title_->SetLabel(entry_->getPath(true));
	txt_track_->SetLabel(S_FMT("%d/%d", subsong_+1, num_tracks_));
	updateInfo();

	// Disable prev/next track buttons if only one track is available
	if (num_tracks_ < 2)
	{
		btn_prev_->Disable();
		btn_next_->Disable();
	}

	opened_ = true;
	return true;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::openAudio
//
// Opens an audio file for playback (SFML 2.x+)
// ----------------------------------------------------------------------------
bool AudioEntryPanel::openAudio(MemChunk& audio, string filename)
{
	// Stop if sound currently playing
	resetStream();

	// (Re)create sound buffer
	sound_buffer_ = std::make_unique<sf::SoundBuffer>();
	audio_type_ = Invalid;

	// Load into buffer
	if (sound_buffer_->loadFromMemory((const char*)audio.getData(), audio.getSize()))
	{
		LOG_MESSAGE(3, "opened as sound");
		// Bind to sound
		sound_->setBuffer(*sound_buffer_);
		audio_type_ = Sound;

		// Enable play controls
#if (SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR < 2)
		// SFML before 2.2 has a bug where it reports an incorrect value for long sounds, so compute it ourselves then
		setAudioDuration((sound_buffer->getSampleCount() / sound_buffer->getSampleRate())*(1000/sound_buffer->getChannelCount()));
#else
		setAudioDuration(sound_buffer_->getDuration().asMilliseconds());
#endif
		btn_play_->Enable();
		btn_pause_->Enable();
		btn_stop_->Enable();

		return true;
	}
	else if (music_->openFromMemory((const char*)audio.getData(), audio.getSize()))
	{
		LOG_MESSAGE(3, "opened as music");
		// Couldn't open the audio as a sf::SoundBuffer, try sf::Music instead
		audio_type_ = Music;

		// Enable play controls
		setAudioDuration(music_->getDuration().asMilliseconds());
		btn_play_->Enable();
		btn_stop_->Enable();

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
	btn_play_->Enable(false);
	btn_pause_->Enable(false);
	btn_stop_->Enable(false);

	return false;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::openMidi
//
// Opens a MIDI file for playback
// ----------------------------------------------------------------------------
bool AudioEntryPanel::openMidi(MemChunk& data, string filename)
{
	// Enable volume control
	slider_volume_->Enable(true);

	// Attempt to open midi
	if (theMIDIPlayer->isReady())
	{
		if (theMIDIPlayer->openData(data))
		{
			// Enable play controls
			btn_play_->Enable();
			btn_pause_->Enable();
			btn_stop_->Enable();

			// Setup seekbar
			setAudioDuration(theMIDIPlayer->getLength());

			return true;
		}
	}

	return false;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::openMod
//
// Opens a Module file for playback
// ----------------------------------------------------------------------------
bool AudioEntryPanel::openMod(MemChunk& data)
{
	// Attempt to load the mod
	if (mod_->loadFromMemory(data.getData(), data.getSize()))
	{
		audio_type_ = Mod;

		// Enable playback controls
		slider_volume_->Enable();
		btn_play_->Enable();
		btn_pause_->Enable();
		btn_stop_->Enable();
		setAudioDuration(mod_->getDuration().asMilliseconds());

		return true;
	}
	else
	{
		// Disable playback controls
		slider_volume_->Enable();
		btn_play_->Enable();
		btn_pause_->Enable();
		btn_stop_->Enable();
		setAudioDuration(0);

		return false;
	}
	return false;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::openMedia
//
// Opens audio file [filename] in the wxMediaCtrl
// ----------------------------------------------------------------------------
bool AudioEntryPanel::openMedia(string filename)
{
	// Attempt to open with wxMediaCtrl
	if (media_ctrl_ && media_ctrl_->Load(filename))
	{
		// Loaded successfully
		audio_type_ = Media;

		// Enable play controls
		setAudioDuration(media_ctrl_->Length());
		btn_play_->Enable(true);
		btn_pause_->Enable(true);
		btn_stop_->Enable(true);

		return true;
	}
	
	return false;
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::startStream
//
// Begins playback of the current audio or MIDI stream
// ----------------------------------------------------------------------------
void AudioEntryPanel::startStream()
{
	if (!opened_)
		open();

	switch (audio_type_)
	{
	case Sound:
		sound_->play(); break;
	case Music:
		music_->play(); break;
	case Mod:
		mod_->play(); break;
	case MIDI:
		theMIDIPlayer->play(); break;
	case Media:
		if (media_ctrl_) media_ctrl_->Play(); break;
	default:
		break;
	}
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::stopStream
//
// Stops playback of the current audio or MIDI stream
// ----------------------------------------------------------------------------
void AudioEntryPanel::stopStream()
{
	switch (audio_type_)
	{
	case Sound:
		sound_->pause(); break;
	case Music:
		music_->pause(); break;
	case Mod:
		mod_->pause(); break;
	case MIDI:
		theMIDIPlayer->pause();	break;
	case Media:
		if (media_ctrl_) media_ctrl_->Pause(); break;
	default:
		break;
	}
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::resetStream
//
// Resets the current audio or MIDI stream to the beginning
// ----------------------------------------------------------------------------
void AudioEntryPanel::resetStream()
{

	switch (audio_type_)
	{
	case Sound:
		sound_->stop(); break;
	case Music:
		music_->stop(); break;
	case Mod:
		mod_->stop(); break;
	case MIDI:
		theMIDIPlayer->stop(); break;
	case Media:
		if (media_ctrl_) media_ctrl_->Stop(); break;
	default:
		break;
	}
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::updateInfo
//
// Used to update the info area, returns true if info is non-empty
// ----------------------------------------------------------------------------
bool AudioEntryPanel::updateInfo()
{
	txt_info_->Clear();
	string info = entry_->getTypeString() + "\n";
	MemChunk& mc = entry_->getMCData();
	switch (audio_type_)
	{
	case Sound:
	case Music:
	case Media:
		if (entry_->getType() == EntryType::fromId("snd_doom"))
		{
			size_t samplerate = READ_L16(mc, 2);
			size_t samples = READ_L16(mc, 4);
			info += S_FMT("%lu samples at %lu Hz", (unsigned long) samples, (unsigned long) samplerate);
		}
		else if (entry_->getType() == EntryType::fromId("snd_speaker"))
		{
			size_t samples = READ_L16(mc, 2);
			info += S_FMT("%lu samples", (unsigned long) samples);
		}
		else if (entry_->getType() == EntryType::fromId("snd_audiot"))
		{
			size_t samples = READ_L16(mc, 0);
			info += S_FMT("%lu samples", (unsigned long) samples);
		}
		else if (entry_->getType() == EntryType::fromId("snd_sun"))
			info += Audio::getSunInfo(mc);
		else if (entry_->getType() == EntryType::fromId("snd_voc"))
			info += Audio::getVocInfo(mc);
		else if (entry_->getType() == EntryType::fromId("snd_wav"))
			info += Audio::getWavInfo(mc);
		else if (entry_->getType() == EntryType::fromId("snd_mp3"))
			info += Audio::getID3Tag(mc);
		else if (entry_->getType() == EntryType::fromId("snd_ogg"))
			info += Audio::getOggComments(mc);
		else if (entry_->getType() == EntryType::fromId("snd_flac"))
			info += Audio::getFlacComments(mc);
		else if (entry_->getType() == EntryType::fromId("snd_aiff"))
			info += Audio::getAiffInfo(mc);
		break;
	case Mod:
		if (entry_->getType() == EntryType::fromId("mod_it"))
			info += Audio::getITComments(mc);
		else if (entry_->getType() == EntryType::fromId("mod_mod"))
			info += Audio::getModComments(mc);
		else if (entry_->getType() == EntryType::fromId("mod_s3m"))
			info += Audio::getS3MComments(mc);
		else if (entry_->getType() == EntryType::fromId("mod_xm"))
			info += Audio::getXMComments(mc);
		break;
	case MIDI:
		info += theMIDIPlayer->getInfo();
		if (entry_->getType() == EntryType::fromId("midi_rmid"))
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
	default:
		break;
	}
	txt_info_->SetValue(info);
	if (info.length())
		return true;
	return false;
}


// ----------------------------------------------------------------------------
//
// AudioEntryPanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// AudioEntryPanel::onBtnPlay
//
// Called when the play button is pressed
// ----------------------------------------------------------------------------
void AudioEntryPanel::onBtnPlay(wxCommandEvent& e)
{
	startStream();
	timer_seek_->Start(10);
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::onBtnPause
//
// Called when the pause button is pressed
// ----------------------------------------------------------------------------
void AudioEntryPanel::onBtnPause(wxCommandEvent& e)
{
	// Stop playing (no reset)
	stopStream();
	timer_seek_->Stop();
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::onBtnStop
//
// Called when the stop button is pressed
// ----------------------------------------------------------------------------
void AudioEntryPanel::onBtnStop(wxCommandEvent& e)
{
	// Stop playing
	stopStream();
	timer_seek_->Stop();

	// Reset
	resetStream();
	slider_seek_->SetValue(0);
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::onBtnPrev
//
// Called when the previous track button is pressed
// ----------------------------------------------------------------------------
void AudioEntryPanel::onBtnPrev(wxCommandEvent& e)
{
	if (subsong_ > 0)
		subsong_--;
	else subsong_ = num_tracks_ - 1;

	if (entry_->getType()->formatId() == "xmi")
	{
		MemChunk& mcdata = entry_->getMCData();
		MemChunk convdata;
		if (Conversions::zmusToMidi(mcdata, convdata, subsong_))
			openMidi(convdata, prevfile_);
	}
	//else if (entry->getType()->getFormat().StartsWith("gme"))
	//	theGMEPlayer->play(subsong);
	txt_track_->SetLabel(S_FMT("%d/%d", subsong_+1, num_tracks_));
	updateInfo();
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::onBtnNext
//
// Called when the next track button is pressed
// ----------------------------------------------------------------------------
void AudioEntryPanel::onBtnNext(wxCommandEvent& e)
{
	int newsong = (subsong_ + 1) % num_tracks_;
	if (entry_->getType()->formatId() == "xmi")
	{
		MemChunk& mcdata = entry_->getMCData();
		MemChunk convdata;
		if (Conversions::zmusToMidi(mcdata, convdata, newsong) && openMidi(convdata, prevfile_))
			subsong_ = newsong;
	}
	/*else if (entry->getType()->getFormat().StartsWith("gme"))
	{
		if (theGMEPlayer->play(newsong))
			subsong = newsong;
	}*/
	txt_track_->SetLabel(S_FMT("%d/%d", subsong_+1, num_tracks_));
	updateInfo();
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::onTimer
//
// Called when the playback timer ticks
// ----------------------------------------------------------------------------
void AudioEntryPanel::onTimer(wxTimerEvent& e)
{
	// Get current playback position
	int pos = 0;

	switch (audio_type_)
	{
	case Sound:
		pos = sound_->getPlayingOffset().asMilliseconds(); break;
	case Music:
		pos = music_->getPlayingOffset().asMilliseconds(); break;
	case Mod:
		pos = mod_->getPlayingOffset().asMilliseconds(); break;
	case MIDI:
		pos = theMIDIPlayer->getPosition(); break;
	case Media:
		if (media_ctrl_) pos = media_ctrl_->Tell(); break;
	default:
		break;
	}

	// Set slider
	slider_seek_->SetValue(pos);

	// Stop the timer if playback has reached the end
	if (pos >= slider_seek_->GetMax() ||
	        (audio_type_ == Sound && sound_->getStatus() == sf::Sound::Stopped) ||
	        (audio_type_ == Music && music_->getStatus() == sf::Sound::Stopped) ||
			(audio_type_ == Mod && mod_->getStatus() == sf::Sound::Stopped) ||
			(audio_type_ == Media && media_ctrl_ && media_ctrl_->GetState() == wxMEDIASTATE_STOPPED) ||
			(audio_type_ == MIDI && theMIDIPlayer && !theMIDIPlayer->isPlaying()))
	{
		timer_seek_->Stop();
		slider_seek_->SetValue(0);
	}
	
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::onSliderSeekChanged
//
// Called when the seek slider position is changed
// ----------------------------------------------------------------------------
void AudioEntryPanel::onSliderSeekChanged(wxCommandEvent& e)
{
	switch (audio_type_)
	{
	case Sound:
		sound_->setPlayingOffset(sf::milliseconds(slider_seek_->GetValue())); break;
	case Music:
		music_->setPlayingOffset(sf::milliseconds(slider_seek_->GetValue())); break;
	case Mod:
		mod_->setPlayingOffset(sf::milliseconds(slider_seek_->GetValue())); break;
	case MIDI:
		theMIDIPlayer->setPosition(slider_seek_->GetValue()); break;
	case Media:
		if (media_ctrl_) media_ctrl_->Seek(slider_seek_->GetValue()); break;
	default:
		break;
	}
}

// ----------------------------------------------------------------------------
// AudioEntryPanel::onSliderVolumeChanged
//
// Called when the volume slider position is changed
// ----------------------------------------------------------------------------
void AudioEntryPanel::onSliderVolumeChanged(wxCommandEvent& e)
{
	snd_volume = slider_volume_->GetValue();

	switch (audio_type_)
	{
	case Sound:
		sound_->setVolume(snd_volume); break;
	case Music:
		music_->setVolume(snd_volume); break;
	case MIDI:
		theMIDIPlayer->setVolume(snd_volume); break;
	case Media:
		if (media_ctrl_) media_ctrl_->SetVolume(snd_volume*0.01); break;
	case Mod:
		mod_->setVolume(snd_volume); break;
	default:
		break;
	}
}
