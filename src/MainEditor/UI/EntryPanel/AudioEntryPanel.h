#pragma once

#include "EntryPanel.h"

class ModMusic;
namespace sf
{
class SoundBuffer;
class Sound;
class Music;
} // namespace sf
class wxMediaCtrl;

class AudioEntryPanel : public EntryPanel
{
public:
	AudioEntryPanel(wxWindow* parent);
	~AudioEntryPanel();

	bool   loadEntry(ArchiveEntry* entry) override;
	bool   saveEntry() override;
	string statusString() override;
	void   setAudioDuration(int duration);

private:
	enum AudioType
	{
		Invalid,
		Sound,
		Music,
		MIDI,
		Media,
		Mod,
		Emu,
		OPL,
	};

	string    prevfile_;
	AudioType audio_type_  = Invalid;
	int       num_tracks_  = 1;
	int       subsong_     = 0;
	int       song_length_ = 0;
	bool      opened_      = false;

	wxBitmapButton* btn_play_      = nullptr;
	wxBitmapButton* btn_pause_     = nullptr;
	wxBitmapButton* btn_stop_      = nullptr;
	wxBitmapButton* btn_next_      = nullptr;
	wxBitmapButton* btn_prev_      = nullptr;
	wxSlider*       slider_seek_   = nullptr;
	wxSlider*       slider_volume_ = nullptr;
	wxTimer*        timer_seek_    = nullptr;
	wxMediaCtrl*    media_ctrl_    = nullptr;
	wxStaticText*   txt_title_     = nullptr;
	wxStaticText*   txt_track_     = nullptr;
	wxTextCtrl*     txt_info_      = nullptr;

	std::unique_ptr<sf::SoundBuffer> sound_buffer_;
	std::unique_ptr<sf::Sound>       sound_;
	std::unique_ptr<sf::Music>       music_;
	std::unique_ptr<ModMusic>        mod_;

	bool open();
	bool openAudio(MemChunk& audio, string filename);
	bool openMidi(MemChunk& data, string filename);
	bool openMod(MemChunk& data);
	bool openMedia(string filename);
	bool updateInfo();
	void startStream();
	void stopStream();
	void resetStream();

	// Events
	void onBtnPlay(wxCommandEvent& e);
	void onBtnPause(wxCommandEvent& e);
	void onBtnStop(wxCommandEvent& e);
	void onBtnPrev(wxCommandEvent& e);
	void onBtnNext(wxCommandEvent& e);
	void onTimer(wxTimerEvent& e);
	void onSliderSeekChanged(wxCommandEvent& e);
	void onSliderVolumeChanged(wxCommandEvent& e);
};
