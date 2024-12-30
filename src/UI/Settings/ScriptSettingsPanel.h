#pragma once

#include "SettingsPanel.h"

class wxListBox;

namespace slade
{
class FileLocationPanel;

namespace ui
{
	class ScriptSettingsPanel : public SettingsPanel
	{
	public:
		ScriptSettingsPanel(wxWindow* parent);
		~ScriptSettingsPanel() override = default;

		string title() const override { return "Scripting && Compiler Settings"; }
		string icon() const override { return "script"; }

		void loadSettings() override;
		void applySettings() override;

	private:
		// ACS
		FileLocationPanel* flp_acc_path_          = nullptr;
		wxButton*          btn_incpath_add_       = nullptr;
		wxButton*          btn_incpath_remove_    = nullptr;
		wxListBox*         list_inc_paths_        = nullptr;
		wxCheckBox*        cb_always_show_output_ = nullptr;

		// DECOHack
		FileLocationPanel* flp_decohack_path_        = nullptr;
		FileLocationPanel* flp_java_path_            = nullptr;
		wxCheckBox*        cb_always_show_output_dh_ = nullptr;

		wxPanel* createACSPanel(wxWindow* parent);
		wxPanel* createDECOHackPanel(wxWindow* parent);

		// Events
		void onBtnAddIncPath(wxCommandEvent& e);
		void onBtnRemoveIncPath(wxCommandEvent& e);
	};
} // namespace ui
} // namespace slade
