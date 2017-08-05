
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    FindReplacePanel.cpp
// Description: Find & Replace UI panel for the text editor
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "FindReplacePanel.h"
#include "General/KeyBind.h"
#include "TextEditorCtrl.h"


// ----------------------------------------------------------------------------
//
// FindReplacePanel Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// FindReplacePanel::FindReplacePanel
//
// FindReplacePanel class constructor
// ----------------------------------------------------------------------------
FindReplacePanel::FindReplacePanel(wxWindow* parent, TextEditorCtrl* text_editor)
	: wxPanel(parent, -1), text_editor_(text_editor)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	sizer->Add(gb_sizer, 1, wxEXPAND | wxBOTTOM, 4);

	// Find
	text_find_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	btn_find_next_ = new wxButton(this, -1, "Find Next");
	btn_find_prev_ = new wxButton(this, -1, "Find Previous");
	gb_sizer->Add(new wxStaticText(this, -1, "Find What:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_find_, wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxEXPAND);
	gb_sizer->Add(btn_find_next_, wxGBPosition(0, 2), wxDefaultSpan, wxEXPAND);
	gb_sizer->Add(btn_find_prev_, wxGBPosition(0, 3), wxDefaultSpan, wxEXPAND);

	// Replace
	text_replace_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	btn_replace_ = new wxButton(this, -1, "Replace");
	btn_replace_all_ = new wxButton(this, -1, "Replace All");
	gb_sizer->Add(new wxStaticText(this, -1, "Replace With:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_replace_, wxGBPosition(1, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
	gb_sizer->Add(btn_replace_, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND);
	gb_sizer->Add(btn_replace_all_, wxGBPosition(1, 3), wxDefaultSpan, wxEXPAND);

	// Options
	cb_match_case_ = new wxCheckBox(this, -1, "Match Case");
	cb_match_word_whole_ = new wxCheckBox(this, -1, "Match Word (Whole)");
	cb_match_word_start_ = new wxCheckBox(this, -1, "Match Word (Start)");
	cb_search_regex_ = new wxCheckBox(this, -1, "Regular Expression");
	cb_allow_escape_ = new wxCheckBox(this, -1, "Allow Backslash Expressions");
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND);
	hbox->Add(cb_match_case_, 0, wxEXPAND | wxRIGHT, 4);
	hbox->Add(cb_match_word_whole_, 0, wxEXPAND | wxRIGHT, 4);
	hbox->Add(cb_match_word_start_, 0, wxEXPAND | wxRIGHT, 4);
	hbox->Add(cb_search_regex_, 0, wxEXPAND | wxRIGHT, 4);
	hbox->Add(cb_allow_escape_, 0, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);

	// Bind events

	// Find Next button clicked
	btn_find_next_->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e)
	{
		text_editor_->findNext(findText(), findFlags());
	});

	// Find Previous button clicked
	btn_find_prev_->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e)
	{
		text_editor_->findPrev(findText(), findFlags());
	});

	// Replace button clicked
	btn_replace_->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e)
	{
		text_editor_->replaceCurrent(findText(), replaceText(), findFlags());
	});

	// Replace All button clicked
	btn_replace_all_->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e)
	{
		text_editor_->replaceAll(findText(), replaceText(), findFlags());
	});

	// Enter pressed in find text box
	text_find_->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& e)
	{
		if (wxGetKeyState(WXK_SHIFT))
			text_editor_->findPrev(findText(), findFlags());
		else
			text_editor_->findNext(findText(), findFlags());
	});
	
	// Enter pressed in replace text box
	text_replace_->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& e)
	{
		text_editor_->replaceCurrent(findText(), replaceText(), findFlags());
	});

	Bind(wxEVT_CHAR_HOOK, &FindReplacePanel::onKeyDown, this);

	// Set tab order
	text_replace_->MoveAfterInTabOrder(text_find_);
}

// ----------------------------------------------------------------------------
// FindReplacePanel::~FindReplacePanel
//
// FindReplacePanel class destructor
// ----------------------------------------------------------------------------
FindReplacePanel::~FindReplacePanel()
{
}

// ----------------------------------------------------------------------------
// FindReplacePanel::setFindText
//
// Sets the 'Find' text to [find], selects all and focuses the text box
// ----------------------------------------------------------------------------
void FindReplacePanel::setFindText(string find) const
{
	text_find_->SetFocus();
	text_find_->SetValue(find);
	text_find_->SelectAll();
}

// ----------------------------------------------------------------------------
// FindReplacePanel::getFindText
//
// Returns the current 'Find' text
// ----------------------------------------------------------------------------
string FindReplacePanel::findText() const
{
	string find = text_find_->GetValue();

	if (cb_allow_escape_->GetValue())
	{
		find.Replace("\\n", "\n");
		find.Replace("\\r", "\r");
		find.Replace("\\t", "\t");
	}

	return find;
}

// ----------------------------------------------------------------------------
// FindReplacePanel::getFindFlags
//
// Returns the selected search options
// ----------------------------------------------------------------------------
int FindReplacePanel::findFlags() const
{
	int flags = 0;
	if (cb_match_case_->GetValue())
		flags |= wxSTC_FIND_MATCHCASE;
	if (cb_match_word_start_->GetValue())
		flags |= wxSTC_FIND_WORDSTART;
	if (cb_match_word_whole_->GetValue())
		flags |= wxSTC_FIND_WHOLEWORD;
	if (cb_search_regex_->GetValue())
		flags |= wxSTC_FIND_REGEXP;

	return flags;
}

// ----------------------------------------------------------------------------
// FindReplacePanel::getReplaceText
//
// Returns the current 'Replace' text
// ----------------------------------------------------------------------------
string FindReplacePanel::replaceText() const
{
	string replace = text_replace_->GetValue();

	if (cb_allow_escape_->GetValue())
	{
		replace.Replace("\\n", "\n");
		replace.Replace("\\r", "\r");
		replace.Replace("\\t", "\t");
	}

	return replace;
}


// ----------------------------------------------------------------------------
//
// FindReplacePanel Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// FindReplacePanel::onKeyDown
//
// Called when a key is pressed while the panel has focus
// ----------------------------------------------------------------------------
void FindReplacePanel::onKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	wxArrayString binds = KeyBind::getBinds(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	bool handled = false;
	for (unsigned a = 0; a < binds.size(); a++)
	{
		string name = binds[a];

		// Find next
		if (name == "ted_findnext")
		{
			text_editor_->findNext(findText(), findFlags());
			handled = true;
		}

		// Find previous
		else if (name == "ted_findprev")
		{
			text_editor_->findPrev(findText(), findFlags());
			handled = true;
		}

		// Replace next
		else if (name == "ted_replacenext")
		{
			text_editor_->replaceCurrent(findText(), replaceText(), findFlags());
			handled = true;
		}

		// Replace all
		else if (name == "ted_replaceall")
		{
			text_editor_->replaceAll(findText(), replaceText(), findFlags());
			handled = true;
		}
	}

	if (!handled)
	{
		// Esc = close panel
		if (e.GetKeyCode() == WXK_ESCAPE)
			text_editor_->showFindReplacePanel(false);
		else
			e.Skip();
	}
}
