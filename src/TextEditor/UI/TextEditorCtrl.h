#pragma once

#include "common.h"
#include "Archive/ArchiveEntry.h"
#include "TextEditor/TextLanguage.h"
#include "TextEditor/TextStyle.h"
#include "TextEditor/Lexer.h"

class wxButton;
class wxCheckBox;
class wxTextCtrl;
class FindReplacePanel;
class SCallTip;
class wxChoice;

wxDECLARE_EVENT(wxEVT_COMMAND_JTCALCULATOR_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_TEXT_CHANGED, wxCommandEvent);

class JumpToCalculator : public wxThread
{
public:
	JumpToCalculator(wxEvtHandler* handler, string text, vector<string> block_names, vector<string> ignore)
		: handler_(handler), text_(text), block_names_(block_names), ignore_(ignore) {}
	virtual ~JumpToCalculator() {}

	ExitCode Entry();

private:
	wxEvtHandler*	handler_;
	string			text_;
	vector<string>	block_names_;
	vector<string>	ignore_;
};

class TextEditorCtrl : public wxStyledTextCtrl
{
public:
	TextEditorCtrl(wxWindow* parent, int id);
	~TextEditorCtrl();

	TextLanguage*	language() const { return language_; }
	long			lastModified() const { return last_modified_; }

	bool	setLanguage(TextLanguage* lang);

	void	setup();
	void	setupFoldMargin(TextStyle* margin_style = nullptr);
	bool	applyStyleSet(StyleSet* style);
	bool	loadEntry(ArchiveEntry* entry);
	void	getRawText(MemChunk& mc);

	// Misc
	void	trimWhitespace();

	// Find/Replace
	void	setFindReplacePanel(FindReplacePanel* panel) { panel_fr_ = panel; }
	void	showFindReplacePanel(bool show = true);
	bool	findNext(string find, int flags);
	bool	findPrev(string find, int flags);
	bool	replaceCurrent(string find, string replace, int flags);
	int		replaceAll(string find, string replace, int flags);

	// Hilight/matching
	void	checkBraceMatch();
	void	matchWord();
	void	clearWordMatch();

	// Calltips
	void	showCalltip(int position);
	void	hideCalltip();
	bool	openCalltip(int pos, int arg = 0, bool dwell = false);
	void	updateCalltip();

	// Jump To
	void	setJumpToControl(wxChoice* jump_to);
	void	updateJumpToList();
	void	jumpToLine();

	// Folding
	void	foldAll(bool fold = true);
	void	setupFolding();

	// Comments
	void	lineComment();
	void 	blockComment();
	void 	cycleComments();

private:
	TextLanguage*			language_;
	FindReplacePanel*		panel_fr_;
	SCallTip*				call_tip_;
	wxChoice*				choice_jump_to_;
	JumpToCalculator*		jump_to_calculator_;
	std::unique_ptr<Lexer>	lexer_;
	string					prev_word_match_;
	string					autocomp_list_;
	vector<int>				jump_to_lines_;
	long					last_modified_;

	// State tracking for updates
	int	prev_cursor_pos_;
	int	prev_text_length_;
	int prev_brace_match_;

	// Timed update stuff
	wxTimer	timer_update_;
	bool	update_jump_to_;
	bool	update_word_match_;

	// Calltip stuff
	TLFunction*	ct_function_;
	int			ct_argset_;
	int			ct_start_;
	bool		ct_dwell_;

	// Default comment strings
	const string	default_line_comment_  = "//";
	const string	default_begin_comment_ = "/*";
	const string	default_end_comment_   = "*/";

	// Events
	void	onKeyDown(wxKeyEvent& e);
	void	onKeyUp(wxKeyEvent& e);
	void	onCharAdded(wxStyledTextEvent& e);
	void	onUpdateUI(wxStyledTextEvent& e);
	void	onCalltipClicked(wxStyledTextEvent& e);
	void	onMouseDwellStart(wxStyledTextEvent& e);
	void	onMouseDwellEnd(wxStyledTextEvent& e);
	void	onMouseDown(wxMouseEvent& e);
	void	onFocusLoss(wxFocusEvent& e);
	void	onActivate(wxActivateEvent& e);
	void	onMarginClick(wxStyledTextEvent& e);
	void	onJumpToCalculateComplete(wxThreadEvent& e);
	void	onJumpToChoiceSelected(wxCommandEvent& e);
	void	onModified(wxStyledTextEvent& e);
	void	onUpdateTimer(wxTimerEvent& e);
	void	onStyleNeeded(wxStyledTextEvent& e);
};
