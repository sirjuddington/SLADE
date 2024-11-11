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

		void applySettings() override;

	private:
		FileLocationPanel* flp_acc_path_          = nullptr;
		wxButton*          btn_incpath_add_       = nullptr;
		wxButton*          btn_incpath_remove_    = nullptr;
		wxListBox*         list_inc_paths_        = nullptr;
		wxCheckBox*        cb_always_show_output_ = nullptr;

		void setupLayout();
		void init() const;

		// Events
		void onBtnAddIncPath(wxCommandEvent& e);
		void onBtnRemoveIncPath(wxCommandEvent& e);
	};
} // namespace ui
} // namespace slade
