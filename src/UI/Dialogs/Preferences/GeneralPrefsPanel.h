#pragma once

#include "PrefsPanelBase.h"

namespace slade
{
class GeneralPrefsPanel : public PrefsPanelBase
{
public:
	GeneralPrefsPanel(wxWindow* parent);
	~GeneralPrefsPanel() override = default;

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox* cb_archive_close_tab_         = nullptr;
	wxCheckBox* cb_wads_root_                 = nullptr;
	wxCheckBox* cb_update_check_              = nullptr;
	wxCheckBox* cb_update_check_beta_         = nullptr;
	wxCheckBox* cb_confirm_exit_              = nullptr;
	wxCheckBox* cb_backup_archives_           = nullptr;
	wxCheckBox* cb_archive_dir_ignore_hidden_ = nullptr;
};
} // namespace slade
