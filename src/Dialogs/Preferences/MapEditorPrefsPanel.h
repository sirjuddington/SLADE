
#ifndef __MAP_EDITOR_PREFS_PANEL_H__
#define __MAP_EDITOR_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class NumberTextCtrl;
class MapEditorPrefsPanel : public PrefsPanelBase
{
private:
	wxCheckBox*		cb_scroll_smooth;
	wxCheckBox*		cb_selection_clear_click;
	wxCheckBox*		cb_merge_undo_step;
	wxCheckBox*		cb_props_auto_apply;
	wxCheckBox*		cb_remove_invalid_lines;
	NumberTextCtrl*	text_max_backups;

public:
	MapEditorPrefsPanel(wxWindow* parent);
	~MapEditorPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__MAP_EDITOR_PREFS_PANEL_H__
