
#ifndef __SCRIPT_EDITOR_PANEL_H__
#define __SCRIPT_EDITOR_PANEL_H__

#include "common.h"
#include "General/SAction.h"

class ArchiveEntry;
class wxTreeListCtrl;
class TextEditorCtrl;
class FindReplacePanel;

class ScriptEditorPanel : public wxPanel, SActionHandler
{
private:
	string			script_text;
	ArchiveEntry*	entry_script;
	ArchiveEntry*	entry_compiled;

	TextEditorCtrl*		text_editor;
	wxButton*			btn_save;
	wxButton*			btn_compile;
	wxTreeListCtrl*		list_words;
	FindReplacePanel*	panel_fr;
	wxChoice*			choice_jump_to;

public:
	ScriptEditorPanel(wxWindow* parent);
	~ScriptEditorPanel();

	ArchiveEntry*	scriptEntry() { return entry_script; }
	ArchiveEntry*	compiledEntry() { return entry_compiled; }

	bool	openScripts(ArchiveEntry* scripts, ArchiveEntry* compiled = NULL);
	void	populateWordList();
	void	saveScripts();
	void	updateUI();

	bool	handleAction(string name);

	void	onWordListActivate(wxCommandEvent& e);
};

#endif//__SCRIPT_EDITOR_PANEL_H__
