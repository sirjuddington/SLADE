
#include "Main.h"
#include "WxStuff.h"
#include "ScriptEditorPanel.h"
#include "Archive.h"
#include "SToolBar.h"
#include "EntryOperations.h"
#include "GameConfiguration.h"
#include "MapEditorWindow.h"
#include <wx/dataview.h>

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

	// Bind events
	list_words->Bind(wxEVT_COMMAND_TREELIST_ITEM_ACTIVATED, &ScriptEditorPanel::onWordListActivate, this);
}

ScriptEditorPanel::~ScriptEditorPanel()
{
	delete entry_script;
	delete entry_compiled;
}

bool ScriptEditorPanel::openScripts(ArchiveEntry* script, ArchiveEntry* compiled)
{
	// Clear current script data
	entry_script->clearData();
	entry_compiled->clearData();

	// Import script data
	if (script) entry_script->importEntry(script);
	if (compiled) entry_compiled->importEntry(compiled);

	// Load script text
	return text_editor->loadEntry(entry_script);
}

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

bool ScriptEditorPanel::handleAction(string name)
{
	// Compile Script
	if (name == "mapw_script_compile")
	{
		// Write text to entry
		wxCharBuffer buf = text_editor->GetText().mb_str();
		entry_script->importMem(buf, buf.length());

		// Compile depending on language
		string lang = theGameConfiguration->scriptLanguage();
		if (lang == "acs_hexen")
			EntryOperations::compileACS(entry_script, true, entry_compiled, theMapEditor);
		else if (lang == "acs_zdoom")
			EntryOperations::compileACS(entry_script, false, entry_compiled, theMapEditor);
	}

	// Save Script
	else if (name == "mapw_script_save")
	{
		// Write text to entry
		wxCharBuffer buf = text_editor->GetText().mb_str();
		entry_script->importMem(buf, buf.length());
	}

	// Jump To
	else if (name == "mapw_script_jumpto")
		text_editor->openJumpToDialog();

	// Not handled
	else
		return false;

	return true;
}

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
