#pragma once

#include "PrefsPanelBase.h"

class wxListBox;

namespace slade
{
class FileLocationPanel;

class DECOHackPrefsPanel : public PrefsPanelBase
{
public:
	DECOHackPrefsPanel(wxWindow* parent);
	~DECOHackPrefsPanel() = default;

	void init() override;
	void applyPreferences() override;

	string pageTitle() override { return "DECOHack Compiler Settings"; }

private:
	FileLocationPanel* flp_decohack_path_     = nullptr;
	FileLocationPanel* flp_java_path_         = nullptr;
	wxCheckBox*        cb_always_show_output_ = nullptr;

	void setupLayout();
};
} // namespace slade
