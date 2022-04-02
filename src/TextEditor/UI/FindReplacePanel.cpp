
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "FindReplacePanel.h"
#include "General/KeyBind.h"
#include "TextEditorCtrl.h"
#include "UI/Controls/SIconButton.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// FindReplacePanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// FindReplacePanel class constructor
// -----------------------------------------------------------------------------
FindReplacePanel::FindReplacePanel(wxWindow* parent, TextEditorCtrl& text_editor) :
	wxPanel(parent, -1),
	text_editor_(text_editor)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	auto gb_sizer = new wxGridBagSizer(ui::pad(), ui::pad());
	GetSizer()->Add(gb_sizer, 1, wxEXPAND | wxBOTTOM, ui::pad());

	// Find
	text_find_     = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	btn_find_next_ = new wxButton(this, -1, "Find Next");
	btn_find_prev_ = new wxButton(this, -1, "Find Previous");
	btn_close_     = new SIconButton(this, "close", "Close");
	gb_sizer->Add(new wxStaticText(this, -1, "Find What:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_find_, { 0, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxEXPAND);
	gb_sizer->Add(btn_find_next_, { 0, 2 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(btn_find_prev_, { 0, 3 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(btn_close_, { 0, 4 }, { 1, 1 }, wxEXPAND);

	// Replace
	text_replace_    = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	btn_replace_     = new wxButton(this, -1, "Replace");
	btn_replace_all_ = new wxButton(this, -1, "Replace All");
	gb_sizer->Add(new wxStaticText(this, -1, "Replace With:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_replace_, { 1, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxEXPAND);
	gb_sizer->Add(btn_replace_, { 1, 2 }, { 1, 1 }, wxEXPAND);
	gb_sizer->Add(btn_replace_all_, { 1, 3 }, { 1, 1 }, wxEXPAND);

	// Options
	cb_match_case_       = new wxCheckBox(this, -1, "Match Case");
	cb_match_word_whole_ = new wxCheckBox(this, -1, "Match Word (Whole)");
	cb_match_word_start_ = new wxCheckBox(this, -1, "Match Word (Start)");
	cb_search_regex_     = new wxCheckBox(this, -1, "Regular Expression");
	cb_allow_escape_     = new wxCheckBox(this, -1, "Allow Backslash Expressions");
	auto wsizer          = new wxWrapSizer(wxHORIZONTAL, wxREMOVE_LEADING_SPACES);
	GetSizer()->Add(wsizer, 0, wxEXPAND);
	wsizer->Add(cb_match_case_, 0, wxEXPAND);
	wsizer->AddSpacer(ui::pad());
	wsizer->Add(cb_match_word_whole_, 0, wxEXPAND);
	wsizer->AddSpacer(ui::pad());
	wsizer->Add(cb_match_word_start_, 0, wxEXPAND);
	wsizer->AddSpacer(ui::pad());
	wsizer->Add(cb_search_regex_, 0, wxEXPAND);
	wsizer->AddSpacer(ui::pad());
	wsizer->Add(cb_allow_escape_, 0, wxEXPAND);

	gb_sizer->AddGrowableCol(1, 1);


	// Bind events
	// -------------------------------------------------------------------------

	// Find Next button clicked
	btn_find_next_->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { text_editor_.findNext(findText(), findFlags()); });

	// Find Previous button clicked
	btn_find_prev_->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { text_editor_.findPrev(findText(), findFlags()); });

	// Close button clicked
	btn_close_->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) { text_editor_.showFindReplacePanel(false); });

	// Replace button clicked
	btn_replace_->Bind(
		wxEVT_BUTTON, [&](wxCommandEvent& e) { text_editor_.replaceCurrent(findText(), replaceText(), findFlags()); });

	// Replace All button clicked
	btn_replace_all_->Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) {
		auto n_replaced = text_editor_.replaceAll(findText(), replaceText(), findFlags());
		wxMessageBox(wxString::Format("Replaced %d occurrence(s)", n_replaced), "Replace All");
	});

	// Enter pressed in find text box
	text_find_->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& e) {
		if (wxGetKeyState(WXK_SHIFT))
			text_editor_.findPrev(findText(), findFlags());
		else
			text_editor_.findNext(findText(), findFlags());
	});

	// Enter pressed in replace text box
	text_replace_->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& e) {
		text_editor_.replaceCurrent(findText(), replaceText(), findFlags());
	});

	Bind(wxEVT_CHAR_HOOK, &FindReplacePanel::onKeyDown, this);


	// Set tab order
	text_replace_->MoveAfterInTabOrder(text_find_);

	Layout();
	Fit();
}

// -----------------------------------------------------------------------------
// Sets the 'Find' text to [find], selects all and focuses the text box
// -----------------------------------------------------------------------------
void FindReplacePanel::setFindText(const wxString& find) const
{
	text_find_->SetFocus();
	text_find_->SetValue(find);
	text_find_->SelectAll();
}

// -----------------------------------------------------------------------------
// Returns the current 'Find' text
// -----------------------------------------------------------------------------
wxString FindReplacePanel::findText() const
{
	wxString find = text_find_->GetValue();

	if (cb_allow_escape_->GetValue())
	{
		find.Replace("\\n", "\n");
		find.Replace("\\r", "\r");
		find.Replace("\\t", "\t");
	}

	return find;
}

// -----------------------------------------------------------------------------
// Returns the selected search options
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Returns the current 'Replace' text
// -----------------------------------------------------------------------------
wxString FindReplacePanel::replaceText() const
{
	wxString replace = text_replace_->GetValue();

	if (cb_allow_escape_->GetValue())
	{
		replace.Replace("\\n", "\n");
		replace.Replace("\\r", "\r");
		replace.Replace("\\t", "\t");
	}

	return replace;
}


// -----------------------------------------------------------------------------
//
// FindReplacePanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a key is pressed while the panel has focus
// -----------------------------------------------------------------------------
void FindReplacePanel::onKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	auto binds = KeyBind::bindsForKey(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	bool handled = false;
	for (const auto& name : binds)
	{
		// Find next
		if (name == "ted_findnext")
		{
			text_editor_.findNext(findText(), findFlags());
			handled = true;
		}

		// Find previous
		else if (name == "ted_findprev")
		{
			text_editor_.findPrev(findText(), findFlags());
			handled = true;
		}

		// Replace next
		else if (name == "ted_replacenext")
		{
			text_editor_.replaceCurrent(findText(), replaceText(), findFlags());
			handled = true;
		}

		// Replace all
		else if (name == "ted_replaceall")
		{
			text_editor_.replaceAll(findText(), replaceText(), findFlags());
			handled = true;
		}
	}

	if (!handled)
	{
		// Esc = close panel
		if (e.GetKeyCode() == WXK_ESCAPE)
			text_editor_.showFindReplacePanel(false);
		else
			e.Skip();
	}
}
