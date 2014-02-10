
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TextEditor.cpp
 * Description: The SLADE Text Editor control, does syntax
 *              highlighting, calltips, autocomplete and more,
 *              using an associated TextLanguage
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
#include "TextEditor.h"
#include "Icons.h"
#include "TextStyle.h"
#include "KeyBind.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, txed_tab_width, 4, CVAR_SAVE)
CVAR(Bool, txed_auto_indent, true, CVAR_SAVE)
CVAR(Bool, txed_syntax_hilight, true, CVAR_SAVE)
CVAR(Bool, txed_brace_match, false, CVAR_SAVE)
CVAR(Int, txed_edge_column, 80, CVAR_SAVE)
CVAR(Bool, txed_indent_guides, false, CVAR_SAVE)
CVAR(String, txed_style_set, "SLADE Default", CVAR_SAVE)
CVAR(Bool, txed_calltips_mouse, true, CVAR_SAVE)
CVAR(Bool, txed_calltips_parenthesis, true, CVAR_SAVE)
rgba_t col_edge_line(200, 200, 230, 255);


/*******************************************************************
 * FINDREPLACEDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* FindReplaceDialog::FindReplaceDialog
 * FindReplaceDialog class constructor
 *******************************************************************/
FindReplaceDialog::FindReplaceDialog(wxWindow* parent) : wxMiniFrame(parent, -1, "Find + Replace", wxDefaultPosition, wxDefaultSize, wxCAPTION|wxCLOSE_BOX|wxFRAME_FLOAT_ON_PARENT)
{
	// Create backing panel
	wxPanel* panel = new wxPanel(this, -1);
	wxBoxSizer* fsizer = new wxBoxSizer(wxVERTICAL);
	fsizer->Add(panel, 1, wxEXPAND);
	SetSizer(fsizer);

	// Create/set dialog sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);


	// 'Find' text entry
	sizer->Add(new wxStaticText(panel, -1, "Find:"), 0, wxTOP|wxLEFT|wxRIGHT, 4);
	text_find = new wxTextCtrl(panel, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	sizer->Add(text_find, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Find options checkboxes
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	sizer->AddSpacer(4);
	hbox->AddStretchSpacer(1);

	// 'Match Case' checkbox
	cb_match_case = new wxCheckBox(panel, -1, "Match Case");
	hbox->Add(cb_match_case, 0, wxEXPAND|wxRIGHT, 4);

	// 'Match Whole Word' checkbox
	cb_match_word = new wxCheckBox(panel, -1, "Match Whole Word");
	hbox->Add(cb_match_word, 0, wxEXPAND);


	// 'Replace With' text entry
	sizer->Add(new wxStaticText(panel, -1, "Replace With:"), 0, wxTOP|wxLEFT|wxRIGHT, 4);
	text_replace = new wxTextCtrl(panel, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	sizer->Add(text_replace, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);


	// Buttons
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->AddStretchSpacer(1);

	// 'Find Next'
	btn_find_next = new wxButton(panel, -1, "Find Next");
	hbox->Add(btn_find_next, 0, wxEXPAND|wxRIGHT, 4);

	// 'Replace'
	btn_replace = new wxButton(panel, -1, "Replace");
	hbox->Add(btn_replace, 0, wxEXPAND|wxRIGHT, 4);

	// 'Replace All'
	btn_replace_all = new wxButton(panel, -1, "Replace All");
	hbox->Add(btn_replace_all, 0, wxEXPAND);


	// Bind events
	//Bind(wxEVT_CHAR_HOOK, &FindReplaceDialog::onKeyDown, this);
	Bind(wxEVT_CLOSE_WINDOW, &FindReplaceDialog::onClose, this);


	// Init layout
	Layout();
	SetInitialSize(wxSize(400, -1));
	Fit();
}

/* FindReplaceDialog::~FindReplaceDialog
 * FindReplaceDialog class destructor
 *******************************************************************/
FindReplaceDialog::~FindReplaceDialog()
{
}

/* FindReplaceDialog::onClose
 * Called when the frame close button is clicked
 *******************************************************************/
void FindReplaceDialog::onClose(wxCloseEvent& e)
{
	Show(false);
	m_parent->SetFocus();
}

/* FindReplaceDialog::onKeyDown
 * Called when a key is pressed
 *******************************************************************/
void FindReplaceDialog::onKeyDown(wxKeyEvent& e)
{
	// Check for ESC key
	if (e.GetKeyCode() == WXK_ESCAPE)
		Close();
	else
		e.Skip();
}


/*******************************************************************
 * TEXTEDITOR CLASS FUNCTIONS
 *******************************************************************/

/* TextEditor::TextEditor
 * TextEditor class constructor
 *******************************************************************/
TextEditor::TextEditor(wxWindow* parent, int id)
	: wxStyledTextCtrl(parent, id)
{
	// Init variables
	language = NULL;
	ct_argset = 0;
	ct_function = NULL;
	ct_start = 0;

	// Set tab width
	SetTabWidth(txed_tab_width);

	// Line numbers by default
	SetMarginType(0, wxSTC_MARGIN_NUMBER);
	SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, "9999"));
	SetMarginWidth(1, 4);

	// Register icons for autocompletion list
	RegisterImage(1, getIcon("ac_key"));
	RegisterImage(2, getIcon("ac_const"));
	RegisterImage(3, getIcon("ac_func"));

	// Init w/no language
	setLanguage(NULL);

	// Find+Replace dialog
	dlg_fr = new FindReplaceDialog(this);

	// Setup various configurable properties
	setup();

	// Bind events
	Bind(wxEVT_KEY_DOWN, &TextEditor::onKeyDown, this);
	Bind(wxEVT_KEY_UP, &TextEditor::onKeyUp, this);
	Bind(wxEVT_STC_CHARADDED, &TextEditor::onCharAdded, this);
	Bind(wxEVT_STC_UPDATEUI, &TextEditor::onUpdateUI, this);
	Bind(wxEVT_STC_CALLTIP_CLICK, &TextEditor::onCalltipClicked, this);
	Bind(wxEVT_STC_DWELLSTART, &TextEditor::onMouseDwellStart, this);
	Bind(wxEVT_STC_DWELLEND, &TextEditor::onMouseDwellEnd, this);
	Bind(wxEVT_LEFT_DOWN, &TextEditor::onMouseDown, this);
	Bind(wxEVT_KILL_FOCUS, &TextEditor::onFocusLoss, this);
	dlg_fr->getBtnFindNext()->Bind(wxEVT_BUTTON, &TextEditor::onFRDBtnFindNext, this);
	dlg_fr->getBtnReplace()->Bind(wxEVT_BUTTON, &TextEditor::onFRDBtnReplace, this);
	dlg_fr->getBtnReplaceAll()->Bind(wxEVT_BUTTON, &TextEditor::onFRDBtnReplaceAll, this);
	//dlg_fr->getTextFind()->Bind(wxEVT_BUTTON, &TextEditor::onFRDBtnFindNext, this);
	//dlg_fr->getTextReplace()->Bind(wxEVT_BUTTON, &TextEditor::onFRDBtnReplace, this);
	dlg_fr->Bind(wxEVT_CHAR_HOOK, &TextEditor::onFRDKeyDown, this);
}

/* TextEditor::~TextEditor
 * TextEditor class destructor
 *******************************************************************/
TextEditor::~TextEditor()
{
}

/* TextEditor::setup
 * Sets up text editor properties depending on cvars and the current
 * text styleset/style
 *******************************************************************/
void TextEditor::setup()
{
	// General settings
	SetBufferedDraw(true);
	SetUseAntiAliasing(true);
	SetMouseDwellTime(500);
	AutoCompSetIgnoreCase(true);
	SetIndentationGuides(txed_indent_guides);
	StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, WXCOL(col_edge_line));

	// Right margin line
	SetEdgeColour(WXCOL(col_edge_line));
	SetEdgeColumn(txed_edge_column);
	if (txed_edge_column == 0)
		SetEdgeMode(wxSTC_EDGE_NONE);
	else
		SetEdgeMode(wxSTC_EDGE_LINE);

	// Apply default style
	StyleSet::applyCurrent(this);
	CallTipUseStyle(10);
	StyleSetChangeable(wxSTC_STYLE_CALLTIP, true);
	wxFont font_ct(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	StyleSetFont(wxSTC_STYLE_CALLTIP, font_ct);
	CallTipSetBackground(wxColour(255, 255, 180));
	CallTipSetForeground(wxColour(0, 0, 0));
	CallTipSetForegroundHighlight(wxColour(0, 0, 200));

	// Set lexer
	if (txed_syntax_hilight)
		SetLexer(wxSTC_LEX_CPPNOCASE);
	else
		SetLexer(wxSTC_LEX_NULL);

	// Re-colour text
	Colourise(0, GetTextLength());
}

/* TextEditor::setLanguage
 * Sets the text editor language
 *******************************************************************/
bool TextEditor::setLanguage(TextLanguage* lang)
{
	// Check language was given
	if (!lang)
	{
		// Clear keywords
		SetKeyWords(0, "");
		SetKeyWords(1, "");
		SetKeyWords(2, "");
		SetKeyWords(3, "");

		// Clear autocompletion list
		autocomp_list.Clear();
	}

	// Setup syntax hilighting if needed
	else
	{
		// Load word lists
		SetKeyWords(0, lang->getKeywordsList().Lower());
		SetKeyWords(1, lang->getFunctionsList().Lower());
		SetKeyWords(2, lang->getConstantsList().Lower());
		SetKeyWords(3, lang->getConstantsList().Lower());

		// Load autocompletion list
		autocomp_list = lang->getAutocompletionList();
	}

	// Set lexer
	if (txed_syntax_hilight)
		SetLexer(wxSTC_LEX_CPPNOCASE);
	else
		SetLexer(wxSTC_LEX_NULL);

	// Update variables
	SetWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.$");
	this->language = lang;

	// Re-colour text
	Colourise(0, GetTextLength());

	return true;
}

/* TextEditor::applyStyleSet
 * Applies the styleset [style] to the text editor
 *******************************************************************/
bool TextEditor::applyStyleSet(StyleSet* style)
{
	// Check if one was given
	if (!style)
		return false;

	// Apply it
	style->applyTo(this);

	return true;
}

/* TextEditor::loadEntry
 * Reads the contents of [entry] into the text area, returns false
 * if the given entry is invalid
 *******************************************************************/
bool TextEditor::loadEntry(ArchiveEntry* entry)
{
	// Clear current text
	ClearAll();

	// Check that the entry exists
	if (!entry)
	{
		Global::error = "Invalid archive entry given";
		return false;
	}

	// Check that the entry has any data, if not do nothing
	if (entry->getSize() == 0 || !entry->getData())
		return true;

	// Get character entry data
	//string text = wxString::From8BitData((const char*)entry->getData(), entry->getSize());
	string text = wxString::FromUTF8((const char*)entry->getData(), entry->getSize());
	// If opening as UTF8 failed for some reason, try again as 8-bit data
	if (text.length() == 0)
		text = wxString::From8BitData((const char*)entry->getData(), entry->getSize());

	// Load text into editor
	SetText(text);

	// Update line numbers margin width
	string numlines = S_FMT("0%d", GetNumberOfLines());
	SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, numlines));

	return true;
}

/* TextEditor::getRawText
 * Writes the raw ASCII text to [mc]
 *******************************************************************/
void TextEditor::getRawText(MemChunk& mc)
{
	mc.clear();
	string text = GetText();
	bool result = mc.importMem((const uint8_t*)text.ToUTF8().data(), text.ToUTF8().length());
}

/* TextEditor::trimWhitespace
 * Removes any unneeded whitespace from the ends of lines
 *******************************************************************/
void TextEditor::trimWhitespace()
{
	// Go through lines
	for (int a = 0; a < GetLineCount(); a++)
	{
		// Get line start and end positions
		int pos = GetLineEndPosition(a) - 1;
		int start = pos - GetLineLength(a);

		while (pos > start)
		{
			int chr = GetCharAt(pos);

			// Check for whitespace character
			if (chr == ' ' || chr == '\t')
			{
				// Remove character if whitespace
				Remove(pos, pos+1);
				pos--;
			}
			else
				break;	// Not whitespace, stop
		}
	}
}

/* TextEditor::findNext
 * Finds the next occurrence of the [find] after the caret position,
 * selects it and scrolls to it if needed. Returns false if the
 * [find] was invalid or no match was found, true otherwise
 *******************************************************************/
bool TextEditor::findNext(string find)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Setup target range
	SetTargetEnd(GetTextLength());
	SetTargetStart(GetSelectionEnd());

	// Search within current target range
	if (SearchInTarget(find) < 0)
	{
		// None found, search again from start
		SetTargetStart(0);
		SetTargetEnd(GetTextLength());
		if (SearchInTarget(find) < 0)
		{
			// No matches found in entire text
			return false;
		}
	}

	// Select matched text
	SetSelection(GetTargetStart(), GetTargetEnd());

	// Scroll to selection
	EnsureCaretVisible();

	return true;
}

/* TextEditor::replaceCurrent
 * Replaces the currently selected occurrence of [find] with
 * [replace], then selects and scrolls to the next occurrence of
 * [find] in the text. Returns false if [find] is invalid or the
 * current selection does not match it, true otherwise
 *******************************************************************/
bool TextEditor::replaceCurrent(string find, string replace)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Check that we've done a find previously
	// (by searching for the find string within the current selection)
	if (GetSelectedText().Length() != find.Length())
		return false;
	SetTargetStart(GetSelectionStart());
	SetTargetEnd(GetSelectionEnd());
	if (SearchInTarget(find) < 0)
		return false;

	// Do the replace
	ReplaceTarget(replace);

	// Update selection
	SetSelection(GetTargetStart(), GetTargetEnd());

	// Do find next
	findNext(find);

	return true;
}

/* TextEditor::replaceAll
 * Replaces all occurrences of [find] in the text with [replace].
 * Returns the number of occurrences replaced
 *******************************************************************/
int TextEditor::replaceAll(string find, string replace)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Init search target to entire text
	SetTargetStart(0);
	SetTargetEnd(GetTextLength());

	// Loop of death
	int replaced = 0;
	while (1)
	{
		if (SearchInTarget(find) < 0)
			break;	// No matches, finished
		else
		{
			// Replace text & increment counter
			ReplaceTarget(replace);
			replaced++;

			// Continue search from end of replaced text to end of text
			SetTargetStart(GetTargetEnd());
			SetTargetEnd(GetTextLength());
		}
	}

	// Return number of instances replaced
	return replaced;
}

/* TextEditor::checkBraceMatch
 * Checks for a brace match at the current cursor position
 *******************************************************************/
void TextEditor::checkBraceMatch()
{
#ifdef __WXMAC__
	bool refresh = false;
#else
	bool refresh = true;
#endif

	// Check for brace match at current position
	int bracematch = BraceMatch(GetCurrentPos());
	if (bracematch != wxSTC_INVALID_POSITION)
	{
		BraceHighlight(GetCurrentPos(), bracematch);
		if (refresh) Refresh();
		return;
	}

	// No match, check for match at previous position
	bracematch = BraceMatch(GetCurrentPos() - 1);
	if (bracematch != wxSTC_INVALID_POSITION)
	{
		BraceHighlight(GetCurrentPos() - 1, bracematch);
		if (refresh) Refresh();
		return;
	}

	// No match at all, clear any previous brace match
	BraceHighlight(-1, -1);
	if (refresh) Refresh();
}

/* TextEditor::openCalltip
 * Opens a calltip for the function name before [pos]. Returns false
 * if the word before [pos] was not a function name, true otherwise
 *******************************************************************/
bool TextEditor::openCalltip(int pos, int arg)
{
	// Don't bother if no language
	if (!language)
		return false;

	// Get start of word before bracket
	int start = WordStartPosition(pos - 1, false);

	// Get word before bracket
	string word = GetTextRange(WordStartPosition(start, true), WordEndPosition(start, true));

	// Get matching language function (if any)
	TLFunction* func = language->getFunction(word);

	// Show calltip if it's a function
	if (func && func->nArgSets() > 0)
	{
		CallTipShow(pos, func->generateCallTipString());
		ct_function = func;
		ct_argset = 0;
		ct_start = pos;

		// Highlight arg
		point2_t arg_ext = ct_function->getArgTextExtent(arg);
		CallTipSetHighlight(arg_ext.x, arg_ext.y);

		return true;
	}
	else
	{
		ct_function = NULL;
		return false;
	}
}

/* TextEditor::updateCalltip
 * Updates the current calltip, or attempts to open one if none is
 * currently showing
 *******************************************************************/
void TextEditor::updateCalltip()
{
	// Don't bother if no language
	if (!language)
		return;

	if (!CallTipActive())
	{
		// No calltip currently showing, check if we're in a function
		int pos = GetCurrentPos() - 1;
		while (pos >= 0)
		{
			// Get character
			int chr = GetCharAt(pos);

			// If we find a closing bracket, skip to matching brace
			if (chr == ')')
			{
				while (pos >= 0 && chr != '(')
				{
					pos--;
					chr = GetCharAt(pos);
				}
				pos--;
				continue;
			}

			// If we find an opening bracket, try to open a calltip
			if (chr == '(')
			{
				if (!openCalltip(pos))
					return;
				else
					break;
			}

			// Go to previous character
			pos--;
		}
	}

	if (ct_function)
	{
		// Calltip currently showing, determine what arg we're at
		int pos = ct_start+1;
		int arg = 0;
		while (pos < GetCurrentPos() && pos < GetTextLength())
		{
			// Get character
			int chr = GetCharAt(pos);

			// If it's an opening brace, skip until closing (ie skip a function as an arg)
			if (chr == '(')
			{
				while (chr != ')')
				{
					// Exit if we get to the current position or the end of the text
					if (pos == GetCurrentPos() || pos == GetTextLength()-1)
						break;

					// Get next character
					pos++;
					chr = GetCharAt(pos);
				}

				pos++;
				continue;
			}

			// If it's a comma, increment arg
			if (chr == ',')
				arg++;

			// If it's a closing brace, we're outside the function, so cancel the calltip
			if (chr == ')')
			{
				CallTipCancel();
				ct_function = NULL;
				return;
			}

			// Go to next character
			pos++;
		}

		// Update calltip string with the selected arg set and the current arg highlighted
		CallTipShow(ct_start, ct_function->generateCallTipString(ct_argset));
		point2_t arg_ext = ct_function->getArgTextExtent(arg, ct_argset);
		CallTipSetHighlight(arg_ext.x, arg_ext.y);
	}
}

/* TextEditor::openJumpToDialog
 * Initialises and opens the 'Jump To' dialog
 *******************************************************************/
void TextEditor::openJumpToDialog()
{
	// Can't do this without a language definition or defined blocks
	if (!language || language->nJumpBlocks() == 0)
		return;

	// --- Scan for functions/scripts ---
	Tokenizer tz;
	vector<jp_t> jump_points;
	tz.openString(GetText());

	string token = tz.getToken();
	while (!token.IsEmpty())
	{
		if (token == "{")
		{
			// Skip block
			while (!token.IsEmpty() && token != "}")
				token = tz.getToken();
		}

		for (unsigned a = 0; a < language->nJumpBlocks(); a++)
		{
			// Get jump block keyword
			string block = language->jumpBlock(a);
			long skip = 0;
			if (block.Contains(":"))
			{
				wxArrayString sp = wxSplit(block, ':');
				sp.back().ToLong(&skip);
				block = sp[0];
			}

			if (S_CMPNOCASE(token, block))
			{
				string name = tz.getToken();
				for (int s = 0; s < skip; s++)
					name = tz.getToken();

				for (unsigned i = 0; i < language->nJBIgnore(); ++i)
					if (S_CMPNOCASE(name, language->jBIgnore(i)))
						name = tz.getToken();

				// Numbered block, add block name
				if (name.IsNumber())
					name = S_FMT("%s %s", language->jumpBlock(a), name);
				// Unnamed block, use block name
				if (name == "{" || name == ";")
					name = language->jumpBlock(a);

				// Create jump point
				jp_t jp;
				jp.name = name;
				jp.line = tz.lineNo() - 1;
				jump_points.push_back(jp);
			}
		}

		token = tz.getToken();
	}

	// Do nothing if no jump points
	if (jump_points.size() == 0)
		return;


	// --- Setup/show dialog ---
	wxDialog dlg(this, -1, "Jump To...");
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	// Add Jump to dropdown
	wxChoice* choice_jump_to = new wxChoice(&dlg, -1);
	sizer->Add(choice_jump_to, 0, wxEXPAND|wxALL, 4);
	for (unsigned a = 0; a < jump_points.size(); a++)
		choice_jump_to->Append(jump_points[a].name);
	choice_jump_to->SetSelection(0);

	// Add dialog buttons
	sizer->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Show dialog
	dlg.SetInitialSize(wxSize(250, -1));
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		int selection = choice_jump_to->GetSelection();
		if (selection >= 0 && selection < (int)jump_points.size())
		{
			// Get line number
			int line = jump_points[selection].line;

			// Move to line
			int pos = GetLineEndPosition(line);
			SetCurrentPos(pos);
			SetSelection(pos, pos);
			SetFirstVisibleLine(line);
			SetFocus();
		}
	}
}


/*******************************************************************
 * TEXTEDITOR CLASS EVENTS
 *******************************************************************/

/* TextEditor::onKeyDown
 * Called when a key is pressed
 *******************************************************************/
void TextEditor::onKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	wxArrayString binds = KeyBind::getBinds(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	bool handled = false;
	for (unsigned a = 0; a < binds.size(); a++)
	{
		string name = binds[a];

		// Open/update calltip
		if (name == "ted_calltip")
		{
			updateCalltip();
			handled = true;
		}

		// Autocomplete
		else if (name == "ted_autocomplete")
		{
			// Get word before cursor
			string word = GetTextRange(WordStartPosition(GetCurrentPos(), true), GetCurrentPos());

			// If a language is loaded, bring up autocompletion list
			if (language)
			{
				autocomp_list = language->getAutocompletionList(word);
				AutoCompShow(word.size(), autocomp_list);
			}

			handled = true;
		}

		// Find/replace
		else if (name == "ted_findreplace")
		{
			showFindReplaceDialog();
			handled = true;
		}

		// Find next
		else if (name == "ted_findnext")
		{
			wxCommandEvent e;
			onFRDBtnFindNext(e);
			handled = true;
		}

		// Jump to
		else if (name == "ted_jumpto")
		{
			openJumpToDialog();
			handled = true;
		}
	}

	if (!handled)
		e.Skip();
}

/* TextEditor::onKeyUp
 * Called when a key is released
 *******************************************************************/
void TextEditor::onKeyUp(wxKeyEvent& e)
{
	e.Skip();
}

/* TextEditor::onCharAdded
 * Called when a character is added to the text
 *******************************************************************/
void TextEditor::onCharAdded(wxStyledTextEvent& e)
{
	// Update line numbers margin width
	string numlines = S_FMT("0%d", GetNumberOfLines());
	SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, numlines));

	// Auto indent
	int currentLine = GetCurrentLine();
	if (txed_auto_indent && e.GetKey() == '\n')
	{
		// Get indentation amount
		int lineInd = 0;
		if (currentLine > 0)
			lineInd = GetLineIndentation(currentLine - 1);

		// Do auto-indent if needed
		if (lineInd != 0)
		{
			SetLineIndentation(currentLine, lineInd);

			// Skip to end of tabs
			while (1)
			{
				int chr = GetCharAt(GetCurrentPos());
				if (chr == '\t' || chr == ' ')
					GotoPos(GetCurrentPos()+1);
				else
					break;
			}
		}
	}

	// The following require a language to work
	if (language)
	{
		// Call tip
		if (e.GetKey() == '(' && txed_calltips_parenthesis)
		{
			openCalltip(GetCurrentPos());
		}

		// End call tip
		if (e.GetKey() == ')')
		{
			CallTipCancel();
		}

		// Comma, possibly update calltip
		if (e.GetKey() == ',' && txed_calltips_parenthesis)
		{
			//openCalltip(GetCurrentPos());
			//if (CallTipActive())
			updateCalltip();
		}
	}

	// Continue
	e.Skip();
}

/* TextEditor::onUpdateUI
 * Called when anything is modified in the text editor (cursor
 * position, styling, text, etc)
 *******************************************************************/
void TextEditor::onUpdateUI(wxStyledTextEvent& e)
{
	// Check for brace match
	if (txed_brace_match)
		checkBraceMatch();

	// If a calltip is open, update it
	if (CallTipActive())
		updateCalltip();

	e.Skip();
}

/* TextEditor::onCalltipClicked
 * Called when the current calltip is clicked on
 *******************************************************************/
void TextEditor::onCalltipClicked(wxStyledTextEvent& e)
{
	// Can't do anything without function
	if (!ct_function)
		return;

	// Argset up
	if (e.GetPosition() == 1)
	{
		if (ct_argset > 0)
		{
			ct_argset--;
			updateCalltip();
		}
	}

	// Argset down
	if (e.GetPosition() == 2)
	{
		if ((unsigned)ct_argset < ct_function->nArgSets() - 1)
		{
			ct_argset++;
			updateCalltip();
		}
	}
}

/* TextEditor::onMouseDwellStart
 * Called when the mouse pointer has 'dwelt' in one position for a
 * certain amount of time
 *******************************************************************/
void TextEditor::onMouseDwellStart(wxStyledTextEvent& e)
{
	if (!CallTipActive() && txed_calltips_mouse)
		openCalltip(e.GetPosition(), -1);
}

/* TextEditor::onMouseDwellEnd
 * Called when a mouse 'dwell' is interrupted/ended
 *******************************************************************/
void TextEditor::onMouseDwellEnd(wxStyledTextEvent& e)
{
	if (!(ct_function && ct_function->nArgSets() > 1))
		CallTipCancel();
}

/* TextEditor::onMouseDown
 * Called when a mouse button is clicked
 *******************************************************************/
void TextEditor::onMouseDown(wxMouseEvent& e)
{
	e.Skip();

	// No language, no checks
	if (!language)
		return;

	// Check for ctrl+left (web lookup)
	if (e.LeftDown() && e.GetModifiers() == wxMOD_CMD)
	{
		int pos = CharPositionFromPointClose(e.GetX(), e.GetY());
		string word = GetTextRange(WordStartPosition(pos, true), WordEndPosition(pos, true));

		if (!word.IsEmpty())
		{
			// Check for keyword
			if (language->isKeyword(word))
			{
				string url = language->getKeywordLink();
				if (!url.IsEmpty())
				{
					url.Replace("%s", word);
					wxLaunchDefaultBrowser(url);
				}
			}

			// Check for constant
			else if (language->isConstant(word))
			{
				string url = language->getConstantLink();
				if (!url.IsEmpty())
				{
					url.Replace("%s", word);
					wxLaunchDefaultBrowser(url);
				}
			}

			// Check for function
			else if (language->isFunction(word))
			{
				string url = language->getFunctionLink();
				if (!url.IsEmpty())
				{
					url.Replace("%s", word);
					wxLaunchDefaultBrowser(url);
				}
			}

			CallTipCancel();
		}
	}
}

/* TextEditor::onFocusLoss
 * Called when the text editor loses focus
 *******************************************************************/
void TextEditor::onFocusLoss(wxFocusEvent& e)
{
	CallTipCancel();
	AutoCompCancel();
}

/* TextEditor::onFRDBtnFindNext
 * Called when the 'Find Next' button on the Find+Replace frame is
 * clicked
 *******************************************************************/
void TextEditor::onFRDBtnFindNext(wxCommandEvent& e)
{
	// Check find string
	string find = dlg_fr->getFindString();
	if (find.IsEmpty())
		return;

	// Set search options
	int flags = 0;
	if (dlg_fr->matchCase()) flags |= wxSTC_FIND_MATCHCASE;
	if (dlg_fr->matchWord()) flags |= wxSTC_FIND_WHOLEWORD;
	SetSearchFlags(flags);

	// Do find
	if (!findNext(find))
		wxLogMessage("No text matching \"%s\" found.", find);
}

/* TextEditor::onFRDBtnReplace
 * Called when the 'Replace' button on the Find+Replace frame is
 * clicked
 *******************************************************************/
void TextEditor::onFRDBtnReplace(wxCommandEvent& e)
{
	// Set search options
	int flags = 0;
	if (dlg_fr->matchCase()) flags |= wxSTC_FIND_MATCHCASE;
	if (dlg_fr->matchWord()) flags |= wxSTC_FIND_WHOLEWORD;
	SetSearchFlags(flags);

	// Do replace
	replaceCurrent(dlg_fr->getFindString(), dlg_fr->getReplaceString());
}

/* TextEditor::onFRDBtnReplaceAll
 * Called when the 'Replace All' button on the Find+Replace frame is
 * clicked
 *******************************************************************/
void TextEditor::onFRDBtnReplaceAll(wxCommandEvent& e)
{
	// Set search options
	int flags = 0;
	if (dlg_fr->matchCase()) flags |= wxSTC_FIND_MATCHCASE;
	if (dlg_fr->matchWord()) flags |= wxSTC_FIND_WHOLEWORD;
	SetSearchFlags(flags);

	// Do replace all
	int replaced = replaceAll(dlg_fr->getFindString(), dlg_fr->getReplaceString());
	wxMessageBox(S_FMT("Replaced %d occurrences", replaced), "Replace All");
}

/* TextEditor::onFRDKeyDown
 * Called when a key is pressed on the Find+Replace frame
 *******************************************************************/
void TextEditor::onFRDKeyDown(wxKeyEvent& e)
{
	// Esc (close)
	if (e.GetKeyCode() == WXK_ESCAPE)
		dlg_fr->Close();

	// Enter
	else if (e.GetKeyCode() == WXK_RETURN)
	{
		// Find Next
		if (dlg_fr->getTextFind()->HasFocus())
		{
			// Check find string
			string find = dlg_fr->getFindString();
			if (find.IsEmpty())
				return;

			// Set search options
			int flags = 0;
			if (dlg_fr->matchCase()) flags |= wxSTC_FIND_MATCHCASE;
			if (dlg_fr->matchWord()) flags |= wxSTC_FIND_WHOLEWORD;
			SetSearchFlags(flags);

			// Do find
			if (!findNext(find))
				wxLogMessage("No text matching \"%s\" found.", find);
		}

		// Replace
		else if (dlg_fr->getTextReplace()->HasFocus())
		{
			// Set search options
			int flags = 0;
			if (dlg_fr->matchCase()) flags |= wxSTC_FIND_MATCHCASE;
			if (dlg_fr->matchWord()) flags |= wxSTC_FIND_WHOLEWORD;
			SetSearchFlags(flags);

			// Do replace
			replaceCurrent(dlg_fr->getFindString(), dlg_fr->getReplaceString());
		}

		else
			e.Skip();
	}

	// Other
	else
		e.Skip();
}
