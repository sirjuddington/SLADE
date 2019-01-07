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
	~ScriptEditorPanel() = default;

	ArchiveEntry* scriptEntry() const { return entry_script_.get(); }
	ArchiveEntry* compiledEntry() const { return entry_compiled_.get(); }

	bool openScripts(ArchiveEntry* scripts, ArchiveEntry* compiled = nullptr) const;
	void populateWordList() const;
	void saveScripts() const;
	void updateUI() const;

	bool handleAction(const string& name) override;

private:
	string                        script_text_;
	std::unique_ptr<ArchiveEntry> entry_script_;
	std::unique_ptr<ArchiveEntry> entry_compiled_;

	TextEditorCtrl*   text_editor_;
	wxTreeListCtrl*   list_words_;
	FindReplacePanel* panel_fr_;
	wxChoice*         choice_jump_to_;

	void onWordListActivate(wxCommandEvent& e);
};
