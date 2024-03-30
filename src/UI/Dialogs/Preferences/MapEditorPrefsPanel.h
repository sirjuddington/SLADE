#pragma once

#include "PrefsPanelBase.h"

namespace slade
{
class NumberTextCtrl;

class MapEditorPrefsPanel : public PrefsPanelBase
{
public:
	MapEditorPrefsPanel(wxWindow* parent);
	~MapEditorPrefsPanel() override = default;

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox*     cb_scroll_smooth_             = nullptr;
	wxCheckBox*     cb_selection_clear_click_     = nullptr;
	wxCheckBox*     cb_selection_clear_move_      = nullptr;
	wxCheckBox*     cb_property_edit_dclick_      = nullptr;
	wxCheckBox*     cb_merge_undo_step_           = nullptr;
	wxCheckBox*     cb_props_auto_apply_          = nullptr;
	wxCheckBox*     cb_remove_invalid_lines_      = nullptr;
	wxCheckBox*     cb_merge_lines_vertex_delete_ = nullptr;
	wxCheckBox*     cb_split_auto_offset_         = nullptr;
	NumberTextCtrl* text_max_backups_             = nullptr;
};
} // namespace slade
