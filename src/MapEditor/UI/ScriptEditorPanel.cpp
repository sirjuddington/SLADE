
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ScriptEditorPanel.cpp
// Description: ScriptEditorPanel class - it's the script editor
//              (this will be replaced by the script manager eventually)
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
#include "ScriptEditorPanel.h"
#include "Game/Configuration.h"
#include "MainEditor/EntryOperations.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "TextEditor/UI/FindReplacePanel.h"
#include "TextEditor/UI/TextEditorCtrl.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, script_show_language_list, true, CVar::Flag::Save)
CVAR(Bool, script_word_wrap, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, txed_trim_whitespace)


// -----------------------------------------------------------------------------
//
// ScriptEditorPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ScriptEditorPanel class constructor
// -----------------------------------------------------------------------------
ScriptEditorPanel::ScriptEditorPanel(wxWindow* parent) :
	wxPanel(parent, -1),
	entry_script_{ new ArchiveEntry() },
	entry_compiled_{ new ArchiveEntry() }
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Toolbar
	auto toolbar = new SToolBar(this);
	sizer->Add(toolbar, 0, wxEXPAND);

	wxArrayString actions;
	toolbar->addActionGroup("Scripts", { "mapw_script_save", "mapw_script_compile", "mapw_script_togglelanguage" });

	// Jump To toolbar group
	auto group_jump_to = new SToolBarGroup(toolbar, "Jump To", true);
	choice_jump_to_    = new wxChoice(group_jump_to, -1, wxDefaultPosition, wxutil::scaledSize(200, -1));
	group_jump_to->addCustomControl(choice_jump_to_);
	toolbar->addGroup(group_jump_to);

	// Add text editor
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND);
	auto vbox = new wxBoxSizer(wxVERTICAL);
	hbox->Add(vbox, 1, wxEXPAND);

	text_editor_ = new TextEditorCtrl(this, -1);
	text_editor_->setJumpToControl(choice_jump_to_);
	vbox->Add(text_editor_, 1, wxEXPAND | wxALL, ui::pad());

	// Set language
	wxString lang = game::configuration().scriptLanguage();
	if (S_CMPNOCASE(lang, "acs_hexen"))
	{
		text_editor_->setLanguage(TextLanguage::fromId("acs"));
		entry_script_->setName("SCRIPTS");
		entry_compiled_->setName("BEHAVIOR");
	}
	else if (S_CMPNOCASE(lang, "acs_zdoom"))
	{
		text_editor_->setLanguage(TextLanguage::fromId("acs_z"));
		entry_script_->setName("SCRIPTS");
		entry_compiled_->setName("BEHAVIOR");
	}
	entry_script_->setName("SCRIPTS");
	entry_compiled_->setName("BEHAVIOR");

	// Add Find+Replace panel
	panel_fr_ = new FindReplacePanel(this, *text_editor_);
	text_editor_->setFindReplacePanel(panel_fr_);
	vbox->Add(panel_fr_, 0, wxEXPAND | wxALL, ui::pad());
	panel_fr_->Hide();

	// Add function/constants list
	list_words_ = new wxTreeListCtrl(this, -1);
	list_words_->SetInitialSize(wxutil::scaledSize(200, -10));
	hbox->Add(list_words_, 0, wxEXPAND | wxALL, ui::pad());
	populateWordList();
	list_words_->Show(script_show_language_list);

	// Bind events
	list_words_->Bind(wxEVT_TREELIST_ITEM_ACTIVATED, &ScriptEditorPanel::onWordListActivate, this);
}

// -----------------------------------------------------------------------------
// Opens script text from entry [scripts], and compiled script data from
// [compiled]
// -----------------------------------------------------------------------------
bool ScriptEditorPanel::openScripts(ArchiveEntry* script, ArchiveEntry* compiled) const
{
	// Clear current script data
	entry_script_->clearData();
	entry_compiled_->clearData();

	// Import script data
	if (script)
		entry_script_->importEntry(script);
	if (compiled)
		entry_compiled_->importEntry(compiled);

	// Process ACS open scripts
	wxString lang = game::configuration().scriptLanguage();
	if (entry_script_->size() > 0 && (lang == "acs_hexen" || lang == "acs_zdoom"))
	{
		auto& map = mapeditor::editContext().map();
		map.mapSpecials()->processACSScripts(entry_script_.get());
		map.mapSpecials()->updateTaggedSectors(&map);
	}

	// Load script text
	bool ok = text_editor_->loadEntry(entry_script_.get());
	if (ok)
	{
		text_editor_->updateJumpToList();
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Adds all functions and constants in the script language definition to the
// word list
// -----------------------------------------------------------------------------
void ScriptEditorPanel::populateWordList() const
{
	// Clear/refresh list
	list_words_->DeleteAllItems();
	list_words_->ClearColumns();
	list_words_->AppendColumn("Language");

	// Get functions and constants
	auto tl        = TextLanguage::fromId("acs_z");
	auto functions = tl->functionsSorted();
	auto constants = tl->wordListSorted(TextLanguage::WordType::Constant);

	// Add functions to list
	auto item = list_words_->AppendItem(list_words_->GetRootItem(), "Functions");
	for (unsigned a = 0; a < functions.size() - 1; a++)
		list_words_->AppendItem(item, functions[a]);

	// Add constants to list
	item = list_words_->AppendItem(list_words_->GetRootItem(), "Constants");
	for (unsigned a = 0; a < constants.size() - 1; a++)
		list_words_->AppendItem(item, constants[a]);
}

// -----------------------------------------------------------------------------
// Saves the current content of the text editor to the scripts entry
// -----------------------------------------------------------------------------
void ScriptEditorPanel::saveScripts() const
{
	// Trim whitespace
	if (txed_trim_whitespace)
		text_editor_->trimWhitespace();

	// Write text to entry
	wxCharBuffer buf = text_editor_->GetText().mb_str();
	entry_script_->importMem(buf, buf.length());

	// Process ACS open scripts
	wxString lang = game::configuration().scriptLanguage();
	if (entry_script_->size() > 0 && (lang == "acs_hexen" || lang == "acs_zdoom"))
	{
		auto map = &(mapeditor::editContext().map());
		map->mapSpecials()->processACSScripts(entry_script_.get());
		map->mapSpecials()->updateTaggedSectors(map);
	}
}

// -----------------------------------------------------------------------------
// Update script editor UI
// -----------------------------------------------------------------------------
void ScriptEditorPanel::updateUI() const
{
	text_editor_->updateJumpToList();
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool ScriptEditorPanel::handleAction(string_view name)
{
	// Compile Script
	if (name == "mapw_script_compile")
	{
		// Save script
		saveScripts();

		// Compile depending on language
		wxString lang = game::configuration().scriptLanguage();
		if (lang == "acs_hexen")
			entryoperations::compileACS(
				entry_script_.get(), true, entry_compiled_.get(), dynamic_cast<wxFrame*>(mapeditor::windowWx()));
		else if (lang == "acs_zdoom")
			entryoperations::compileACS(
				entry_script_.get(), false, entry_compiled_.get(), dynamic_cast<wxFrame*>(mapeditor::windowWx()));
	}

	// Save Script
	else if (name == "mapw_script_save")
		saveScripts();

	// Toggle language list
	else if (name == "mapw_script_togglelanguage")
	{
		list_words_->Show(script_show_language_list);
		Layout();
		Refresh();
	}

	// Not handled
	else
		return false;

	return true;
}


// -----------------------------------------------------------------------------
//
// ScriptEditorPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a word list entry is activated (double-clicked)
// -----------------------------------------------------------------------------
void ScriptEditorPanel::onWordListActivate(wxCommandEvent& e)
{
	// Get word
	auto item = list_words_->GetSelection();
	auto word = list_words_->GetItemText(item).ToStdString();

	// Get language
	auto language = text_editor_->language();
	if (!language)
		return;

	// Check for selection
	if (text_editor_->GetSelectionStart() < text_editor_->GetSelectionEnd())
	{
		// Replace selection with word
		text_editor_->ReplaceSelection(word);
		text_editor_->SetFocus();
		return;
	}

	// Check for function
	int pos = text_editor_->GetCurrentPos();
	if (language->isFunction(word))
	{
		auto func = language->function(word);

		// Add function + ()
		word += "()";
		text_editor_->InsertText(pos, word);

		// Move caret inside braces and show calltip
		pos += word.length() - 1;
		text_editor_->SetCurrentPos(pos + word.length() - 1);
		text_editor_->SetSelection(pos, pos);
		text_editor_->updateCalltip();

		text_editor_->SetFocus();
	}
	else
	{
		// Not a function, just add it & move caret position
		text_editor_->InsertText(pos, word);
		pos += word.length();
		text_editor_->SetCurrentPos(pos);
		text_editor_->SetSelection(pos, pos);
		text_editor_->SetFocus();
	}
}
