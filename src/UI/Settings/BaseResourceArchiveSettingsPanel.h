#pragma once

#include "SettingsPanel.h"

class wxListBox;
class wxButton;
namespace slade
{
class FileLocationPanel;
}

namespace slade::ui
{
class ArchiveListView;

class BaseResourceArchiveSettingsPanel : public SettingsPanel
{
public:
	BaseResourceArchiveSettingsPanel(wxWindow* parent);
	~BaseResourceArchiveSettingsPanel() override = default;

	int  selectedPathIndex() const;
	void autodetect() const;

	void   loadSettings() override;
	void   applySettings() override;
	string title() const override { return "Base Resource Archive"; }

	static bool popupDialog(wxWindow* parent = nullptr);

private:
	ArchiveListView*   list_base_archive_paths_ = nullptr;
	wxButton*          btn_add_                 = nullptr;
	wxButton*          btn_remove_              = nullptr;
	wxButton*          btn_detect_              = nullptr;
	FileLocationPanel* flp_zdoom_pk3_           = nullptr;

	void setupLayout();

	// Events
	void onBtnAdd(wxCommandEvent& e);
	void onBtnRemove(wxCommandEvent& e);
};
} // namespace slade::ui
