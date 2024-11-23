#pragma once

#include "SettingsPanel.h"

namespace slade
{
class NumberTextCtrl;
namespace ui
{
	class NodeBuildersSettingsPanel;
	class Map3DSettingsPanel;
} // namespace ui
} // namespace slade

namespace slade::ui
{
class MapGeneralSettingsPanel : public SettingsPanel
{
public:
	MapGeneralSettingsPanel(wxWindow* parent);
	~MapGeneralSettingsPanel() override = default;

	string title() const override { return "Map Editor Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	wxCheckBox*     cb_selection_clear_click_     = nullptr;
	wxCheckBox*     cb_selection_clear_move_      = nullptr;
	wxCheckBox*     cb_property_edit_dclick_      = nullptr;
	wxCheckBox*     cb_merge_undo_step_           = nullptr;
	wxCheckBox*     cb_props_auto_apply_          = nullptr;
	wxCheckBox*     cb_remove_invalid_lines_      = nullptr;
	wxCheckBox*     cb_merge_lines_vertex_delete_ = nullptr;
	wxCheckBox*     cb_split_auto_offset_         = nullptr;
	NumberTextCtrl* text_max_backups_             = nullptr;
	wxCheckBox*     cb_save_archive_with_map_     = nullptr;

	NodeBuildersSettingsPanel* nodebuilders_panel_ = nullptr;
	Map3DSettingsPanel*        map3d_panel_        = nullptr;

	wxPanel* createGeneralPanel(wxWindow* parent);
};
} // namespace slade::ui
