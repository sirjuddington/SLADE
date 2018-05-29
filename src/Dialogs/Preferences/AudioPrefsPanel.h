#pragma once

#include "PrefsPanelBase.h"

class FileLocationPanel;

class AudioPrefsPanel : public PrefsPanelBase
{
public:
	AudioPrefsPanel(wxWindow* parent);
	~AudioPrefsPanel();

	void	init() override;
	void	applyPreferences() override;

private:
	wxCheckBox*			cb_snd_autoplay_;
	wxCheckBox*			cb_dmx_padding_;
	wxRadioButton*		rb_fluidsynth_;
	wxRadioButton*		rb_timidity_;
	wxTextCtrl*			text_timidity_options_;
	wxButton*			btn_reset_player_;
	FileLocationPanel*	flp_soundfont_;
	FileLocationPanel*	flp_timidity_;

	void	setupLayout();
	void	updateControls() const;

	// Events
	void	onBtnResetPlayer(wxCommandEvent& e);
};
