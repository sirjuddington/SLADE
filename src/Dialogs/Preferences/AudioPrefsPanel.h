
#ifndef __AUDIO_PREFS_PANEL_H__
#define __AUDIO_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class AudioPrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*	cb_snd_autoplay;
	wxCheckBox*	cb_dmx_padding;
	wxCheckBox*	cb_snd_timidity;
	wxTextCtrl*	text_soundfont_path;
	wxTextCtrl* text_timidity_path;
	wxTextCtrl*	text_timidity_options;
	wxButton*	btn_browse_soundfont;
	wxButton*	btn_browse_timidityexe;
	wxButton*	btn_reset_player;

public:
	AudioPrefsPanel(wxWindow* parent);
	~AudioPrefsPanel();

	void	init();
	void	applyPreferences();

	// Events
	void	onBtnBrowseSoundfont(wxCommandEvent& e);
	void	onBtnResetPlayer(wxCommandEvent& e);
	void	onBtnBrowseTimidityExe(wxCommandEvent &e);
};

#endif//__AUDIO_PREFS_PANEL_H__
