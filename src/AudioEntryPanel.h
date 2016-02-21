
#ifndef __AUDIO_ENTRY_PANEL_H__
#define __AUDIO_ENTRY_PANEL_H__

#include "EntryPanel.h"
#include <wx/mediactrl.h>

#undef Status
#include <SFML/Audio.hpp>
#include "ModMusic.h"

class AudioEntryPanel : public EntryPanel
{
private:
	string	prevfile;
	int		audio_type;
	int		num_tracks;
	int		subsong;
	int		song_length;
	bool	opened;

	wxBitmapButton*	btn_play;
	wxBitmapButton*	btn_pause;
	wxBitmapButton*	btn_stop;
	wxBitmapButton*	btn_next;
	wxBitmapButton*	btn_prev;
	wxSlider*		slider_seek;
	wxSlider*		slider_volume;
	wxTimer*		timer_seek;
	wxMediaCtrl*	media_ctrl;
	wxStaticText*	txt_title;
	wxStaticText*	txt_track;
	wxTextCtrl*		txt_info;

	sf::SoundBuffer*	sound_buffer;
	sf::Sound			sound;
	sf::Music			music;
	ModMusic			mod;

	enum
	{
		AUTYPE_INVALID,
		AUTYPE_SOUND,
		AUTYPE_MUSIC,
		AUTYPE_MIDI,
		AUTYPE_MEDIA,
		AUTYPE_MOD,
		AUTYPE_EMU,
		AUTYPE_OPL,
	};

public:
	AudioEntryPanel(wxWindow* parent);
	~AudioEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	string	statusString();
	void	setAudioDuration(int duration);

	bool	open();
	bool	openAudio(MemChunk& audio, string filename);
	bool	openMidi(MemChunk& data, string filename);
	bool	openMod(MemChunk& data);
	bool	openMedia(string filename);
	bool	updateInfo();
	void	startStream();
	void	stopStream();
	void	resetStream();

	// Events
	void	onBtnPlay(wxCommandEvent& e);
	void	onBtnPause(wxCommandEvent& e);
	void	onBtnStop(wxCommandEvent& e);
	void	onBtnPrev(wxCommandEvent& e);
	void	onBtnNext(wxCommandEvent& e);
	void	onTimer(wxTimerEvent& e);
	void	onSliderSeekChanged(wxCommandEvent& e);
	void	onSliderVolumeChanged(wxCommandEvent& e);
};

#endif//__AUDIO_ENTRY_PANEL_H__
