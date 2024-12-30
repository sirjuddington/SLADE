
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "AudioEntryPanel.h"
#include "App.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "Audio/AudioTags.h"
#include "Audio/MIDIPlayer.h"
#include "Audio/ModMusic.h"
#include "Audio/Mp3Music.h"
#include "Audio/Music.h"
#include "MainEditor/Conversions.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Layout.h"
#include "UI/SToolBar/SToolBar.h"
#include "Utility/StringUtils.h"

using namespace slade;

#undef Status


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, snd_volume, 100, CVar::Flag::Save)
CVAR(Bool, snd_autoplay, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// AudioEntryPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// AudioEntryPanel class constructor
// -----------------------------------------------------------------------------
AudioEntryPanel::AudioEntryPanel(wxWindow* parent) :
	EntryPanel(parent, "audio"),
	timer_seek_{ new wxTimer(this) },
#if (SFML_VERSION_MAJOR > 2)
	sound_{ new sf::Sound(*sound_buffer_) },
#else
	sound_{ new sf::Sound() },
#endif
	music_{ new audio::Music() },
	mod_{ new audio::ModMusic() },
	mp3_{ new audio::Mp3Music() }
{
	auto lh = ui::LayoutHelper(this);

	// Setup sizer
	auto sizer_gb = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer_main_->AddStretchSpacer();
	sizer_main_->Add(sizer_gb, wxSizerFlags().Center());
	sizer_main_->AddStretchSpacer();

	// Add seekbar
	slider_seek_ = new wxSlider(this, -1, 0, 0, 100);
	sizer_gb->Add(slider_seek_, wxGBPosition(0, 0), wxGBSpan(1, 9), wxEXPAND);

	// Add play controls
	btn_play_ = new SIconButton(this, "play", "", 24);
	sizer_gb->Add(btn_play_, wxGBPosition(1, 0));
	btn_pause_ = new SIconButton(this, "pause", "", 24);
	sizer_gb->Add(btn_pause_, wxGBPosition(1, 1));
	btn_stop_ = new SIconButton(this, "stop", "", 24);
	sizer_gb->Add(btn_stop_, wxGBPosition(1, 2));
	btn_prev_ = new SIconButton(this, "prev", "", 24);
	sizer_gb->Add(btn_prev_, wxGBPosition(1, 3));
	btn_next_ = new SIconButton(this, "next", "", 24);
	sizer_gb->Add(btn_next_, wxGBPosition(1, 4));

	// Separator
	sizer_gb->Add(new wxStaticLine(this), { 2, 0 }, { 1, 9 }, wxEXPAND | wxTOP | wxBOTTOM, lh.pad());

	// Add title
	txt_title_ = new wxStaticText(this, -1, wxEmptyString);
	sizer_gb->Add(txt_title_, wxGBPosition(3, 0), wxGBSpan(1, 9));

	// Add info
	txt_info_ = new wxTextCtrl(
		this, -1, wxEmptyString, wxDefaultPosition, lh.size(-1, 200), wxTE_MULTILINE | wxTE_READONLY | wxTE_BESTWRAP);
	sizer_gb->Add(txt_info_, wxGBPosition(4, 0), wxGBSpan(1, 9), wxEXPAND | wxHORIZONTAL);

	// Add track number
	txt_track_ = new wxStaticText(this, -1, "1/1");
	sizer_gb->Add(txt_track_, wxGBPosition(1, 5), wxDefaultSpan, wxALIGN_CENTER);

	// Separator
	sizer_gb->Add(
		new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL),
		wxGBPosition(1, 6),
		wxDefaultSpan,
		wxEXPAND);

	// Add volume slider
	sizer_gb->Add(new wxStaticText(this, -1, "Volume:"), wxGBPosition(1, 7), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	slider_volume_ = new wxSlider(this, -1, 0, 0, 100, wxDefaultPosition, lh.size(128, -1));
	slider_volume_->SetValue(snd_volume);
	sizer_gb->Add(slider_volume_, wxGBPosition(1, 8), { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	// Set volume
	sound_->setVolume(snd_volume);
	music_->setVolume(snd_volume);
	audio::midiPlayer().setVolume(snd_volume);
	mod_->setVolume(snd_volume);
	mp3_->setVolume(snd_volume);

	// Disable general entrypanel buttons
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

	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// AudioEntryPanel class destructor
// -----------------------------------------------------------------------------
AudioEntryPanel::~AudioEntryPanel()
{
	// Stop the timer to avoid crashes
	timer_seek_->Stop();
	resetStream();
}

// -----------------------------------------------------------------------------
// Loads an entry into the audio entry panel
// -----------------------------------------------------------------------------
bool AudioEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Are we reopening the same entry? For example having looked at
	// a text file or image or any other non-audio entry, then
	// going back to the original audio entry? Then there is no need to
	// abort the current song to restart it.
	if (entry_.lock().get() == entry)
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
	if (!open(entry))
		return false;

	// Autoplay if option is on
	if (snd_autoplay)
	{
		startStream();
		timer_seek_->Start(10);
	}

	Refresh();
	return true;
}

// -----------------------------------------------------------------------------
// Returns a string with extended editing/entry info for the status bar
// -----------------------------------------------------------------------------
wxString AudioEntryPanel::statusString()
{
	int hours, minutes, seconds, milliseconds = song_length_ % 1000;
	seconds      = (song_length_ / 1000) % 60;
	minutes      = (song_length_ / 60000) % 60;
	hours        = (song_length_ / 3600000);
	wxString ret = wxEmptyString;
	if (hours)
		ret = wxString::Format("%d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
	else if (minutes)
		ret = wxString::Format("%d:%02d.%03d", minutes, seconds, milliseconds);
	else
		ret = wxString::Format("%d.%03d", seconds, milliseconds);

	return ret;
}

// -----------------------------------------------------------------------------
// Sets the seek slider control duration
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Opens the current entry and performs the appropriate conversions
// -----------------------------------------------------------------------------
bool AudioEntryPanel::open(ArchiveEntry* entry)
{
	// Check if already opened
	if (opened_)
		return true;

	// Stop if sound currently playing
	music_->allowSeek(false); // Needed to avoid a crash in SFML with ogg files
	resetStream();

	subsong_    = 0;
	num_tracks_ = 1;

	// Get entry data
	auto& mcdata = entry->data();

	// Setup temp filename
	wxFileName path(app::path(entry->name(), app::Dir::Temp));
	// Add extension if missing
	if (path.GetExt().IsEmpty())
		path.SetExt(entry->type()->extension());

	// Convert if necessary, then write to file
	data_.clear();
	if (entry->type()->formatId() == "snd_doom" || // Doom Sound -> WAV
		entry->type()->formatId() == "snd_doom_mac")
		conversion::doomSndToWav(mcdata, data_);
	else if (entry->type()->formatId() == "snd_speaker") // Doom PC Speaker Sound -> WAV
		conversion::spkSndToWav(mcdata, data_);
	else if (entry->type()->formatId() == "snd_audiot") // AudioT PC Speaker Sound -> WAV
		conversion::spkSndToWav(mcdata, data_, true);
	else if (entry->type()->formatId() == "snd_wolf") // Wolfenstein 3D Sound -> WAV
		conversion::wolfSndToWav(mcdata, data_);
	else if (entry->type()->formatId() == "snd_voc") // Creative Voice File -> WAV
		conversion::vocToWav(mcdata, data_);
	else if (entry->type()->formatId() == "snd_jaguar") // Jaguar Doom Sound -> WAV
		conversion::jagSndToWav(mcdata, data_);
	else if (entry->type()->formatId() == "snd_bloodsfx") // Blood Sound -> WAV
		conversion::bloodToWav(entry, data_);
	else if (entry->type()->formatId() == "midi_mus") // MUS -> MIDI
	{
		conversion::musToMidi(mcdata, data_);
		path.SetExt("mid");
	}
	else if (
		entry->type()->formatId() == "midi_xmi" || // HMI/HMP/XMI -> MIDI
		entry->type()->formatId() == "midi_hmi" || entry->type()->formatId() == "midi_hmp")
	{
		conversion::zmusToMidi(mcdata, data_, 0, &num_tracks_);
		path.SetExt("mid");
	}
	else if (entry->type()->formatId() == "midi_gmid") // GMID -> MIDI
	{
		conversion::gmidToMidi(mcdata, data_);
		path.SetExt("mid");
	}
	else
		data_.importMem(mcdata.data(), mcdata.size());

	// MIDI format
	if (strutil::startsWith(entry->type()->formatId(), "midi_"))
		openMidi(data_, path.GetFullPath());

	// MOD format
	else if (strutil::startsWith(entry->type()->formatId(), "mod_"))
		openMod(data_);

	// Mp3 format
	else if (strutil::startsWith(entry->type()->formatId(), "snd_mp3"))
		openMp3(data_);

	// Other format
	else
		openAudio(data_, path.GetFullPath());

	// Keep filename so we can delete it later
	prevfile_ = path.GetFullPath();

	txt_title_->SetLabel(entry->path(true));
	txt_track_->SetLabel(wxString::Format("%d/%d", subsong_ + 1, num_tracks_));
	updateInfo(*entry);

	// Disable prev/next track buttons if only one track is available
	if (num_tracks_ < 2)
	{
		btn_prev_->Disable();
		btn_next_->Disable();
	}

	opened_ = true;
	music_->allowSeek(true);
	return true;
}

// -----------------------------------------------------------------------------
// Opens an audio file for playback (SFML 2.x+)
// -----------------------------------------------------------------------------
bool AudioEntryPanel::openAudio(MemChunk& audio, const wxString& filename)
{
	// Stop if sound currently playing
	resetStream();

	// (Re)create sound buffer
	sound_buffer_ = std::make_unique<sf::SoundBuffer>();
	audio_type_   = Invalid;

	// Load into buffer
	if (sound_buffer_->loadFromMemory((const char*)audio.data(), audio.size()))
	{
		log::info(3, "opened as sound");
		// Bind to sound
		sound_->setBuffer(*sound_buffer_);
		audio_type_ = Sound;

		// Enable play controls
		setAudioDuration(sound_buffer_->getDuration().asMilliseconds());
		btn_play_->Enable();
		btn_pause_->Enable();
		btn_stop_->Enable();

		return true;
	}
	else if (music_->openFromMemory((const char*)audio.data(), audio.size()))
	{
		log::info(3, "opened as music");
		// Couldn't open the audio as a sf::SoundBuffer, try sf::Music instead
		audio_type_ = Music;

		// Enable play controls
		setAudioDuration(music_->getDuration().asMilliseconds());
		btn_play_->Enable();
		btn_stop_->Enable();

		return true;
	}

	// Unable to open audio, disable play controls
	setAudioDuration(0);
	btn_play_->Enable(false);
	btn_pause_->Enable(false);
	btn_stop_->Enable(false);

	return false;
}

// -----------------------------------------------------------------------------
// Opens a MIDI file for playback
// -----------------------------------------------------------------------------
bool AudioEntryPanel::openMidi(MemChunk& data, const wxString& filename)
{
	audio_type_ = MIDI;

	// Enable volume control
	slider_volume_->Enable(true);

	// Attempt to open midi
	if (audio::midiPlayer().isReady())
	{
		if (audio::midiPlayer().openData(data))
		{
			// Enable play controls
			btn_play_->Enable();
			btn_pause_->Enable();
			btn_stop_->Enable();

			// Setup seekbar
			setAudioDuration(audio::midiLength(data));

			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Opens a Module file for playback
// -----------------------------------------------------------------------------
bool AudioEntryPanel::openMod(MemChunk& data)
{
	// Attempt to load the mod
	if (mod_->loadFromMemory(data.data(), data.size()))
	{
		audio_type_ = Mod;

		// Enable playback controls
		slider_volume_->Enable();
		btn_play_->Enable();
		btn_pause_->Enable();
		btn_stop_->Enable();
		setAudioDuration(mod_->duration().asMilliseconds());

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
}

// -----------------------------------------------------------------------------
// Opens an mp3 file for playback
// -----------------------------------------------------------------------------
bool AudioEntryPanel::openMp3(MemChunk& data)
{
	// Attempt to load the mp3
	if (mp3_->loadFromMemory(data.data(), data.size()))
	{
		audio_type_ = Mp3;

		// Enable playback controls
		slider_volume_->Enable();
		btn_play_->Enable();
		btn_pause_->Enable();
		btn_stop_->Enable();
		setAudioDuration(mp3_->duration().asMilliseconds());

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
}

// -----------------------------------------------------------------------------
// Begins playback of the current audio or MIDI stream
// -----------------------------------------------------------------------------
void AudioEntryPanel::startStream()
{
	if (!opened_ && entry_.lock())
		open(entry_.lock().get());

	switch (audio_type_)
	{
	case Sound: sound_->play(); break;
	case Music: music_->play(); break;
	case Mod:   mod_->play(); break;
	case MIDI:  audio::midiPlayer().play(); break;
	case Mp3:   mp3_->play(); break;
	default:    break;
	}
}

// -----------------------------------------------------------------------------
// Stops playback of the current audio or MIDI stream
// -----------------------------------------------------------------------------
void AudioEntryPanel::stopStream() const
{
	switch (audio_type_)
	{
	case Sound: sound_->pause(); break;
	case Music: music_->pause(); break;
	case Mod:   mod_->pause(); break;
	case MIDI:  audio::midiPlayer().pause(); break;
	case Mp3:   mp3_->pause(); break;
	default:    break;
	}
}

// -----------------------------------------------------------------------------
// Resets the current audio or MIDI stream to the beginning
// -----------------------------------------------------------------------------
void AudioEntryPanel::resetStream() const
{
	switch (audio_type_)
	{
	case Sound: sound_->stop(); break;
	case Music: music_->stop(); break;
	case Mod:   mod_->stop(); break;
	case MIDI:  audio::midiPlayer().stop(); break;
	case Mp3:   mp3_->stop(); break;
	default:    break;
	}
}

// -----------------------------------------------------------------------------
// Used to update the info area, returns true if info is non-empty
// -----------------------------------------------------------------------------
bool AudioEntryPanel::updateInfo(ArchiveEntry& entry) const
{
	txt_info_->Clear();

	wxString info = entry.typeString() + "\n";
	auto&    mc   = entry.data();
	switch (audio_type_)
	{
	case Sound:
	case Music:
	case Mp3:
		if (entry.type() == EntryType::fromId("snd_doom"))
		{
			size_t samplerate = mc.readL16(2);
			size_t samples    = mc.readL16(4);
			info += wxString::Format(
				"%lu samples at %lu Hz", static_cast<unsigned long>(samples), static_cast<unsigned long>(samplerate));
		}
		else if (entry.type() == EntryType::fromId("snd_speaker"))
		{
			size_t samples = mc.readL16(2);
			info += wxString::Format("%lu samples", static_cast<unsigned long>(samples));
		}
		else if (entry.type() == EntryType::fromId("snd_audiot"))
		{
			size_t samples = mc.readL16(0);
			info += wxString::Format("%lu samples", static_cast<unsigned long>(samples));
		}
		else if (entry.type() == EntryType::fromId("snd_sun"))
			info += audio::getSunInfo(mc);
		else if (entry.type() == EntryType::fromId("snd_voc"))
			info += audio::getVocInfo(mc);
		else if (entry.type() == EntryType::fromId("snd_wav"))
			info += audio::getWavInfo(mc);
		else if (entry.type() == EntryType::fromId("snd_mp3"))
			info += audio::getID3Tag(mc);
		else if (entry.type() == EntryType::fromId("snd_ogg"))
			info += audio::getOggComments(mc);
		else if (entry.type() == EntryType::fromId("snd_flac"))
			info += audio::getFlacComments(mc);
		else if (entry.type() == EntryType::fromId("snd_aiff"))
			info += audio::getAiffInfo(mc);
		break;
	case Mod:
		if (entry.type() == EntryType::fromId("mod_it"))
			info += audio::getITComments(mc);
		else if (entry.type() == EntryType::fromId("mod_mod"))
			info += audio::getModComments(mc);
		else if (entry.type() == EntryType::fromId("mod_s3m"))
			info += audio::getS3MComments(mc);
		else if (entry.type() == EntryType::fromId("mod_xm"))
			info += audio::getXMComments(mc);
		break;
	case MIDI:
		info += audio::midiInfo(mc);
		if (entry.type() == EntryType::fromId("midi_rmid"))
			info += audio::getRmidInfo(mc);
		break;
	default: break;
	}
	txt_info_->SetValue(info);
	if (info.length())
		return true;
	return false;
}


// -----------------------------------------------------------------------------
//
// AudioEntryPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the play button is pressed
// -----------------------------------------------------------------------------
void AudioEntryPanel::onBtnPlay(wxCommandEvent& e)
{
	startStream();
	timer_seek_->Start(10);
}

// -----------------------------------------------------------------------------
// Called when the pause button is pressed
// -----------------------------------------------------------------------------
void AudioEntryPanel::onBtnPause(wxCommandEvent& e)
{
	// Stop playing (no reset)
	stopStream();
	timer_seek_->Stop();
}

// -----------------------------------------------------------------------------
// Called when the stop button is pressed
// -----------------------------------------------------------------------------
void AudioEntryPanel::onBtnStop(wxCommandEvent& e)
{
	// Stop playing
	stopStream();
	timer_seek_->Stop();

	// Reset
	resetStream();
	slider_seek_->SetValue(0);
}

// -----------------------------------------------------------------------------
// Called when the previous track button is pressed
// -----------------------------------------------------------------------------
void AudioEntryPanel::onBtnPrev(wxCommandEvent& e)
{
	if (subsong_ > 0)
		subsong_--;
	else
		subsong_ = num_tracks_ - 1;

	if (auto entry = entry_.lock(); entry && entry->type()->formatId() == "xmi")
	{
		MemChunk convdata;
		if (conversion::zmusToMidi(entry->data(), convdata, subsong_))
			openMidi(convdata, prevfile_);

		updateInfo(*entry);
	}

	txt_track_->SetLabel(wxString::Format("%d/%d", subsong_ + 1, num_tracks_));
}

// -----------------------------------------------------------------------------
// Called when the next track button is pressed
// -----------------------------------------------------------------------------
void AudioEntryPanel::onBtnNext(wxCommandEvent& e)
{
	int newsong = (subsong_ + 1) % num_tracks_;

	if (auto entry = entry_.lock(); entry && entry->type()->formatId() == "xmi")
	{
		MemChunk convdata;
		if (conversion::zmusToMidi(entry->data(), convdata, newsong) && openMidi(convdata, prevfile_))
			subsong_ = newsong;

		updateInfo(*entry);
	}

	txt_track_->SetLabel(wxString::Format("%d/%d", subsong_ + 1, num_tracks_));
}

// -----------------------------------------------------------------------------
// Called when the playback timer ticks
// -----------------------------------------------------------------------------
void AudioEntryPanel::onTimer(wxTimerEvent& e)
{
	// Get current playback position
	int pos = 0;

	switch (audio_type_)
	{
	case Sound: pos = sound_->getPlayingOffset().asMilliseconds(); break;
	case Music: pos = music_->getPlayingOffset().asMilliseconds(); break;
	case Mod:   pos = mod_->getPlayingOffset().asMilliseconds(); break;
	case MIDI:  pos = audio::midiPlayer().position(); break;
	case Mp3:   pos = mp3_->getPlayingOffset().asMilliseconds(); break;
	default:    break;
	}

	// Set slider
	slider_seek_->SetValue(pos);

// Stop the timer if playback has reached the end
#if (SFML_VERSION_MAJOR > 2)
	if (pos >= slider_seek_->GetMax() || (audio_type_ == Sound && sound_->getStatus() == sf::Sound::Status::Stopped)
		|| (audio_type_ == Music && music_->getStatus() == sf::Sound::Status::Stopped)
		|| (audio_type_ == Mod && mod_->getStatus() == sf::Sound::Status::Stopped)
		|| (audio_type_ == Mp3 && mp3_->getStatus() == sf::Sound::Status::Stopped)
		|| (audio_type_ == MIDI && !audio::midiPlayer().isPlaying()))
#else
	if (pos >= slider_seek_->GetMax() || (audio_type_ == Sound && sound_->getStatus() == sf::Sound::Stopped)
		|| (audio_type_ == Music && music_->getStatus() == sf::Sound::Stopped)
		|| (audio_type_ == Mod && mod_->getStatus() == sf::Sound::Stopped)
		|| (audio_type_ == Mp3 && mp3_->getStatus() == sf::Sound::Stopped)
		|| (audio_type_ == MIDI && !audio::midiPlayer().isPlaying()))
#endif
	{
		timer_seek_->Stop();
		slider_seek_->SetValue(0);
	}
}

// -----------------------------------------------------------------------------
// Called when the seek slider position is changed
// -----------------------------------------------------------------------------
void AudioEntryPanel::onSliderSeekChanged(wxCommandEvent& e)
{
	switch (audio_type_)
	{
	case Sound: sound_->setPlayingOffset(sf::milliseconds(slider_seek_->GetValue())); break;
	case Music: music_->setPlayingOffset(sf::milliseconds(slider_seek_->GetValue())); break;
	case Mod:   mod_->setPlayingOffset(sf::milliseconds(slider_seek_->GetValue())); break;
	case MIDI:  audio::midiPlayer().setPosition(slider_seek_->GetValue()); break;
	case Mp3:   mp3_->setPlayingOffset(sf::milliseconds(slider_seek_->GetValue())); break;
	default:    break;
	}
}

// -----------------------------------------------------------------------------
// Called when the volume slider position is changed
// -----------------------------------------------------------------------------
void AudioEntryPanel::onSliderVolumeChanged(wxCommandEvent& e)
{
	snd_volume = slider_volume_->GetValue();

	switch (audio_type_)
	{
	case Sound: sound_->setVolume(snd_volume); break;
	case Music: music_->setVolume(snd_volume); break;
	case MIDI:  audio::midiPlayer().setVolume(snd_volume); break;
	case Mp3:   mp3_->setVolume(snd_volume); break;
	case Mod:   mod_->setVolume(snd_volume); break;
	default:    break;
	}
}
