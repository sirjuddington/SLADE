
#include "Main.h"
#include "App.h"
#include "ScriptPanel.h"
#include "Scripting/ScriptManager.h"
#include "TextEditor/UI/FindReplacePanel.h"
#include "TextEditor/UI/TextEditorCtrl.h"
#include "General/SAction.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"

ScriptPanel::ScriptPanel(wxWindow* parent, ScriptManager::Script* script) :
	wxPanel(parent, -1),
	script_{ script }
{
	wxPanel::SetName("script");

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Toolbar
	auto toolbar = setupToolbar();
	sizer->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	
	// Text Editor
	text_editor_ = new TextEditorCtrl(this, -1);
	text_editor_->setLanguage(TextLanguage::fromId("sladescript"));
	if (script_)
		text_editor_->SetText(script_->text);
	sizer->Add(text_editor_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Find+Replace panel
	find_replace_panel_ = new FindReplacePanel(this, text_editor_);
	sizer->Add(find_replace_panel_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	find_replace_panel_->Show(false);

	text_editor_->setFindReplacePanel(find_replace_panel_);

	last_saved_ = App::runTimer();
}

string ScriptPanel::currentText() const
{
	return text_editor_->GetText();
}

bool ScriptPanel::modified() const
{
	return text_editor_->lastModified() > last_saved_;
}

bool ScriptPanel::close()
{
	if (modified() && !script_->read_only)
	{
		auto response = wxMessageBox("Save changes to script?", "Close", wxYES_NO | wxCANCEL);
		if (response == wxCANCEL)
			return false;
		if (response == wxYES)
			save();
	}

	return true;
}

bool ScriptPanel::save()
{
	last_saved_ = App::runTimer();

	if (script_ && !script_->read_only)
	{
		script_->text = text_editor_->GetText();
		return true;
	}

	return false;
}

bool ScriptPanel::handleAction(const string& id)
{
	// Script->Save
	if (id == "scrm_save")
		save();

	// Text->Jump To Line
	else if (id == "scrm_jump_to_line")
		text_editor_->jumpToLine();

	// Text->Find+Replace
	else if (id == "scrm_find_replace")
		text_editor_->showFindReplacePanel();

	// Text->Word Wrapping toggle
	else if (id == "scrm_wrap")
	{
		SAction* action = SAction::fromId("scrm_wrap");
		if (action->isChecked())
			text_editor_->SetWrapMode(wxSTC_WRAP_WORD);
		else
			text_editor_->SetWrapMode(wxSTC_WRAP_NONE);
	}

	// Text->Fold All
	else if (id == "scrm_fold_foldall")
		text_editor_->foldAll(true);

	// Text->Unfold All
	else if (id == "scrm_fold_unfoldall")
		text_editor_->foldAll(false);

	// Text->Not handled
	else
		return false;

	return true;
}

SToolBar* ScriptPanel::setupToolbar()
{
	auto toolbar = new SToolBar(this);

	// Create Script toolbar
	auto tbg_script = new SToolBarGroup(toolbar, "_Script");
	tbg_script->addActionButton("scrm_run", "", true);
	tbg_script->addActionButton("scrm_save", "", true)->Enable(!script_->read_only);
	toolbar->addGroup(tbg_script);

	return toolbar;
}
