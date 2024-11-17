#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class GeneralSettingsPanel : public SettingsPanel
{
public:
	GeneralSettingsPanel(wxWindow* parent);
	~GeneralSettingsPanel() override = default;

	string title() const override { return "General Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	wxCheckBox* cb_show_start_page_           = nullptr;
	wxCheckBox* cb_confirm_exit_              = nullptr;
	wxCheckBox* cb_update_check_              = nullptr;
	wxCheckBox* cb_update_check_beta_         = nullptr;
	wxCheckBox* cb_close_archive_with_tab_    = nullptr;
	wxCheckBox* cb_auto_open_wads_root_       = nullptr;
	wxCheckBox* cb_backup_archives_           = nullptr;
	wxCheckBox* cb_archive_dir_ignore_hidden_ = nullptr;
};
} // namespace slade::ui
