
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TextEntryPanel.cpp
 * Description: TextEntryPanel class. The UI for editing text entries.
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
#include "Archive/ArchiveManager.h"
#include "Game/Configuration.h"
#include "TextEditor/UI/FindReplacePanel.h"
#include "TextEditor/UI/TextEditorCtrl.h"
#include "TextEntryPanel.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
wxArrayString languages;


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, txed_trim_whitespace)


/*******************************************************************
 * TEXTENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* TextEntryPanel::TextEntryPanel
 * TextEntryPanel class constructor
 *******************************************************************/
TextEntryPanel::TextEntryPanel(wxWindow* parent)
	: EntryPanel(parent, "text")
{
	// Create the text area
	text_area_ = new TextEditorCtrl(this, -1);
	sizer_main->Add(text_area_, 1, wxEXPAND, 0);

	// Create the find+replace panel
	panel_fr_ = new FindReplacePanel(this, text_area_);
	text_area_->setFindReplacePanel(panel_fr_);
	panel_fr_->Hide();
	sizer_main->Add(panel_fr_, 0, wxEXPAND|wxTOP, 8);
	sizer_main->AddSpacer(4);

	// Add 'Text Language' choice to toolbar
	SToolBarGroup* group_language = new SToolBarGroup(toolbar, "Text Language", true);
	languages = TextLanguage::languageNames();
	languages.Sort();
	languages.Insert("None", 0, 1);
	choice_text_language_ = new wxChoice(group_language, -1, wxDefaultPosition, wxDefaultSize, languages);
	choice_text_language_->Select(0);
	group_language->addCustomControl(choice_text_language_);
	toolbar->addGroup(group_language);

	// Add 'Jump To' choice to toolbar
	SToolBarGroup* group_jump_to = new SToolBarGroup(toolbar, "Jump To", true);
	choice_jump_to_ = new wxChoice(group_jump_to, -1, wxDefaultPosition, wxSize(200, -1));
	group_jump_to->addCustomControl(choice_jump_to_);
	toolbar->addGroup(group_jump_to);
	text_area_->setJumpToControl(choice_jump_to_);

	// Bind events
	choice_text_language_->Bind(wxEVT_CHOICE, &TextEntryPanel::onChoiceLanguageChanged, this);
	text_area_->Bind(wxEVT_TEXT_CHANGED, &TextEntryPanel::onTextModified, this);
	text_area_->Bind(wxEVT_STC_UPDATEUI, &TextEntryPanel::onUpdateUI, this);

	// Custom toolbar
	custom_toolbar_actions = "arch_scripts_compileacs;arch_scripts_compilehacs";
	toolbar->addActionGroup("Scripts", wxSplit(custom_toolbar_actions, ';'));


	// --- Custom menu ---
	menu_custom = new wxMenu();
	SAction::fromId("ptxt_find_replace")->addToMenu(menu_custom);
	SAction::fromId("ptxt_jump_to_line")->addToMenu(menu_custom);

	// 'Code Folding' submenu
	wxMenu* menu_fold = new wxMenu();
	menu_custom->AppendSubMenu(menu_fold, "Code Folding");
	SAction::fromId("ptxt_fold_foldall")->addToMenu(menu_fold);
	SAction::fromId("ptxt_fold_unfoldall")->addToMenu(menu_fold);

	// 'Compile' submenu
	wxMenu* menu_scripts = new wxMenu();
	menu_custom->AppendSubMenu(menu_scripts, "Compile");
	SAction::fromId("arch_scripts_compileacs")->addToMenu(menu_scripts);
	SAction::fromId("arch_scripts_compilehacs")->addToMenu(menu_scripts);

	menu_custom->AppendSeparator();

	SAction::fromId("ptxt_wrap")->addToMenu(menu_custom);
	custom_menu_name = "Text";


	Layout();
}

/* TextEntryPanel::~TextEntryPanel
 * TextEntryPanel class destructor
 *******************************************************************/
TextEntryPanel::~TextEntryPanel()
{
}

/* TextEntryPanel::loadEntry
 * Loads an entry into the panel as text
 *******************************************************************/
bool TextEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Load entry into the text editor
	if (!text_area_->loadEntry(entry))
		return false;

	// Scroll to previous position (if any)
	if (entry->exProps().propertyExists("TextPosition"))
		text_area_->GotoPos((int)(entry->exProp("TextPosition")));

	// --- Attempt to determine text language ---
	TextLanguage* tl = nullptr;

	// Level markers use FraggleScript
	if (entry->getType() == EntryType::mapMarkerType())
		tl = TextLanguage::fromId("fragglescript");

	// From entry language hint
	if (entry->exProps().propertyExists("TextLanguage"))
	{
		string lang_id = entry->exProp("TextLanguage");
		tl = TextLanguage::fromId(lang_id);
	}

	// Or, from entry type
	if (!tl && entry->getType()->extraProps().propertyExists("text_language"))
	{
		string lang_id = entry->getType()->extraProps()["text_language"];
		tl = TextLanguage::fromId(lang_id);
	}

	// Load language
	text_area_->setLanguage(tl);

	// Select it in the choice box
	if (tl)
	{
		for (unsigned a = 0; a < languages.size(); a++)
		{
			if (S_CMPNOCASE(tl->name(), languages[a]))
			{
				choice_text_language_->Select(a);
				break;
			}
		}
	}
	else
		choice_text_language_->Select(0);

	// Prevent undoing loading the entry
	text_area_->EmptyUndoBuffer();

	// Update variables
	this->entry = entry;
	setModified(false);

	return true;
}

/* TextEntryPanel::saveEntry
 * Saves any changes to the entry
 *******************************************************************/
bool TextEntryPanel::saveEntry()
{
	// Trim whitespace
	if (txed_trim_whitespace)
		text_area_->trimWhitespace();

	// Write raw text to the entry
	MemChunk mc;
	text_area_->getRawText(mc);
	if (mc.hasData())
		entry->importMemChunk(mc);
	else
		entry->clearData();

	if (entry->getState() == 0)
		entry->setState(1);

	// Re-detect entry type
	EntryType::detectEntryType(entry);

	// Set text if unknown
	if (entry->getType() == EntryType::unknownType())
		entry->setType(EntryType::fromId("text"));

	// Update custom definitions if decorate or zscript
	if (text_area_->language() &&
		(text_area_->language()->id() == "decorate" || text_area_->language()->id() == "zscript"))
		Game::updateCustomDefinitions();

	// Update variables
	setModified(false);

	return true;
}

/* TextEntryPanel::refreshPanel
 * Updates the text editor options and redraws it
 *******************************************************************/
void TextEntryPanel::refreshPanel()
{
	// Update text editor
	text_area_->setup();
	text_area_->Refresh();

	Refresh();
	Update();
}

/* TextEntryPanel::closeEntry
 * Performs any actions required on closing the entry
 *******************************************************************/
void TextEntryPanel::closeEntry()
{
	// Check any entry is open
	if (!entry)
		return;

	// Save current caret position
	entry->exProp("TextPosition") = text_area_->GetCurrentPos();
}

/* TextEntryPanel::statusString
 * Returns a string with extended editing/entry info for the status
 * bar
 *******************************************************************/
string TextEntryPanel::statusString()
{
	// Setup status string
	int line = text_area_->GetCurrentLine()+1;
	int pos = text_area_->GetCurrentPos();
	int col = text_area_->GetColumn(pos)+1;
	string status = S_FMT("Ln %d, Col %d, Pos %d", line, col, pos);

	return status;
}

/* TextEntryPanel::undo
 * Tells the text editor to undo
 *******************************************************************/
bool TextEntryPanel::undo()
{
	if (text_area_->CanUndo())
	{
		text_area_->Undo();
		// If we have undone all the way back, it is not modified anymore
		if (!text_area_->CanUndo())
			setModified(false);
		return true;
	}
	return false;
}

/* TextEntryPanel::redo
 * Tells the text editor to redo
 *******************************************************************/
bool TextEntryPanel::redo()
{
	if (text_area_->CanRedo())
	{
		text_area_->Redo();
		return true;
	}
	return false;
}

/* TextEntryPanel::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool TextEntryPanel::handleAction(string id)
{
	// Don't handle actions if hidden
	if (!isActivePanel())
		return false;

	// Jump To Line
	if (id == "ptxt_jump_to_line")
		text_area_->jumpToLine();

	// Find+Replace
	else if (id == "ptxt_find_replace")
		text_area_->showFindReplacePanel();

	// Word Wrapping toggle
	else if (id == "ptxt_wrap")
	{
		SAction* action = SAction::fromId("ptxt_wrap");
		bool m = isModified();
		if (action->isChecked())
			text_area_->SetWrapMode(wxSTC_WRAP_WORD);
		else
			text_area_->SetWrapMode(wxSTC_WRAP_NONE);
		setModified(m);
	}

	// Fold All
	else if (id == "ptxt_fold_foldall")
		text_area_->foldAll(true);

	// Unfold All
	else if (id == "ptxt_fold_unfoldall")
		text_area_->foldAll(false);

	// compileACS
	else if (id == "arch_scripts_compileacs")
		EntryOperations::compileACS(entry, false, nullptr, nullptr);

	else if (id == "arch_scripts_compilehacs")
		EntryOperations::compileACS(entry, true, nullptr, nullptr);

	// Not handled
	else
		return false;

	return true;
}


/*******************************************************************
 * TEXTENTRYPANEL CLASS EVENTS
 *******************************************************************/

/* TextEntryPanel::onTextModified
 * Called when the text in the TextEditor is modified
 *******************************************************************/
void TextEntryPanel::onTextModified(wxCommandEvent& e)
{
	if (!isModified() && text_area_->CanUndo())
		setModified();
	e.Skip();
}

/* TextEntryPanel::onChoiceLanguageChanged
 * Called when the language in the dropdown is changed
 *******************************************************************/
void TextEntryPanel::onChoiceLanguageChanged(wxCommandEvent& e)
{
	int index = choice_text_language_->GetSelection();
	// Get selected language
	TextLanguage* tl = TextLanguage::fromName(choice_text_language_->GetStringSelection());

	// Set text editor language
	text_area_->setLanguage(tl);

	// Set entry language hint
	if (tl)
		entry->exProp("TextLanguage") = tl->id();
	else
		entry->exProps().removeProperty("TextLanguage");
}

/* TextEntryPanel::onUpdateUI
 * Called when the text editor UI is updated
 *******************************************************************/
void TextEntryPanel::onUpdateUI(wxStyledTextEvent& e)
{
	updateStatus();
	e.Skip();
}
