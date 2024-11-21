#pragma once

#include "SettingsPanel.h"

namespace slade::ui
{
class RadioButtonPanel;
class TextEditorStyleSettingsPanel;

class TextEditorSettingsPanel : public SettingsPanel
{
public:
	TextEditorSettingsPanel(wxWindow* parent);
	~TextEditorSettingsPanel() override = default;

	string title() const override { return "Text Editor Settings"; }

	void loadSettings() override;
	void applySettings() override;

private:
	wxSpinCtrl*       spin_tab_width_           = nullptr;
	wxCheckBox*       cb_tab_spaces_            = nullptr;
	wxCheckBox*       cb_auto_indent_           = nullptr;
	wxCheckBox*       cb_trim_whitespace_       = nullptr;
	wxCheckBox*       cb_syntax_hilight_        = nullptr;
	wxCheckBox*       cb_brace_match_           = nullptr;
	wxSpinCtrl*       spin_right_margin_        = nullptr;
	wxSpinCtrl*       spin_line_spacing_        = nullptr;
	wxCheckBox*       cb_indent_guides_         = nullptr;
	wxCheckBox*       cb_calltips_mouse_        = nullptr;
	wxCheckBox*       cb_calltips_parenthesis_  = nullptr;
	wxCheckBox*       cb_calltips_colourise_    = nullptr;
	wxCheckBox*       cb_calltips_dim_optional_ = nullptr;
	wxCheckBox*       cb_calltips_use_font_     = nullptr;
	wxCheckBox*       cb_calltips_argset_kb_    = nullptr;
	wxCheckBox*       cb_fold_enable_           = nullptr;
	wxCheckBox*       cb_fold_comments_         = nullptr;
	wxCheckBox*       cb_fold_preprocessor_     = nullptr;
	wxCheckBox*       cb_fold_lines_            = nullptr;
	wxCheckBox*       cb_match_cursor_word_     = nullptr;
	RadioButtonPanel* rbp_line_hilight_         = nullptr;
	RadioButtonPanel* rbp_show_whitespace_      = nullptr;

	TextEditorStyleSettingsPanel* style_panel_ = nullptr;

	wxPanel* createSettingsPanel(wxWindow* parent);
	wxPanel* createCodePanel(wxWindow* parent);
};
} // namespace slade::ui
