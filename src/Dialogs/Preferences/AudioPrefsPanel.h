#pragma once

#include "PrefsPanelBase.h"

class FileLocationPanel;

class AudioPrefsPanel : public PrefsPanelBase
{
public:
	AudioPrefsPanel(wxWindow* parent);
	~AudioPrefsPanel() = default;

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox*        cb_snd_autoplay_       = nullptr;
	wxCheckBox*        cb_dmx_padding_        = nullptr;
	wxRadioButton*     rb_fluidsynth_         = nullptr;
	wxRadioButton*     rb_timidity_           = nullptr;
	wxTextCtrl*        text_timidity_options_ = nullptr;
	wxButton*          btn_reset_player_      = nullptr;
	FileLocationPanel* flp_soundfont_         = nullptr;
	FileLocationPanel* flp_timidity_          = nullptr;

	void setupLayout();
	void updateControls() const;
};
