
#ifndef __TEXTEDITOR_H__
#define	__TEXTEDITOR_H__

#include <wx/stc/stc.h>
#include <wx/panel.h>
#include "Archive/ArchiveEntry.h"
#include "TextLanguage.h"
#include "TextStyle.h"

class wxButton;
class wxCheckBox;
class wxTextCtrl;
class TextEditor;
class FindReplacePanel : public wxPanel
{
public:
	FindReplacePanel(wxWindow* parent, TextEditor* text_editor);
	~FindReplacePanel();

	void	setFindText(string find);
	string	getFindText();
	int		getFindFlags();
	string	getReplaceText();

private:
	TextEditor*	text_editor;

	wxTextCtrl*	text_find;
	wxTextCtrl*	text_replace;
	wxButton*	btn_find_next;
	wxButton*	btn_find_prev;
	wxButton*	btn_replace;
	wxButton*	btn_replace_all;
	wxCheckBox*	cb_match_case;
	wxCheckBox*	cb_match_word_whole;
	wxCheckBox*	cb_match_word_start;
	wxCheckBox*	cb_search_regex;
	wxCheckBox*	cb_allow_escape;

	// Events
	void	onBtnFindNext(wxCommandEvent& e);
	void	onBtnFindPrev(wxCommandEvent& e);
	void	onBtnReplace(wxCommandEvent& e);
	void	onBtnReplaceAll(wxCommandEvent& e);
	void	onKeyDown(wxKeyEvent& e);
	void	onTextFindEnter(wxCommandEvent& e);
	void	onTextReplaceEnter(wxCommandEvent& e);
};

class TextEditor : public wxStyledTextCtrl
{
private:
	TextLanguage*		language;
	FindReplacePanel*	panel_fr;

	// Calltip stuff
	TLFunction*	ct_function;
	int			ct_argset;
	int			ct_start;

	// Autocompletion
	string		autocomp_list;

	// Jump To stuff
	struct jp_t
	{
		string	name;
		int		line;
	};

	// Brace matching
	int	bm_cursor_last_pos;

public:
	TextEditor(wxWindow* parent, int id);
	~TextEditor();

	TextLanguage*	getLanguage() { return language; }
	bool			setLanguage(TextLanguage* lang);

	void	setup();
	void	setupFoldMargin(TextStyle* margin_style = NULL);
	bool	applyStyleSet(StyleSet* style);
	bool	loadEntry(ArchiveEntry* entry);
	void	getRawText(MemChunk& mc);

	// Misc
	void	trimWhitespace();

	// Find/Replace
	void	setFindReplacePanel(FindReplacePanel* panel) { panel_fr = panel; }
	void	showFindReplacePanel(bool show = true);
	bool	findNext(string find, int flags);
	bool	findPrev(string find, int flags);
	bool	replaceCurrent(string find, string replace, int flags);
	int		replaceAll(string find, string replace, int flags);

	// Brace matching
	void	checkBraceMatch();

	// Calltips
	bool	openCalltip(int pos, int arg = 0);
	void	updateCalltip();

	// Jump To
	void	openJumpToDialog();
	void	jumpToLine();

	// Folding
	void	foldAll(bool fold = true);
	void	setupFolding();

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
};

#endif //__TEXTEDITOR_H__
