
#ifndef __TEXTEDITOR_H__
#define	__TEXTEDITOR_H__

#include <wx/stc/stc.h>
#include <wx/minifram.h>
#include "Archive/ArchiveEntry.h"
#include "TextLanguage.h"
#include "TextStyle.h"

class wxButton;
class wxCheckBox;
class wxTextCtrl;
class FindReplaceDialog : public wxMiniFrame
{
private:
	wxTextCtrl*	text_find;
	wxTextCtrl*	text_replace;
	wxButton*	btn_find_next;
	wxButton*	btn_replace;
	wxButton*	btn_replace_all;
	wxCheckBox*	cb_match_case;
	wxCheckBox*	cb_match_word;

public:
	FindReplaceDialog(wxWindow* parent);
	~FindReplaceDialog();

	wxButton*	getBtnFindNext() { return btn_find_next; }
	wxButton*	getBtnReplace() { return btn_replace; }
	wxButton*	getBtnReplaceAll() { return btn_replace_all; }
	wxTextCtrl*	getTextFind() { return text_find; }
	wxTextCtrl*	getTextReplace() { return text_replace; }
	string		getFindString();
	string		getReplaceString();
	bool		matchCase();
	bool		matchWord();

	// Events
	void	onClose(wxCloseEvent& e);
	void	onKeyDown(wxKeyEvent& e);
};

class TextEditor : public wxStyledTextCtrl
{
private:
	TextLanguage*		language;
	FindReplaceDialog*	dlg_fr;

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

public:
	TextEditor(wxWindow* parent, int id);
	~TextEditor();

	TextLanguage*	getLanguage() { return language; }
	bool			setLanguage(TextLanguage* lang);

	void	setup();
	bool	applyStyleSet(StyleSet* style);
	bool	loadEntry(ArchiveEntry* entry);
	void	getRawText(MemChunk& mc);

	// Misc
	void	trimWhitespace();

	// Find/Replace
	void	showFindReplaceDialog() { dlg_fr->Show(); dlg_fr->CenterOnParent(); dlg_fr->Raise(); }
	bool	findNext(string find);
	bool	replaceCurrent(string find, string replace);
	int		replaceAll(string find, string replace);

	// Brace matching
	void	checkBraceMatch();

	// Calltips
	bool	openCalltip(int pos, int arg = 0);
	void	updateCalltip();

	// Jump To
	void	openJumpToDialog();

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
	void	onFRDBtnFindNext(wxCommandEvent& e);
	void	onFRDBtnReplace(wxCommandEvent& e);
	void	onFRDBtnReplaceAll(wxCommandEvent& e);
	void	onFRDKeyDown(wxKeyEvent& e);
	void	onActivate(wxActivateEvent& e);
};

#endif //__TEXTEDITOR_H__
