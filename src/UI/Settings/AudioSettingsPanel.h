#pragma once

#include "SettingsPanel.h"

namespace slade
{
class FileLocationPanel;

namespace ui
{
	class SettingsTable;

	class AudioSettingsPanel : public SettingsPanel
	{
	public:
		AudioSettingsPanel(wxWindow* parent);
		~AudioSettingsPanel() override = default;

		string title() const override { return "Audio Settings"; }

		void loadSettings() override;
		void applySettings() override;

	private:
		wxButton*          btn_reset_player_     = nullptr;
		FileLocationPanel* flp_soundfont_        = nullptr;
		FileLocationPanel* flp_timidity_         = nullptr;
		wxChoice*          choice_fs_samplerate_ = nullptr;
		SettingsTable*     settings_panel_       = nullptr;
	};
} // namespace ui
} // namespace slade
