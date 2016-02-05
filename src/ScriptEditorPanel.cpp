
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ScriptEditorPanel.cpp
 * Description: ScriptEditorPanel class - it's the script editor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "UI/WxStuff.h"
#include "ScriptEditorPanel.h"
#include "Archive.h"
#include "SToolBar.h"
#include "EntryOperations.h"
#include "GameConfiguration.h"
#include "MapEditorWindow.h"
#include <wx/dataview.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, script_show_language_list, true, CVAR_SAVE)


/*******************************************************************
 * SCRIPTEDTIORPANEL CLASS FUNCTIONS
 *******************************************************************/

/* ScriptEditorPanel::ScriptEditorPanel
 * ScriptEditorPanel class constructor
 *******************************************************************/
ScriptEditorPanel::ScriptEditorPanel(wxWindow* parent)
	: wxPanel(parent, -1)
{
	// Init variables
	entry_script = new ArchiveEntry();
	entry_compiled = new ArchiveEntry();

	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Toolbar
	SToolBar* toolbar = new SToolBar(this);
	sizer->Add(toolbar, 0, wxEXPAND);

	wxArrayString actions;
	actions.Add("mapw_script_save");
	actions.Add("mapw_script_compile");
	actions.Add("mapw_script_jumpto");
	actions.Add("mapw_script_togglelanguage");
	toolbar->addActionGroup("Scripts", actions);

	// Add text editor
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND);

	text_editor = new TextEditor(this, -1);
	hbox->Add(text_editor, 1, wxEXPAND|wxALL, 4);

	// Set language
	string lang = theGameConfiguration->scriptLanguage();
	if (S_CMPNOCASE(lang, "acs_hexen"))
	{
		text_editor->setLanguage(TextLanguage::getLanguage("acs"));
		entry_script->setName("SCRIPTS");
		entry_compiled->setName("BEHAVIOR");
	}
	else if (S_CMPNOCASE(lang, "acs_zdoom"))
	{
		text_editor->setLanguage(TextLanguage::getLanguage("acs_z"));
		entry_script->setName("SCRIPTS");
		entry_compiled->setName("BEHAVIOR");
	}

	// Add function/constants list
	list_words = new wxTreeListCtrl(this, -1);
	list_words->SetInitialSize(wxSize(200, -10));
	hbox->Add(list_words, 0, wxEXPAND|wxALL, 4);
	populateWordList();
	list_words->Show(script_show_language_list);

	// Bind events
	list_words->Bind(wxEVT_TREELIST_ITEM_ACTIVATED, &ScriptEditorPanel::onWordListActivate, this);
}

/* ScriptEditorPanel::~ScriptEditorPanel
 * ScriptEditorPanel class destructor
 *******************************************************************/
ScriptEditorPanel::~ScriptEditorPanel()
{
	delete entry_script;
	delete entry_compiled;
}

/* ScriptEditorPanel::openScripts
 * Opens script text from entry [scripts], and compiled script data
 * from [compiled]
 *******************************************************************/
bool ScriptEditorPanel::openScripts(ArchiveEntry* script, ArchiveEntry* compiled)
{
	// Clear current script data
	entry_script->clearData();
	entry_compiled->clearData();

	// Import script data
	if (script) entry_script->importEntry(script);
	if (compiled) entry_compiled->importEntry(compiled);

	// Process ACS open scripts
	string lang = theGameConfiguration->scriptLanguage();
	if (entry_script->getSize() > 0 && (lang == "acs_hexen" || lang == "acs_zdoom"))
	{
		SLADEMap* map = &(theMapEditor->mapEditor().getMap());
		map->mapSpecials()->processACSScripts(entry_script);
		map->mapSpecials()->updateTaggedSectors(map);
	}

	// Load script text
	return text_editor->loadEntry(entry_script);
}

/* ScriptEditorPanel::populateWordList
 * Adds all functions and constants in the script language definition
 * to the word list
 *******************************************************************/
void ScriptEditorPanel::populateWordList()
{
	// Clear/refresh list
	list_words->DeleteAllItems();
	list_words->ClearColumns();
	list_words->AppendColumn("Language");

	// Get functions and constants
	TextLanguage* tl = TextLanguage::getLanguage("acs_z");
	wxArrayString functions = tl->getFunctionsSorted();
	wxArrayString constants = tl->getConstantsSorted();

	// Add functions to list
	wxTreeListItem item = list_words->AppendItem(list_words->GetRootItem(), "Functions");
	for (unsigned a = 0; a < functions.size()-1; a++)
	{
		list_words->AppendItem(item, functions[a]);
	}

	// Add constants to list
	item = list_words->AppendItem(list_words->GetRootItem(), "Constants");
	for (unsigned a = 0; a < constants.size()-1; a++)
	{
		list_words->AppendItem(item, constants[a]);
	}
}

/* ScriptEditorPanel::saveScripts
 * Saves the current content of the text editor to the scripts entry
 *******************************************************************/
void ScriptEditorPanel::saveScripts()
{
	// Write text to entry
	wxCharBuffer buf = text_editor->GetText().mb_str();
	entry_script->importMem(buf, buf.length());

	// Process ACS open scripts
	string lang = theGameConfiguration->scriptLanguage();
	if (entry_script->getSize() > 0 && (lang == "acs_hexen" || lang == "acs_zdoom"))
	{
		SLADEMap* map = &(theMapEditor->mapEditor().getMap());
		map->mapSpecials()->processACSScripts(entry_script);
		map->mapSpecials()->updateTaggedSectors(map);
	}
}

/* ScriptEditorPanel::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool ScriptEditorPanel::handleAction(string name)
{
	// Compile Script
	if (name == "mapw_script_compile")
	{
		// Save script
		saveScripts();

		// Compile depending on language
		string lang = theGameConfiguration->scriptLanguage();
		if (lang == "acs_hexen")
			EntryOperations::compileACS(entry_script, true, entry_compiled, theMapEditor);
		else if (lang == "acs_zdoom")
			EntryOperations::compileACS(entry_script, false, entry_compiled, theMapEditor);
	}

	// Save Script
	else if (name == "mapw_script_save")
		saveScripts();

	// Jump To
	else if (name == "mapw_script_jumpto")
		text_editor->openJumpToDialog();

	// Toggle language list
	else if (name == "mapw_script_togglelanguage")
	{
		script_show_language_list = !script_show_language_list;
		list_words->Show(script_show_language_list);
		Layout();
		Refresh();
	}

	// Not handled
	else
		return false;

	return true;
}


/*******************************************************************
 * SCRIPTEDTIORPANEL CLASS EVENTS
 *******************************************************************/

/* ScriptEditorPanel::onWordListActivate
 * Called when a word list entry is activated (double-clicked)
 *******************************************************************/
void ScriptEditorPanel::onWordListActivate(wxCommandEvent& e)
{
	// Get word
	wxTreeListItem item = list_words->GetSelection();
	string word = list_words->GetItemText(item);

	// Get language
	TextLanguage* language = text_editor->getLanguage();
	if (!language)
		return;

	// Check for selection
	if (text_editor->GetSelectionStart() < text_editor->GetSelectionEnd())
	{
		// Replace selection with word
		text_editor->ReplaceSelection(word);
		text_editor->SetFocus();
		return;
	}

	// Check for function
	int pos = text_editor->GetCurrentPos();
	if (language->isFunction(word))
	{
		TLFunction* func = language->getFunction(word);

		// Add function + ()
		word += "()";
		text_editor->InsertText(pos, word);

		// Move caret inside braces and show calltip
		pos += word.length() - 1;
		text_editor->SetCurrentPos(pos + word.length() - 1);
		text_editor->SetSelection(pos, pos);
		text_editor->updateCalltip();

		text_editor->SetFocus();
	}
	else
	{
		// Not a function, just add it & move caret position
		text_editor->InsertText(pos, word);
		pos += word.length();
		text_editor->SetCurrentPos(pos);
		text_editor->SetSelection(pos, pos);
		text_editor->SetFocus();
	}
}
