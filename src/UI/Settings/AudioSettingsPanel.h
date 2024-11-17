#pragma once

#include "SettingsPanel.h"

namespace slade
{
class FileLocationPanel;

namespace ui
{
	class AudioSettingsPanel : public SettingsPanel
	{
	public:
		AudioSettingsPanel(wxWindow* parent);
		~AudioSettingsPanel() override = default;

		string title() const override { return "Audio Settings"; }

		void loadSettings() override;
		void applySettings() override;

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
} // namespace ui
} // namespace slade
