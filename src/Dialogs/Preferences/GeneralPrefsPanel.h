#pragma once

#include "PrefsPanelBase.h"

class GeneralPrefsPanel : public PrefsPanelBase
{
public:
	GeneralPrefsPanel(wxWindow* parent);
	~GeneralPrefsPanel();

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox* cb_gl_np2_;
	wxCheckBox* cb_archive_load_;
	wxCheckBox* cb_archive_close_tab_;
	wxCheckBox* cb_wads_root_;
	wxCheckBox* cb_update_check_;
	wxCheckBox* cb_update_check_beta_;
	wxCheckBox* cb_confirm_exit_;
	wxCheckBox* cb_backup_archives_;
};
