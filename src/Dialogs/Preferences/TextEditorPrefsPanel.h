
#ifndef __TEXT_EDITOR_PREFS_PANEL_H__
#define __TEXT_EDITOR_PREFS_PANEL_H__

#include "PrefsPanelBase.h"

class TextEditorPrefsPanel : public PrefsPanelBase
{
private:
	wxSpinCtrl*	spin_tab_width;
	wxCheckBox*	cb_auto_indent;
	wxCheckBox*	cb_trim_whitespace;
	wxCheckBox*	cb_syntax_hilight;
	wxCheckBox*	cb_brace_match;
	wxSpinCtrl*	spin_right_margin;
	wxCheckBox*	cb_indent_guides;
	wxCheckBox*	cb_calltips_mouse;
	wxCheckBox*	cb_calltips_parenthesis;

public:
	TextEditorPrefsPanel(wxWindow* parent);
	~TextEditorPrefsPanel();

	void	init();
	void	applyPreferences();
};

#endif//__TEXT_EDITOR_PREFS_PANEL_H__
