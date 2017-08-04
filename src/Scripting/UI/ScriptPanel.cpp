
#include "Main.h"
#include "ScriptPanel.h"
#include "Scripting/ScriptManager.h"
#include "TextEditor/UI/FindReplacePanel.h"
#include "TextEditor/UI/TextEditorCtrl.h"
#include "General/SAction.h"

ScriptPanel::ScriptPanel(wxWindow* parent, ScriptManager::Script* script) :
	wxPanel(parent, -1),
	script_{ script }
{
	wxPanel::SetName("script");

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	
	// Text Editor
	text_editor_ = new TextEditorCtrl(this, -1);
	text_editor_->setLanguage(TextLanguage::fromId("sladescript"));
	if (script_)
		text_editor_->SetText(script_->text);
	sizer->Add(text_editor_, 1, wxEXPAND | wxALL, 10);

	// Find+Replace panel
	find_replace_panel_ = new FindReplacePanel(this, text_editor_);
	sizer->Add(find_replace_panel_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	find_replace_panel_->Show(false);

	text_editor_->setFindReplacePanel(find_replace_panel_);
}

string ScriptPanel::currentText() const
{
	return text_editor_->GetText();
}

bool ScriptPanel::handleAction(const string& id)
{
	// Jump To Line
	if (id == "scrm_jump_to_line")
		text_editor_->jumpToLine();

	// Find+Replace
	else if (id == "scrm_find_replace")
		text_editor_->showFindReplacePanel();

	// Word Wrapping toggle
	else if (id == "scrm_wrap")
	{
		SAction* action = SAction::fromId("scrm_wrap");
		//bool m = isModified();
		if (action->isChecked())
			text_editor_->SetWrapMode(wxSTC_WRAP_WORD);
		else
			text_editor_->SetWrapMode(wxSTC_WRAP_NONE);
		//setModified(m);
	}

	// Fold All
	else if (id == "scrm_fold_foldall")
		text_editor_->foldAll(true);

	// Unfold All
	else if (id == "scrm_fold_unfoldall")
		text_editor_->foldAll(false);

	// Not handled
	else
		return false;

	return true;
}
