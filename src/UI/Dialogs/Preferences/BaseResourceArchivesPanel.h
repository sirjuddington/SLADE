#pragma once

#include "PrefsPanelBase.h"

class wxListBox;
class wxButton;

namespace slade
{
class FileLocationPanel;

class BaseResourceArchivesPanel : public PrefsPanelBase
{
public:
	BaseResourceArchivesPanel(wxWindow* parent);
	~BaseResourceArchivesPanel() override = default;

	int  selectedPathIndex() const;
	void autodetect() const;

	void     init() override;
	void     applyPreferences() override;
	wxString pageTitle() override { return "Base Resource Archive"; }

private:
	wxListBox*         list_base_archive_paths_ = nullptr;
	wxButton*          btn_add_                 = nullptr;
	wxButton*          btn_remove_              = nullptr;
	wxButton*          btn_detect_              = nullptr;
	FileLocationPanel* flp_zdoom_pk3_           = nullptr;

	void setupLayout();

	// Events
	void onBtnAdd(wxCommandEvent& e);
	void onBtnRemove(wxCommandEvent& e);
};
} // namespace slade
