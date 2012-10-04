
#ifndef __SCRIPT_EDITOR_PANEL_H__
#define __SCRIPT_EDITOR_PANEL_H__

#include "TextEditor.h"
#include "MainApp.h"
#include <wx/treelist.h>

class ArchiveEntry;
class ScriptEditorPanel : public wxPanel, SActionHandler {
private:
	string			script_text;
	ArchiveEntry*	entry_script;
	ArchiveEntry*	entry_compiled;

	TextEditor*		text_editor;
	wxButton*		btn_save;
	wxButton*		btn_compile;
	wxTreeListCtrl*	list_words;

public:
	ScriptEditorPanel(wxWindow* parent);
	~ScriptEditorPanel();

	void	setScriptEntry(ArchiveEntry* entry) { entry_script = entry; }
	void	setCompiledEntry(ArchiveEntry* entry) { entry_compiled = entry; }

	bool	openScripts(ArchiveEntry* entry);
	void	populateWordList();

	bool	handleAction(string name);

	void	onWordListActivate(wxCommandEvent& e);
};

#endif//__SCRIPT_EDITOR_PANEL_H__
