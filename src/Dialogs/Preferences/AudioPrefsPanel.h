
#ifndef __AUDIO_PREFS_PANEL_H__
#define __AUDIO_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class AudioPrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*	cb_snd_autoplay;
	wxTextCtrl*	text_timidity_path;
	wxTextCtrl*	text_timidity_options;
	wxButton*	btn_browse_timidityexe;
	wxButton*	btn_reset_player;

	wxCheckBox*	cb_dmx_padding;
#ifndef NO_FLUIDSYNTH
	wxCheckBox*	cb_snd_timidity;
	wxTextCtrl*	text_soundfont_path;
	wxButton*	btn_browse_soundfont;
#endif

public:
	AudioPrefsPanel(wxWindow* parent);
	~AudioPrefsPanel();

	void	init();
	void	applyPreferences();

	// Events
#ifndef NO_FLUIDSYNTH
	void	onBtnBrowseSoundfont(wxCommandEvent& e);
#endif
	void	onBtnResetPlayer(wxCommandEvent& e);
	void	onBtnBrowseTimidityExe(wxCommandEvent &e);
};

#endif//__AUDIO_PREFS_PANEL_H__
