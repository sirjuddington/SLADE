#pragma once


class JumpToCalculator;
class wxButton;
class wxCheckBox;
class wxTextCtrl;
class wxChoice;

wxDECLARE_EVENT(wxEVT_COMMAND_JTCALCULATOR_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_TEXT_CHANGED, wxCommandEvent);

namespace slade
{
class StyleSet;
class TextStyle;
class TLFunction;
class TextLanguage;
class Lexer;
class FindReplacePanel;
class SCallTip;

class TextEditorCtrl : public wxStyledTextCtrl
{
public:
	TextEditorCtrl(wxWindow* parent, int id);
	~TextEditorCtrl() override;

	TextLanguage* language() const { return language_; }
	long          lastModified() const { return last_modified_; }

	bool setLanguage(TextLanguage* lang);

	void setup();
	void setupFoldMargin(const TextStyle* margin_style = nullptr);
	bool applyStyleSet(StyleSet* style);
	bool loadEntry(const ArchiveEntry* entry);
	void getRawText(MemChunk& mc) const;

	// Misc
	void trimWhitespace();

	// Find/Replace
	void setFindReplacePanel(FindReplacePanel* panel) { panel_fr_ = panel; }
	void showFindReplacePanel(bool show = true);
	bool findNext(string_view find, int flags);
	bool findPrev(string_view find, int flags);
	bool replaceCurrent(string_view find, string_view replace, int flags);
	int  replaceAll(string_view find, string_view replace, int flags);
	void selectNextOccurrence();
	void selectAllOccurrences();

	// Hilight/matching
	void checkBraceMatch();
	void matchWord();
	void clearWordMatch();

	// Calltips
	void showCalltip(int position);
	void hideCalltip();
	bool openCalltip(int pos, int arg = 0, bool dwell = false);
	void updateCalltip();

	// Jump To
	void setJumpToControl(wxChoice* jump_to);
	void updateJumpToList();
	void jumpToLine();

	// Folding
	void foldAll(bool fold = true);
	void setupFolding();

	// Comments
	void lineComment();
	void blockComment();
	void cycleComments() const;

private:
	TextLanguage*     language_           = nullptr;
	FindReplacePanel* panel_fr_           = nullptr;
	SCallTip*         call_tip_           = nullptr;
	wxChoice*         choice_jump_to_     = nullptr;
	JumpToCalculator* jump_to_calculator_ = nullptr;
	unique_ptr<Lexer> lexer_;
	wxString          prev_word_match_;
	string            autocomp_list_;
	vector<int>       jump_to_lines_;
	long              last_modified_ = 0;

	// State tracking for updates
	int  prev_cursor_pos_      = -1;
	int  prev_text_length_     = -1;
	int  prev_brace_match_     = -1;
	bool block_comment_closed_ = false;

	// Timed update stuff
	wxTimer timer_update_;
	bool    update_jump_to_    = false;
	bool    update_word_match_ = false;

	// Calltip stuff
	TLFunction* ct_function_ = nullptr;
	int         ct_argset_   = 0;
	int         ct_start_    = 0;
	bool        ct_dwell_    = false;

	// Default comment strings
	const wxString default_line_comment_  = wxS("//");
	const wxString default_begin_comment_ = wxS("/*");
	const wxString default_end_comment_   = wxS("*/");

	// Events
	void onKeyDown(wxKeyEvent& e);
	void onKeyUp(wxKeyEvent& e);
	void onCharAdded(wxStyledTextEvent& e);
	void onUpdateUI(wxStyledTextEvent& e);
	void onCalltipClicked(wxStyledTextEvent& e);
	void onMouseDwellStart(wxStyledTextEvent& e);
	void onMouseDwellEnd(wxStyledTextEvent& e);
	void onMouseDown(wxMouseEvent& e);
	void onFocusLoss(wxFocusEvent& e);
	void onActivate(wxActivateEvent& e);
	void onMarginClick(wxStyledTextEvent& e);
	void onJumpToCalculateComplete(wxThreadEvent& e);
	void onJumpToChoiceSelected(wxCommandEvent& e);
	void onModified(wxStyledTextEvent& e);
	void onUpdateTimer(wxTimerEvent& e);
	void onStyleNeeded(wxStyledTextEvent& e);
};
} // namespace slade
