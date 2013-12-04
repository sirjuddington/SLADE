
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
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
#include "WxStuff.h"
#include "TextEntryPanel.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Bool, txed_trim_whitespace, false, CVAR_SAVE)
wxArrayString languages;


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
	text_area = new TextEditor(this, -1);
	sizer_main->Add(text_area, 1, wxEXPAND, 0);

	// Add 'Text Language' choice to toolbar
	SToolBarGroup* group_language = new SToolBarGroup(toolbar, "Text Language", true);
	languages = TextLanguage::getLanguageNames();
	languages.Sort();
	languages.Insert("None", 0, 1);
	choice_text_language = new wxChoice(group_language, -1, wxDefaultPosition, wxDefaultSize, languages);
	choice_text_language->Select(0);
	group_language->addCustomControl(choice_text_language);
	toolbar->addGroup(group_language);

	// Add 'Word Wrap' checkbox to top sizer
	sizer_bottom->AddStretchSpacer();
	cb_wordwrap = new wxCheckBox(this, -1, "Word Wrapping");
	sizer_bottom->Add(cb_wordwrap, 0, wxEXPAND, 0);

	// Add 'Jump To' button to top sizer
	btn_jump_to = new wxButton(this, -1, "Jump To");
	sizer_bottom->Add(btn_jump_to, 0, wxEXPAND|wxRIGHT, 4);

	// Add 'Find/Replace' button to top sizer
	btn_find_replace = new wxButton(this, -1, "Find + Replace");
	sizer_bottom->Add(btn_find_replace, 0, wxEXPAND, 0);

	// Bind events
	choice_text_language->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &TextEntryPanel::onChoiceLanguageChanged, this);
	text_area->Bind(wxEVT_STC_CHANGE, &TextEntryPanel::onTextModified, this);
	btn_find_replace->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &TextEntryPanel::onBtnFindReplace, this);
	text_area->Bind(wxEVT_STC_UPDATEUI, &TextEntryPanel::onUpdateUI, this);
	cb_wordwrap->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &TextEntryPanel::onWordWrapChanged, this);
	btn_jump_to->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &TextEntryPanel::onBtnJumpTo, this);

	// Custom toolbar
	custom_toolbar_actions = "arch_scripts_compileacs;arch_scripts_compilehacs";
	toolbar->addActionGroup("Scripts", wxSplit(custom_toolbar_actions, ';'));

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
	if (!text_area->loadEntry(entry))
		return false;

	// Scroll to previous position (if any)
	if (entry->exProps().propertyExists("TextPosition"))
		text_area->GotoPos((int)(entry->exProp("TextPosition")));

	// --- Attempt to determine text language ---
	TextLanguage* tl = NULL;

	// Level markers use FraggleScript
	if (entry->getType() == EntryType::mapMarkerType())
	{
		tl = TextLanguage::getLanguage("fragglescript");
	}

	// From entry language hint
	if (entry->exProps().propertyExists("TextLanguage"))
	{
		string lang_id = entry->exProp("TextLanguage");
		tl = TextLanguage::getLanguage(lang_id);
	}

	// Or, from entry type
	if (!tl && entry->getType()->extraProps().propertyExists("text_language"))
	{
		string lang_id = entry->getType()->extraProps()["text_language"];
		tl = TextLanguage::getLanguage(lang_id);
	}

	// Or, from entry's parent directory
	if (!tl)
	{
		// ZDoom DECORATE (within 'actors' or 'decorate' directories)
		if (S_CMPNOCASE(wxString("/actors/"), entry->getPath().Left(8)) ||
		        S_CMPNOCASE(wxString("/decorate/"), entry->getPath().Left(10)))
			tl = TextLanguage::getLanguage("decorate");
	}

	// Load language
	text_area->setLanguage(tl);

	// Select it in the choice box
	if (tl)
	{
		for (unsigned a = 0; a < languages.size(); a++)
		{
			if (S_CMPNOCASE(tl->getName(), languages[a]))
			{
				choice_text_language->Select(a);
				break;
			}
		}
	}
	else
		choice_text_language->Select(0);

	// Prevent undoing loading the entry
	text_area->EmptyUndoBuffer();

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
		text_area->trimWhitespace();

	// Write raw text to the entry
	text_area->getRawText(entry->getMCData());
	if (entry->getState() == 0)
		entry->setState(1);

	// Re-detect entry type
	EntryType::detectEntryType(entry);

	// Set text if unknown
	if (entry->getType() == EntryType::unknownType())
		entry->setType(EntryType::getType("text"));

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
	text_area->setup();

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
	entry->exProp("TextPosition") = text_area->GetCurrentPos();
}

/* TextEntryPanel::statusString
 * Returns a string with extended editing/entry info for the status
 * bar
 *******************************************************************/
string TextEntryPanel::statusString()
{
	// Setup status string
	int line = text_area->GetCurrentLine()+1;
	int pos = text_area->GetCurrentPos();
	int col = text_area->GetColumn(pos)+1;
	string status = S_FMT("Ln %d, Col %d, Pos %d", line, col, pos);

	return status;
}

/* TextEntryPanel::undo
 * Tells the text editor to undo
 *******************************************************************/
bool TextEntryPanel::undo()
{
	if (text_area->CanUndo())
	{
		text_area->Undo();
		// If we have undone all the way back, it is not modified anymore
		if (!text_area->CanUndo())
		{
			setModified(false);
		}
		return true;
	}
	return false;
}

/* TextEntryPanel::redo
 * Tells the text editor to redo
 *******************************************************************/
bool TextEntryPanel::redo()
{
	if (text_area->CanRedo())
	{
		text_area->Redo();
		return true;
	}
	return false;
}


/*******************************************************************
 * TEXTENTRYPANEL CLASS EVENTS
 *******************************************************************/

/* TextEntryPanel::onTextModified
 * Called when the text in the TextEditor is modified
 *******************************************************************/
void TextEntryPanel::onTextModified(wxStyledTextEvent& e)
{
	setModified();
}

/* TextEntryPanel::onBtnFindReplace
 * Called when the 'Find+Replace' button is clicked
 *******************************************************************/
void TextEntryPanel::onBtnFindReplace(wxCommandEvent& e)
{
	text_area->showFindReplaceDialog();
}

/* TextEntryPanel::onChoiceLanguageChanged
 * Called when the language in the dropdown is changed
 *******************************************************************/
void TextEntryPanel::onChoiceLanguageChanged(wxCommandEvent& e)
{
	int index = choice_text_language->GetSelection();
	// Get selected language
	TextLanguage* tl = TextLanguage::getLanguageByName(choice_text_language->GetStringSelection());

	// Set text editor language
	text_area->setLanguage(tl);

	// Set entry language hint
	if (tl)
		entry->exProp("TextLanguage") = tl->getId();
	else
		entry->exProps().removeProperty("TextLanguage");
}

/* TextEntryPanel::onWordWrapChanged
 * Called when the "Word Wrap" checkbox is clicked
 *******************************************************************/
void TextEntryPanel::onWordWrapChanged(wxCommandEvent& e)
{
	bool m = isModified();
	if (cb_wordwrap->IsChecked())
		text_area->SetWrapMode(wxSTC_WRAP_WORD);
	else
		text_area->SetWrapMode(wxSTC_WRAP_NONE);
	setModified(m);
}

/* TextEntryPanel::onUpdateUI
 * Called when the text editor UI is updated
 *******************************************************************/
void TextEntryPanel::onUpdateUI(wxStyledTextEvent& e)
{
	updateStatus();
	e.Skip();
}

/* TextEntryPanel::onBtnJumpTo
 * Called when the 'Jump To' button is clicked
 *******************************************************************/
void TextEntryPanel::onBtnJumpTo(wxCommandEvent& e)
{
	text_area->openJumpToDialog();
}
