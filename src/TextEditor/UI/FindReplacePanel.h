#pragma once

namespace slade
{
class TextEditorCtrl;

class FindReplacePanel : public wxPanel
{
public:
	FindReplacePanel(wxWindow* parent, TextEditorCtrl& text_editor);
	~FindReplacePanel() = default;

	void   setFindText(const wxString& find) const;
	string findText() const;
	int    findFlags() const;
	string replaceText() const;

private:
	TextEditorCtrl& text_editor_;
	wxTextCtrl*     text_find_           = nullptr;
	wxTextCtrl*     text_replace_        = nullptr;
	wxButton*       btn_find_next_       = nullptr;
	wxButton*       btn_find_prev_       = nullptr;
	wxButton*       btn_replace_         = nullptr;
	wxButton*       btn_replace_all_     = nullptr;
	wxButton*       btn_close_           = nullptr;
	wxCheckBox*     cb_match_case_       = nullptr;
	wxCheckBox*     cb_match_word_whole_ = nullptr;
	wxCheckBox*     cb_match_word_start_ = nullptr;
	wxCheckBox*     cb_search_regex_     = nullptr;
	wxCheckBox*     cb_allow_escape_     = nullptr;

	// Events
	void onKeyDown(wxKeyEvent& e);
};
} // namespace slade
