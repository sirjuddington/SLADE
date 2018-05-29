#pragma once

class TextEditorCtrl;

class FindReplacePanel : public wxPanel
{
public:
	FindReplacePanel(wxWindow* parent, TextEditorCtrl& text_editor);
	~FindReplacePanel() {}

	void	setFindText(string find) const;
	string	findText() const;
	int		findFlags() const;
	string	replaceText() const;

private:
	TextEditorCtrl&	text_editor_;
	wxTextCtrl*		text_find_;
	wxTextCtrl*		text_replace_;
	wxButton*		btn_find_next_;
	wxButton*		btn_find_prev_;
	wxButton*		btn_replace_;
	wxButton*		btn_replace_all_;
	wxButton*		btn_close_;
	wxCheckBox*		cb_match_case_;
	wxCheckBox*		cb_match_word_whole_;
	wxCheckBox*		cb_match_word_start_;
	wxCheckBox*		cb_search_regex_;
	wxCheckBox*		cb_allow_escape_;

	// Events
	void	onKeyDown(wxKeyEvent& e);
};
