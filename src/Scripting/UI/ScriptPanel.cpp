
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ScriptPanel.cpp
// Description: A panel for displaying/editing a single script in the script
//              editor
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ScriptPanel.h"
#include "App.h"
#include "General/SAction.h"
#include "General/UI.h"
#include "Scripting/ScriptManager.h"
#include "TextEditor/TextLanguage.h"
#include "TextEditor/UI/FindReplacePanel.h"
#include "TextEditor/UI/TextEditorCtrl.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ScriptPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ScriptPanel class constructor
// -----------------------------------------------------------------------------
ScriptPanel::ScriptPanel(wxWindow* parent, scriptmanager::Script* script) : wxPanel(parent, -1), script_{ script }
{
	wxPanel::SetName("script");

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Toolbar
	auto toolbar = setupToolbar();
	sizer->AddSpacer(ui::padMin());
	sizer->Add(toolbar, wxutil::sfWithBorder(0, wxLEFT | wxRIGHT).Expand());
	sizer->AddSpacer(ui::padMin());

	// Text Editor
	text_editor_ = new TextEditorCtrl(this, -1);
	text_editor_->setLanguage(TextLanguage::fromId("sladescript"));
	if (script_)
		text_editor_->SetText(script_->text);
	sizer->Add(text_editor_, wxutil::sfWithBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Find+Replace panel
	find_replace_panel_ = new FindReplacePanel(this, *text_editor_);
	sizer->Add(find_replace_panel_, wxutil::sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	find_replace_panel_->Show(false);

	text_editor_->setFindReplacePanel(find_replace_panel_);

	last_saved_ = app::runTimer();
}

// -----------------------------------------------------------------------------
// Returns the current text content in the editor
// -----------------------------------------------------------------------------
string ScriptPanel::currentText() const
{
	return text_editor_->GetText().ToStdString();
}

// -----------------------------------------------------------------------------
// Returns true if the script content has been modified since it was last saved
// -----------------------------------------------------------------------------
bool ScriptPanel::modified() const
{
	return text_editor_->lastModified() > last_saved_;
}

// -----------------------------------------------------------------------------
// Prepares to close the script (prompts to save if modified etc.)
// Returns true if the panel can be closed
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Saves the text editor content to the script (if it's allowed)
// -----------------------------------------------------------------------------
bool ScriptPanel::save()
{
	last_saved_ = app::runTimer();

	if (script_ && !script_->read_only)
	{
		script_->text = text_editor_->GetText();
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool ScriptPanel::handleAction(string_view id)
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
		auto action = SAction::fromId("scrm_wrap");
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

// -----------------------------------------------------------------------------
// Creates and returns the toolbar for this script panel
// -----------------------------------------------------------------------------
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
