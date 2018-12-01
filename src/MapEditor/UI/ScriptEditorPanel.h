#pragma once

#include "General/SAction.h"

class ArchiveEntry;
class wxTreeListCtrl;
class TextEditorCtrl;
class FindReplacePanel;

class ScriptEditorPanel : public wxPanel, SActionHandler
{
public:
	ScriptEditorPanel(wxWindow* parent);
	~ScriptEditorPanel();

	ArchiveEntry* scriptEntry() { return entry_script_; }
	ArchiveEntry* compiledEntry() { return entry_compiled_; }

	bool openScripts(ArchiveEntry* scripts, ArchiveEntry* compiled = nullptr);
	void populateWordList();
	void saveScripts();
	void updateUI();

	bool handleAction(string name);

private:
	string        script_text_;
	ArchiveEntry* entry_script_;
	ArchiveEntry* entry_compiled_;

	TextEditorCtrl*   text_editor_;
	wxButton*         btn_save_;
	wxButton*         btn_compile_;
	wxTreeListCtrl*   list_words_;
	FindReplacePanel* panel_fr_;
	wxChoice*         choice_jump_to_;

	void onWordListActivate(wxCommandEvent& e);
};
