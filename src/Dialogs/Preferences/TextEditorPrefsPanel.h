#pragma once

#include "PrefsPanelBase.h"

class TextEditorPrefsPanel : public PrefsPanelBase
{
public:
	TextEditorPrefsPanel(wxWindow* parent);
	~TextEditorPrefsPanel();

	void	init() override;
	void	applyPreferences() override;

private:
	wxSpinCtrl*	spin_tab_width_;
	wxCheckBox*	cb_tab_spaces_;
	wxCheckBox*	cb_auto_indent_;
	wxCheckBox*	cb_trim_whitespace_;
	wxCheckBox*	cb_syntax_hilight_;
	wxCheckBox*	cb_brace_match_;
	wxSpinCtrl*	spin_right_margin_;
	wxSpinCtrl*	spin_line_spacing_;
	wxCheckBox*	cb_indent_guides_;
	wxCheckBox*	cb_calltips_mouse_;
	wxCheckBox*	cb_calltips_parenthesis_;
	wxCheckBox*	cb_calltips_colourise_;
	wxCheckBox*	cb_calltips_dim_optional_;
	wxCheckBox*	cb_calltips_use_font_;
	wxCheckBox* cb_calltips_argset_kb_;
	wxCheckBox*	cb_fold_enable_;
	wxCheckBox*	cb_fold_comments_;
	wxCheckBox*	cb_fold_preprocessor_;
	wxCheckBox*	cb_fold_lines_;
	wxCheckBox*	cb_match_cursor_word_;
	wxChoice*	choice_line_hilight_;
	wxChoice*	choice_show_whitespace_;
};
