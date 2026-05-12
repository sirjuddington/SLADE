#pragma once

#include "EntryPanel.h"

namespace slade
{
class TextEditorCtrl;
class FindReplacePanel;

class TextEntryPanel : public EntryPanel
{
public:
	TextEntryPanel(wxWindow* parent);
	~TextEntryPanel() override = default;

	void   refreshPanel() override;
	void   closeEntry() override;
	string statusString() override;
	bool   undo() override;
	bool   redo() override;

	// SAction Handler
	bool handleEntryPanelAction(string_view id) override;

protected:
	bool loadEntry(ArchiveEntry* entry) override;
	bool writeEntry(ArchiveEntry& entry) override;

private:
	TextEditorCtrl*   text_area_            = nullptr;
	FindReplacePanel* panel_fr_             = nullptr;
	wxChoice*         choice_text_language_ = nullptr;
	wxChoice*         choice_jump_to_       = nullptr;

	// Events
	void onTextModified(wxCommandEvent& e);
	void onChoiceLanguageChanged(wxCommandEvent& e);
	void onUpdateUI(wxStyledTextEvent& e);
};
} // namespace slade
