#pragma once

#include "PrefsPanelBase.h"

class NumberTextCtrl;
class MapEditorPrefsPanel : public PrefsPanelBase
{
public:
	MapEditorPrefsPanel(wxWindow* parent);
	~MapEditorPrefsPanel();

	void init() override;
	void applyPreferences() override;

private:
	wxCheckBox*     cb_scroll_smooth_;
	wxCheckBox*     cb_selection_clear_click_;
	wxCheckBox*     cb_selection_clear_move_;
	wxCheckBox*     cb_property_edit_dclick_;
	wxCheckBox*     cb_merge_undo_step_;
	wxCheckBox*     cb_props_auto_apply_;
	wxCheckBox*     cb_remove_invalid_lines_;
	wxCheckBox*     cb_merge_lines_vertex_delete_;
	wxCheckBox*     cb_split_auto_offset_;
	NumberTextCtrl* text_max_backups_;
};
