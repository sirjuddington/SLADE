
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextEditorCtrl.cpp
// Description: The SLADE Text Editor control. Does syntax highlighting,
//              calltips, autocomplete and more, using an associated
//              TextLanguage
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
#include "App.h"
#include "TextEditorCtrl.h"
#include "Graphics/Icons.h"
#include "General/KeyBind.h"
#include "SCallTip.h"
#include "FindReplacePanel.h"
#include "Utility/Tokenizer.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
CVAR(Int, txed_tab_width, 4, CVAR_SAVE)
CVAR(Bool, txed_auto_indent, true, CVAR_SAVE)
CVAR(Bool, txed_syntax_hilight, true, CVAR_SAVE)
CVAR(Bool, txed_brace_match, false, CVAR_SAVE)
CVAR(Int, txed_edge_column, 80, CVAR_SAVE)
CVAR(Bool, txed_indent_guides, false, CVAR_SAVE)
CVAR(String, txed_style_set, "SLADE Default", CVAR_SAVE)
CVAR(Bool, txed_calltips_mouse, true, CVAR_SAVE)
CVAR(Bool, txed_calltips_parenthesis, true, CVAR_SAVE)
CVAR(Bool, txed_fold_enable, true, CVAR_SAVE)
CVAR(Bool, txed_fold_comments, false, CVAR_SAVE)
CVAR(Bool, txed_fold_preprocessor, true, CVAR_SAVE)
CVAR(Bool, txed_fold_lines, true, CVAR_SAVE)
CVAR(Bool, txed_fold_debug, false, CVAR_SECRET)
CVAR(Bool, txed_trim_whitespace, false, CVAR_SAVE)
CVAR(Bool, txed_word_wrap, false, CVAR_SAVE)
CVAR(Bool, txed_calltips_colourise, true, CVAR_SAVE)
CVAR(Bool, txed_calltips_use_font, false, CVAR_SAVE)
CVAR(Bool, txed_match_cursor_word, true, CVAR_SAVE)
CVAR(Int, txed_hilight_current_line, 2, CVAR_SAVE)
CVAR(Int, txed_line_extra_height, 0, CVAR_SAVE)
CVAR(Bool, txed_tab_spaces, false, CVAR_SAVE)
CVAR(Int, txed_show_whitespace, 0, CVAR_SAVE)

wxDEFINE_EVENT(wxEVT_COMMAND_JTCALCULATOR_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_TEXT_CHANGED, wxCommandEvent);


// ----------------------------------------------------------------------------
//
// JumpToCalculator Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// JumpToCalculator::Entry
//
// JumpToCalculator thread entry function
// ----------------------------------------------------------------------------
wxThread::ExitCode JumpToCalculator::Entry()
{
	string jump_points;

	Tokenizer tz;
	tz.setSpecialCharacters(";,:|={}/()");
	tz.openString(text_);

	string token = tz.getToken();
	while (!tz.atEnd())
	{
		if (token == "{")
		{
			// Skip block
			while (!tz.atEnd() && token != "}")
				token = tz.getToken();
		}

		for (unsigned a = 0; a < block_names_.size(); a++)
		{
			// Get jump block keyword
			string block = block_names_[a];
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

				for (unsigned i = 0; i < ignore_.size(); ++i)
					if (S_CMPNOCASE(name, ignore_[i]))
						name = tz.getToken();

				// Numbered block, add block name
				if (name.IsNumber())
					name = S_FMT("%s %s", block, name);
				// Unnamed block, use block name
				if (name == "{" || name == ";")
					name = block;

				// Add jump point
				jump_points += S_FMT("%d,%s,", tz.lineNo() - 1, CHR(name));
			}
		}

		token = tz.getToken();
	}

	// Remove ending comma
	if (!jump_points.empty())
		jump_points.RemoveLast(1);

	// Send event
	wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_JTCALCULATOR_COMPLETED);
	event->SetString(jump_points);
	wxQueueEvent(handler_, event);

	return nullptr;
}


// ----------------------------------------------------------------------------
//
// TextEditorCtrl Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// TextEditorCtrl::TextEditorCtrl
//
// TextEditorCtrl class constructor
// ----------------------------------------------------------------------------
TextEditorCtrl::TextEditorCtrl(wxWindow* parent, int id) :
	wxStyledTextCtrl(parent, id),
	lexer_{ std::make_unique<Lexer>() },
	timer_update_(this)
{
	// Init variables
	language_ = nullptr;
	ct_argset_ = 0;
	ct_function_ = nullptr;
	ct_start_ = 0;
	prev_cursor_pos_ = -1;
	prev_text_length_ = -1;
	prev_brace_match_ = -1;
	panel_fr_ = nullptr;
	call_tip_ = new SCallTip(this);
	choice_jump_to_ = nullptr;
	jump_to_calculator_ = nullptr;
	update_jump_to_ = false;
	update_word_match_ = false;
	last_modified_ = App::runTimer();

	// Line numbers by default
	SetMarginType(0, wxSTC_MARGIN_NUMBER);
	SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, "9999"));

	// Folding margin
	setupFoldMargin();

	// Border margin
	SetMarginWidth(2, 4);

	// Register icons for autocompletion list
	RegisterImage(1, Icons::getIcon(Icons::TEXT_EDITOR, "key"));
	RegisterImage(2, Icons::getIcon(Icons::TEXT_EDITOR, "const"));
	RegisterImage(3, Icons::getIcon(Icons::TEXT_EDITOR, "func"));

	// Init w/no language
	setLanguage(nullptr);

	// Setup various configurable properties
	setup();

	// Add to text styles editor list
	StyleSet::addEditor(this);

	// Bind events
	Bind(wxEVT_KEY_DOWN, &TextEditorCtrl::onKeyDown, this);
	Bind(wxEVT_KEY_UP, &TextEditorCtrl::onKeyUp, this);
	Bind(wxEVT_STC_CHARADDED, &TextEditorCtrl::onCharAdded, this);
	Bind(wxEVT_STC_UPDATEUI, &TextEditorCtrl::onUpdateUI, this);
	Bind(wxEVT_STC_CALLTIP_CLICK, &TextEditorCtrl::onCalltipClicked, this);
	Bind(wxEVT_STC_DWELLSTART, &TextEditorCtrl::onMouseDwellStart, this);
	Bind(wxEVT_STC_DWELLEND, &TextEditorCtrl::onMouseDwellEnd, this);
	Bind(wxEVT_LEFT_DOWN, &TextEditorCtrl::onMouseDown, this);
	Bind(wxEVT_KILL_FOCUS, &TextEditorCtrl::onFocusLoss, this);
	Bind(wxEVT_ACTIVATE, &TextEditorCtrl::onActivate, this);
	Bind(wxEVT_STC_MARGINCLICK, &TextEditorCtrl::onMarginClick, this);
	Bind(wxEVT_COMMAND_JTCALCULATOR_COMPLETED, &TextEditorCtrl::onJumpToCalculateComplete, this);
	Bind(wxEVT_STC_CHANGE, &TextEditorCtrl::onModified, this);
	Bind(wxEVT_TIMER, &TextEditorCtrl::onUpdateTimer, this);
	Bind(wxEVT_STC_STYLENEEDED, &TextEditorCtrl::onStyleNeeded, this);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::~TextEditorCtrl
//
// TextEditorCtrl class destructor
// ----------------------------------------------------------------------------
TextEditorCtrl::~TextEditorCtrl()
{
	StyleSet::removeEditor(this);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::setup
//
// Sets up text editor properties depending on cvars and the current text
// styleset/style
// ----------------------------------------------------------------------------
void TextEditorCtrl::setup()
{
	// General settings
	SetBufferedDraw(true);
	SetUseAntiAliasing(true);
	SetMouseDwellTime(300);
	AutoCompSetIgnoreCase(true);
	SetIndentationGuides(txed_indent_guides);
	SetExtraAscent(txed_line_extra_height);
	SetExtraDescent(txed_line_extra_height);

	// Tab width and style
	SetTabWidth(txed_tab_width);
	SetIndent(txed_tab_width);
	SetUseTabs(!txed_tab_spaces);

	// TODO: Caret options?
	//SetCaretWidth(2);
	//SetCaretForeground(...);

	// Caret line hilight
	SetCaretLineVisible(txed_hilight_current_line > 0);

	// Whitespace
	if (txed_show_whitespace > 0)
	{
		SetViewWhiteSpace(txed_show_whitespace == 1 ? wxSTC_WS_VISIBLEAFTERINDENT : wxSTC_WS_VISIBLEALWAYS);
		SetWhitespaceSize(3);

		// TODO: separate colour
		SetWhitespaceForeground(true, WXCOL(StyleSet::currentSet()->getStyle("guides")->getForeground()));
	}
	else
		SetViewWhiteSpace(wxSTC_WS_INVISIBLE);

	// Right margin line
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
	CallTipSetForegroundHighlight(WXCOL(StyleSet::currentSet()->getStyle("calltip_hl")->getForeground()));

	// Set folding options
	setupFolding();

	// Re-colour text
	Colourise(0, GetTextLength());

	// Set word wrapping
	if (txed_word_wrap)
		SetWrapMode(wxSTC_WRAP_WORD);
	else
		SetWrapMode(wxSTC_WRAP_NONE);

	// Set word match indicator style
	SetIndicatorCurrent(8);
	IndicatorSetStyle(8, wxSTC_INDIC_ROUNDBOX);
	IndicatorSetUnder(8, true);
	IndicatorSetOutlineAlpha(8, 60);
	IndicatorSetAlpha(8, 40);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::setupFoldMargin
//
// Sets up the code folding margin
// ----------------------------------------------------------------------------
void TextEditorCtrl::setupFoldMargin(TextStyle* margin_style)
{
	if (!txed_fold_enable)
	{
		SetMarginWidth(1, 0);
		return;
	}

	wxColour col_fg, col_bg;
	if (margin_style)
	{
		col_fg = WXCOL(margin_style->getForeground());
		col_bg = WXCOL(margin_style->getBackground());
	}
	else
	{
		col_fg = WXCOL(StyleSet::currentSet()->getStyle("foldmargin")->getForeground());
		col_bg = WXCOL(StyleSet::currentSet()->getStyle("foldmargin")->getBackground());
	}

	SetMarginType(1, wxSTC_MARGIN_SYMBOL);
	SetMarginWidth(1, 16);
	SetMarginSensitive(1, true);
	SetMarginMask(1, wxSTC_MASK_FOLDERS);
	SetFoldMarginColour(true, col_bg);
	SetFoldMarginHiColour(true, col_bg);
	MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_VLINE, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_LCORNER, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUSCONNECTED, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER, col_bg, col_fg);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::setLanguage
//
// Sets the text editor language
// ----------------------------------------------------------------------------
bool TextEditorCtrl::setLanguage(TextLanguage* lang)
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
		autocomp_list_.Clear();

		// Set lexer to basic mode
		lexer_->loadLanguage(nullptr);
	}

	// Setup syntax hilighting if needed
	else
	{
		// Create correct lexer type for language
		if (lang->id() == "zscript")
			lexer_ = std::make_unique<ZScriptLexer>();
		else
			lexer_ = std::make_unique<Lexer>();

		// Load to lexer
		lexer_->loadLanguage(lang);

		// Load autocompletion list
		autocomp_list_ = lang->autocompletionList();
	}

	// Set folding options
	setupFolding();

	// Update variables
	SetWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-$");
	this->language_ = lang;

	// Re-colour text
	Colourise(0, GetTextLength());

	// Update Jump To list
	updateJumpToList();

	return true;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::applyStyleSet
//
// Applies the styleset [style] to the text editor
// ----------------------------------------------------------------------------
bool TextEditorCtrl::applyStyleSet(StyleSet* style)
{
	// Check if one was given
	if (!style)
		return false;

	// Apply it
	style->applyTo(this);

	return true;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::loadEntry
//
// Reads the contents of [entry] into the text area, returns false if the given
// entry is invalid
// ----------------------------------------------------------------------------
bool TextEditorCtrl::loadEntry(ArchiveEntry* entry)
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
	string text = wxString::FromUTF8((const char*)entry->getData(), entry->getSize());
	// If opening as UTF8 failed for some reason, try again as 8-bit data
	if (text.length() == 0)
		text = wxString::From8BitData((const char*)entry->getData(), entry->getSize());

	// Load text into editor
	SetText(text);
	last_modified_ = App::runTimer();

	// Update line numbers margin width
	string numlines = S_FMT("0%d", txed_fold_debug ? 1234567 : GetNumberOfLines());
	SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, numlines));

	return true;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::getRawText
//
// Writes the raw ASCII text to [mc]
// ----------------------------------------------------------------------------
void TextEditorCtrl::getRawText(MemChunk& mc)
{
	mc.clear();
	string text = GetText();
	mc.importMem((const uint8_t*)text.ToUTF8().data(), text.ToUTF8().length());
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::trimWhitespace
//
// Removes any unneeded whitespace from the ends of lines
// ----------------------------------------------------------------------------
void TextEditorCtrl::trimWhitespace()
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

// ----------------------------------------------------------------------------
// TextEditorCtrl::showFindReplacePanel
//
// Shows or hides the Find+Replace panel, depending on [show].
// If shown, fills the find text box with the current selection or the current
// word at the caret
// ----------------------------------------------------------------------------
void TextEditorCtrl::showFindReplacePanel(bool show)
{
	// Do nothing if no F+R panel has been set
	if (!panel_fr_)
		return;

	// Hide if needed
	if (!show)
	{
		panel_fr_->Hide();
		panel_fr_->GetParent()->Layout();
		SetFocus();
		return;
	}

	// Get currently selected text
	string find = GetSelectedText();

	// Get the word at the current cursor position if there is no current selection
	if (find.IsEmpty())
	{
		int ws = WordStartPosition(GetCurrentPos(), true);
		int we = WordEndPosition(GetCurrentPos(), true);
		find = GetTextRange(ws, we);
	}

	// Show the F+R panel
	panel_fr_->Show();
	panel_fr_->GetParent()->Layout();
	panel_fr_->setFindText(find);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::findNext
//
// Finds the next occurrence of the [find] after the caret position, selects it
// and scrolls to it if needed.
// Returns false if the [find] was invalid or no match was found, true
// otherwise
// ----------------------------------------------------------------------------
bool TextEditorCtrl::findNext(string find, int flags)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Get current selection
	int sel_start = GetSelectionStart();
	int sel_end = GetSelectionEnd();

	// Search forwards from the end of the current selection
	SetSelection(GetCurrentPos(), GetCurrentPos());
	SearchAnchor();
	int found = SearchNext(flags, find);
	if (found < 0)
	{
		// Not found, loop back to start
		SetSelection(0, 0);
		SearchAnchor();
		found = SearchNext(flags, find);
		if (found < 0)
		{
			// No match found in entire text, reset selection
			SetSelection(sel_start, sel_end);
			return false;
		}
	}

	// Set caret to the end of the matching text
	// (it defaults to the start for some dumb reason)
	// and scroll to the selection
	SetSelection(found, found + find.length());
	EnsureCaretVisible();

	return true;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::findPrev
//
// Finds the previous occurrence of the [find] after the caret position,
// selects it and scrolls to it if needed.
// Returns false if the [find] was invalid or no match was found, true
// otherwise
// ----------------------------------------------------------------------------
bool TextEditorCtrl::findPrev(string find, int flags)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Get current selection
	int sel_start = GetSelectionStart();
	int sel_end = GetSelectionEnd();

	// Search back from the start of the current selection
	SetSelection(sel_start, sel_start);
	SearchAnchor();
	int found = SearchPrev(flags, find);
	if (found < 0)
	{
		// Not found, loop back to end
		SetSelection(GetTextLength() - 1, GetTextLength() - 1);
		SearchAnchor();
		found = SearchPrev(flags, find);
		if (found < 0)
		{
			// No match found in entire text, reset selection
			SetSelection(sel_start, sel_end);
			return false;
		}
	}

	// Set caret to the end of the matching text
	// (it defaults to the start for some dumb reason)
	// and scroll to the selection
	SetSelection(found, found + find.length());
	EnsureCaretVisible();

	return true;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::replaceCurrent
//
// Replaces the currently selected occurrence of [find] with [replace], then
// selects and scrolls to the next occurrence of [find] in the text.
// Returns false if [find] is invalid or the current selection does not match
// it, true otherwise
// ----------------------------------------------------------------------------
bool TextEditorCtrl::replaceCurrent(string find, string replace, int flags)
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
	findNext(find, flags);

	return true;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::replaceAll
//
// Replaces all occurrences of [find] in the text with [replace].
// Returns the number of occurrences replaced
// ----------------------------------------------------------------------------
int TextEditorCtrl::replaceAll(string find, string replace, int flags)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Start at beginning
	SetSelection(0, 0);

	// Loop of death
	int replaced = 0;
	while (true)
	{
		SearchAnchor();
		int found = SearchNext(flags, find);
		if (found < 0)
			break;	// No matches, finished
		else
		{
			// Replace text & increment counter
			Replace(found, found + find.length(), replace);
			replaced++;

			// Continue from end of replaced text
			SetSelection(found + find.length(), found + find.length());
		}
	}

	// Return number of instances replaced
	return replaced;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::checkBraceMatch
//
// Checks for a brace match at the current cursor position
// ----------------------------------------------------------------------------
void TextEditorCtrl::checkBraceMatch()
{
#ifdef __WXMAC__
	bool refresh = false;
#else
	bool refresh = true;
#endif

	// Ignore if cursor position hasn't changed since the last check
	if (GetCurrentPos() == prev_cursor_pos_)
		return;

	// Check for brace match at current position
	int bracematch = BraceMatch(GetCurrentPos());
	if (bracematch != wxSTC_INVALID_POSITION)
	{
		BraceHighlight(GetCurrentPos(), bracematch);
		if (refresh && prev_brace_match_ != bracematch)
		{
			Refresh();
			Update();
		}
		prev_brace_match_ = bracematch;
		return;
	}

	// No match, check for match at previous position
	bracematch = BraceMatch(GetCurrentPos() - 1);
	if (bracematch != wxSTC_INVALID_POSITION)
	{
		BraceHighlight(GetCurrentPos() - 1, bracematch);
		if (refresh && prev_brace_match_ != bracematch)
		{
			Refresh();
			Update();
		}
		prev_brace_match_ = bracematch;
		return;
	}

	// No match at all, clear any previous brace match
	BraceHighlight(-1, -1);
	if (refresh && prev_brace_match_ != -1)
	{
		Refresh();
		Update();
	}

	prev_brace_match_ = -1;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::matchWord
//
// Highlights all words in the text matching the word at the current cursor
// position
// ----------------------------------------------------------------------------
void TextEditorCtrl::matchWord()
{
	if (!txed_match_cursor_word || !language_)
		return;

	// Get word/text to match
	string current_word;
	int word_start, word_end;
	if (!HasSelection())
	{
		// No selection, get word at cursor
		word_start = WordStartPosition(GetCurrentPos(), true);
		word_end = WordEndPosition(GetCurrentPos(), true);
		current_word = GetTextRange(word_start, word_end);
	}
	else
	{
		// Get selection
		current_word = GetSelectedText();
		GetSelection(&word_start, &word_end);
	}

	if (!current_word.IsEmpty() && HasFocus())
	{
		if (current_word != prev_word_match_)
		{
			prev_word_match_ = current_word;

			// Apply word match indicator to matching text
			SetIndicatorCurrent(8);
			IndicatorClearRange(0, GetTextLength());
			SetTargetStart(0);
			SetTargetEnd(GetTextLength());
			SetSearchFlags(0);
			while (SearchInTarget(current_word) != -1)
			{
				// Don't apply to current selection
				if (GetTargetStart() != word_start || !HasSelection())
					IndicatorFillRange(GetTargetStart(), GetTargetEnd() - GetTargetStart());

				SetTargetStart(GetTargetEnd());
				SetTargetEnd(GetTextLength());
			}
		}
	}
	else
		clearWordMatch();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::clearWordMatch
//
// Clears all word match highlights
// ----------------------------------------------------------------------------
void TextEditorCtrl::clearWordMatch()
{
	SetIndicatorCurrent(8);
	IndicatorClearRange(0, GetTextLength());
	prev_word_match_ = "";
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::showCalltip
//
// Shows the calltip window underneath [position] in the text
// ----------------------------------------------------------------------------
void TextEditorCtrl::showCalltip(int position)
{
	// Setup calltip colours
	StyleSet* ss_current = StyleSet::currentSet();
	call_tip_->setBackgroundColour(ss_current->getStyle("calltip")->getBackground());
	call_tip_->setTextColour(ss_current->getStyle("calltip")->getForeground());
	call_tip_->setTextHighlightColour(ss_current->getStyle("calltip_hl")->getForeground());
	if (txed_calltips_colourise)
	{
		call_tip_->setFunctionColour(ss_current->getStyle("function")->getForeground());
		call_tip_->setTypeColour(ss_current->getStyle("type")->getForeground());
		call_tip_->setKeywordColour(ss_current->getStyle("keyword")->getForeground());
	}
	if (txed_calltips_use_font)
		call_tip_->setFont(ss_current->getDefaultFontFace(), ss_current->getDefaultFontSize() * 0.9);
	else
		call_tip_->setFont("", 0);

	// Determine position
	wxPoint pos = GetScreenPosition() + PointFromPosition(position);
	pos.y += TextHeight(GetCurrentLine()) + 2;
	call_tip_->SetPosition(wxPoint(pos.x, pos.y));

	call_tip_->Show();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::hideCalltip
//
// Hides the calltip window
// ----------------------------------------------------------------------------
void TextEditorCtrl::hideCalltip()
{
	call_tip_->Hide();
	CallTipCancel();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::openCalltip
//
// Opens a calltip for the function name before [pos].
// Returns false if the word before [pos] was not a function name, true
// otherwise
// ----------------------------------------------------------------------------
bool TextEditorCtrl::openCalltip(int pos, int arg, bool dwell)
{
	// Don't bother if no language
	if (!language_)
		return false;

	// Get start of word before bracket
	int start = WordStartPosition(pos - 1, false);
	int end = WordEndPosition(pos - 1, true);

	// Check with the lexer if we have a function
	if (!lexer_->isFunction(this, WordStartPosition(start, true), WordEndPosition(start, true)))
		return false;

	// Get word before bracket
	string word = GetTextRange(WordStartPosition(start, true), WordEndPosition(start, true));

	// Get matching language function (if any)
	TLFunction* func = language_->function(word);

	// Show calltip if it's a function
	if (func && func->contexts().size() > 0)
	{
		call_tip_->enableArgSwitch(!dwell && func->contexts().size() > 1);
		call_tip_->openFunction(func, arg);
		showCalltip(dwell ? pos : end + 1);

		ct_function_ = func;
		ct_start_ = pos;
		ct_dwell_ = dwell;

		// Highlight arg
		call_tip_->setCurrentArg(arg);

		return true;
	}
	else
	{
		ct_function_ = nullptr;
		return false;
	}
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::updateCalltip
//
// Updates the current calltip, or attempts to open one if none is currently
// showing
// ----------------------------------------------------------------------------
void TextEditorCtrl::updateCalltip()
{
	// Don't bother if no language
	if (!language_)
		return;

	if (!call_tip_->IsShown())
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
				if (!openCalltip(pos, 0))
					return;
				else
					break;
			}

			// Go to previous character
			pos--;
		}
	}

	if (ct_function_)
	{
		// Hide calltip if we've gone before the start of the function
		if (GetCurrentPos() < ct_start_)
		{
			hideCalltip();
			ct_function_ = nullptr;
			return;
		}

		// Calltip currently showing, determine what arg we're at
		int pos = ct_start_+1;
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
				hideCalltip();
				ct_function_ = nullptr;
				return;
			}

			// Go to next character
			pos++;
		}

		// Update calltip string with the selected arg set and the current arg highlighted
		call_tip_->setCurrentArg(arg);
	}
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::setJumpToControl
//
// Sets the wxChoice control to use for the 'Jump To' feature
// ----------------------------------------------------------------------------
void TextEditorCtrl::setJumpToControl(wxChoice* jump_to)
{
	choice_jump_to_ = jump_to;
	choice_jump_to_->Bind(wxEVT_CHOICE, &TextEditorCtrl::onJumpToChoiceSelected, this);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::updateJumpToList
//
// Begin updating the 'Jump To' list
// ----------------------------------------------------------------------------
void TextEditorCtrl::updateJumpToList()
{
	if (!choice_jump_to_)
		return;

	if (!language_ || jump_to_calculator_ || GetText().length() == 0)
	{
		choice_jump_to_->Clear();
		return;
	}

	// Begin jump to calculation thread
	choice_jump_to_->Enable(false);
	jump_to_calculator_ = new JumpToCalculator(this, GetText(), language_->jumpBlocks(), language_->jumpBlocksIgnored());
	jump_to_calculator_->Run();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::jumpToLine
//
// Prompts the user for a line number and moves the cursor to the end of the
// entered line
// ----------------------------------------------------------------------------
void TextEditorCtrl::jumpToLine()
{
	int numlines = GetNumberOfLines();

	// Prompt for line number
	long line = wxGetNumberFromUser(
		"Enter a line number to jump to",
		S_FMT("Line number (1-%d):", numlines),
		"Jump To Line",
		GetCurrentLine() + 1,
		1,
		numlines,
		this);

	if (line >= 1)
	{
		// Move to line
		int pos = GetLineEndPosition(line - 1);
		SetCurrentPos(pos);
		SetSelection(pos, pos);
		EnsureCaretVisible();
		SetFocus();
	}
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::foldAll
//
// Folds or unfolds all code folding levels, depending on [fold]
// ----------------------------------------------------------------------------
void TextEditorCtrl::foldAll(bool fold)
{
#if (wxMAJOR_VERSION >= 3 && wxMINOR_VERSION >= 1)
	// FoldAll is only available in wxWidgets 3.1+
	FoldAll(fold ? wxSTC_FOLDACTION_CONTRACT : wxSTC_FOLDACTION_EXPAND);
#else
	for (int a = 0; a < GetNumberOfLines(); a++)
	{
		int level = GetFoldLevel(a);
		if ((level & wxSTC_FOLDLEVELHEADERFLAG) > 0 &&
			GetFoldExpanded(a) == fold)
			ToggleFold(a);
	}
#endif
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::setupFolding
//
// Sets up code folding options
// ----------------------------------------------------------------------------
void TextEditorCtrl::setupFolding()
{
	if (txed_fold_enable)
	{
		// Set folding options
		lexer_->foldComments(txed_fold_comments);
		lexer_->foldPreprocessor(txed_fold_preprocessor);

		int flags = 0;
		if (txed_fold_debug)
			flags |= wxSTC_FOLDFLAG_LEVELNUMBERS;
		if (txed_fold_lines)
			flags |= wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED;
		SetFoldFlags(flags);
	}
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::lineComment
//
// Comment selected/current lines using line comments
// ----------------------------------------------------------------------------
void TextEditorCtrl::lineComment()
{
	string space, empty, comment, commentSpace;
	space = wxString::FromUTF8(" ");
	empty = wxString::FromUTF8("");

	if(language_)
		comment = language_->lineComment();
	else
		comment = default_line_comment_;

	commentSpace = comment + space;

	int selectionStart, selectionEnd;
	GetSelection(&selectionStart, &selectionEnd);

	bool singleLine = false;
	if (selectionStart == selectionEnd)
		singleLine = true;

	int firstLine, lastLine;
	firstLine = LineFromPosition(selectionStart);
	lastLine = LineFromPosition(selectionEnd);

	size_t selectionStartOffs, selectionEndOffs;
	selectionStartOffs = selectionEndOffs = 0;

	BeginUndoAction();
	for (int line = firstLine; line <= lastLine; ++line)
	{
		string lineText = GetTextRange(PositionFromLine(line), GetLineEndPosition(line));

		SetTargetStart(PositionFromLine(line));
		SetTargetEnd(GetLineEndPosition(line));

		if (lineText.StartsWith(commentSpace))
		{
			if (line == firstLine)
				selectionStartOffs -= commentSpace.Len();
			selectionEndOffs -= commentSpace.Len();

			lineText.Replace(commentSpace, empty, false);
			ReplaceTarget(lineText);
		}
		else if (lineText.StartsWith(comment))
		{
			if (line == firstLine)
				selectionStartOffs -= comment.Len();
			selectionEndOffs -= comment.Len();

			lineText.Replace(comment, empty, false);
			ReplaceTarget(lineText);
		}
		else if (lineText.Trim().Len() != 0)
		{
			if (line == firstLine)
				selectionStartOffs += commentSpace.Len();
			selectionEndOffs += commentSpace.Len();

			ReplaceTarget(lineText.Prepend(commentSpace));
		}
	}
	EndUndoAction();

	if (singleLine)
		GotoPos(selectionStart + selectionEndOffs);
	else
		SetSelection(selectionStart + selectionStartOffs, selectionEnd + selectionEndOffs);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::blockComment
//
// Comment selected text using block comments
// ----------------------------------------------------------------------------
void TextEditorCtrl::blockComment()
{
	string space, commentBegin, commentEnd;
	space = wxString::FromUTF8(" ");
	if(language_)
	{
		commentBegin = language_->commentBegin();
		commentEnd   = language_->commentEnd();
	}
	else
	{
		commentBegin = default_begin_comment_;
		commentEnd   = default_end_comment_;
	}

	size_t commentBeginLen, commentEndLen;
	commentBeginLen = commentBegin.Len();
	commentEndLen = commentEnd.Len();

	int selectionStart, selectionEnd;
	GetSelection(&selectionStart, &selectionEnd);

	SetTargetStart(selectionStart);
	SetTargetEnd(selectionEnd);

	SetInsertionPoint(selectionStart);

	string textString = GetRange(selectionStart, selectionEnd);

	if (!textString.StartsWith(commentBegin) && !textString.EndsWith(commentEnd))
	{
		commentBegin = commentBegin.append(space);
		commentEnd = commentEnd.Prepend(space);

		ReplaceTarget(textString.Prepend(commentBegin).append(commentEnd));
		selectionEnd += (int)(commentBegin.Len() + commentEnd.Len());
	}
	else if (textString.StartsWith(commentBegin) && textString.EndsWith(commentEnd))
	{
		if (textString.StartsWith(commentBegin.append(space)))
			commentBeginLen = commentBegin.Len();
		if (textString.EndsWith(commentEnd.Prepend(space)))
			commentEndLen = commentEnd.Len();

		ReplaceTarget(textString.Remove(0, commentBeginLen).RemoveLast(commentEndLen));
		selectionEnd -= (int)(commentBeginLen + commentEndLen);
	}

	SetSelection(selectionStart, selectionEnd);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::cycleComments
//
// Switch the prefered comment style to next style available.
// ----------------------------------------------------------------------------
void TextEditorCtrl::cycleComments()
{
	if (!language_)
		return;

	// For now, we assume all comment types have the same number of styles.
	size_t total_styles = language_->lineCommentL().size();
	unsigned next_style = language_->getPreferedComments() + 1;
	next_style =  next_style >= total_styles ? 0 : next_style;
	language_->setPreferedComments(next_style);
}

// ----------------------------------------------------------------------------
//
// TextEditorCtrl Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// TextEditorCtrl::onKeyDown
//
// Called when a key is pressed
// ----------------------------------------------------------------------------
void TextEditorCtrl::onKeyDown(wxKeyEvent& e)
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
			if (language_)
			{
				autocomp_list_ = language_->autocompletionList(word);
				AutoCompShow(word.size(), autocomp_list_);
			}

			handled = true;
		}

		// Find/replace
		else if (name == "ted_findreplace")
		{
			showFindReplacePanel();
			handled = true;
		}

		// Find next
		else if (name == "ted_findnext")
		{
			if (panel_fr_ && panel_fr_->IsShown())
				findNext(panel_fr_->findText(), panel_fr_->findFlags());

			handled = true;
		}

		// Find previous
		else if (name == "ted_findprev")
		{
			if (panel_fr_ && panel_fr_->IsShown())
				findPrev(panel_fr_->findText(), panel_fr_->findFlags());

			handled = true;
		}

		// Replace next
		else if (name == "ted_replacenext")
		{
			if (panel_fr_ && panel_fr_->IsShown())
				replaceCurrent(panel_fr_->findText(), panel_fr_->replaceText(), panel_fr_->findFlags());

			handled = true;
		}

		// Replace all
		else if (name == "ted_replaceall")
		{
			if (panel_fr_ && panel_fr_->IsShown())
				replaceAll(panel_fr_->findText(), panel_fr_->replaceText(), panel_fr_->findFlags());

			handled = true;
		}

		// Fold all
		else if (name == "ted_fold_foldall")
		{
			foldAll(true);
			handled = true;
		}

		// Unfold all
		else if (name == "ted_fold_unfoldall")
		{
			foldAll(false);
			handled = true;
		}

		// Jump to line
		else if (name == "ted_jumptoline")
		{
			jumpToLine();
			handled = true;
		}

		// Comments
		else if (name == "ted_line_comment")
		{
			lineComment();
			handled = true;
		}

		else if (name == "ted_block_comment")
		{
			blockComment();
			handled = true;
		}

		else if (name == "ted_cycle_comments")
		{
			cycleComments();
			handled = true;
		}
	}

	// Check for esc key
	if (!handled && e.GetKeyCode() == WXK_ESCAPE)
	{
		// Hide call tip if showing
		if (call_tip_->IsShown())
			call_tip_->Show(false);

		// Hide F+R panel if showing
		else if (panel_fr_ && panel_fr_->IsShown())
			showFindReplacePanel(false);
	}

	// Check for up/down keys while calltip with multiple arg sets is open
	if (call_tip_->IsShown() && ct_function_ && ct_function_->contexts().size() > 1 && !ct_dwell_)
	{
		if (e.GetKeyCode() == WXK_UP)
		{
			call_tip_->prevArgSet();
			handled = true;
		}
		else if (e.GetKeyCode() == WXK_DOWN)
		{
			call_tip_->nextArgSet();
			handled = true;
		}
	}

#ifdef __WXMSW__
	Colourise(GetCurrentPos(), GetLineEndPosition(GetCurrentLine()));
#endif
	
#ifdef __APPLE__
	if (!handled)
	{
		const int  keyCode =   e.GetKeyCode();
		const bool shiftDown = e.ShiftDown();

		if (e.ControlDown())
		{
			if (WXK_LEFT == keyCode)
			{
				if (shiftDown)
					HomeExtend();
				else
					Home();

				handled = true;
			}
			else if (WXK_RIGHT == keyCode)
			{
				if (shiftDown)
					LineEndExtend();
				else
					LineEnd();

				handled = true;
			}
			else if (WXK_UP == keyCode)
			{
				if (shiftDown)
					DocumentStartExtend();
				else
					DocumentStart();

				handled = true;
			}
			else if (WXK_DOWN == keyCode)
			{
				if (shiftDown)
					DocumentEndExtend();
				else
					DocumentEnd();

				handled = true;
			}
		}
		else if (e.RawControlDown())
		{
			if (WXK_LEFT == keyCode)
			{
				if (shiftDown)
					WordLeftExtend();
				else
					WordLeft();

				handled = true;
			}
			else if (WXK_RIGHT == keyCode)
			{
				if (shiftDown)
					WordRightExtend();
				else
					WordRight();

				handled = true;
			}
		}
	}
#endif // __APPLE__

	if (!handled)
		e.Skip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onKeyUp
//
// Called when a key is released
// ----------------------------------------------------------------------------
void TextEditorCtrl::onKeyUp(wxKeyEvent& e)
{
	e.Skip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onCharAdded
//
// Called when a character is added to the text
// ----------------------------------------------------------------------------
void TextEditorCtrl::onCharAdded(wxStyledTextEvent& e)
{
	// Update line numbers margin width
	string numlines = S_FMT("0%d", txed_fold_debug ? 1234567 : GetNumberOfLines());
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
	if (language_)
	{
		// Call tip
		if (e.GetKey() == '(' && txed_calltips_parenthesis)
			openCalltip(GetCurrentPos(), 0);

		// End call tip
		if (e.GetKey() == ')' || e.GetKey() == WXK_BACK)
			updateCalltip();

		// Comma, possibly update calltip
		if (e.GetKey() == ',' && txed_calltips_parenthesis)
			updateCalltip();
	}

	// Continue
	e.Skip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onUpdateUI
//
// Called when anything is modified in the text editor
// (cursor position, styling, text, etc)
// ----------------------------------------------------------------------------
void TextEditorCtrl::onUpdateUI(wxStyledTextEvent& e)
{
	// Check for brace match
	if (txed_brace_match)
		checkBraceMatch();

	// If a calltip is open, update it
	if (call_tip_->IsShown())
		updateCalltip();

	// Do word matching if appropriate
	if (txed_match_cursor_word && language_ && prev_cursor_pos_ != GetCurrentPos())
	{
		clearWordMatch();
		update_word_match_ = true;

		if (!HasSelection())
			timer_update_.Start(500, true);
		else
			timer_update_.Start(100, true);
	}

	// Hilight current line
	MarkerDeleteAll(1);
	MarkerDeleteAll(2);
	if (txed_hilight_current_line > 0 && HasFocus())
	{
		int line = LineFromPosition(GetCurrentPos());
		if (txed_hilight_current_line > 1)
			MarkerAdd(line, 2);
	}

	prev_cursor_pos_ = GetCurrentPos();
	prev_text_length_ = GetTextLength();

	e.Skip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onCalltipClicked
//
// Called when the current calltip is clicked on
// ----------------------------------------------------------------------------
void TextEditorCtrl::onCalltipClicked(wxStyledTextEvent& e)
{
	// Can't do anything without function
	if (!ct_function_)
		return;

	// Argset up
	if (e.GetPosition() == 1)
	{
		if (ct_argset_ > 0)
		{
			ct_argset_--;
			updateCalltip();
		}
	}

	// Argset down
	if (e.GetPosition() == 2)
	{
		if ((unsigned)ct_argset_ < ct_function_->contexts().size() - 1)
		{
			ct_argset_++;
			updateCalltip();
		}
	}
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onMouseDwellStart
//
// Called when the mouse pointer has 'dwelt' in one position for a certain
// amount of time
// ----------------------------------------------------------------------------
void TextEditorCtrl::onMouseDwellStart(wxStyledTextEvent& e)
{
	if (wxTheApp->IsActive() && HasFocus() && !call_tip_->IsShown() && txed_calltips_mouse && e.GetPosition() >= 0)
	{
		openCalltip(e.GetPosition(), -1, true);
		ct_dwell_ = true;
	}
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onMouseDwellEnd
//
// Called when a mouse 'dwell' is interrupted/ended
// ----------------------------------------------------------------------------
void TextEditorCtrl::onMouseDwellEnd(wxStyledTextEvent& e)
{
	if (call_tip_->IsShown() && ct_dwell_)
		hideCalltip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onMouseDown
//
// Called when a mouse button is clicked
// ----------------------------------------------------------------------------
void TextEditorCtrl::onMouseDown(wxMouseEvent& e)
{
	e.Skip();

	// No language, no checks
	if (!language_)
		return;

	// Check for ctrl+left (web lookup)
	if (e.LeftDown() && e.GetModifiers() == wxMOD_CMD)
	{
		int pos = CharPositionFromPointClose(e.GetX(), e.GetY());
		string word = GetTextRange(WordStartPosition(pos, true), WordEndPosition(pos, true));

		if (!word.IsEmpty())
		{
			// TODO: Reimplement for word lists
			//// Check for keyword
			//if (language->isKeyword(word))
			//{
			//	string url = language->getKeywordLink();
			//	if (!url.IsEmpty())
			//	{
			//		url.Replace("%s", word);
			//		wxLaunchDefaultBrowser(url);
			//	}
			//}

			//// Check for constant
			//else if (language->isConstant(word))
			//{
			//	string url = language->getConstantLink();
			//	if (!url.IsEmpty())
			//	{
			//		url.Replace("%s", word);
			//		wxLaunchDefaultBrowser(url);
			//	}
			//}

			// Check for function
			if (language_->isFunction(word))
			{
				string url = language_->functionLink();
				if (!url.IsEmpty())
				{
					url.Replace("%s", word);
					wxLaunchDefaultBrowser(url);
				}
			}

			hideCalltip();
		}
	}

	if (e.RightDown() || e.LeftDown())
		hideCalltip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onFocusLoss
//
// Called when the text editor loses focus
// ----------------------------------------------------------------------------
void TextEditorCtrl::onFocusLoss(wxFocusEvent& e)
{
	// Hide calltip+autocomplete box
	hideCalltip();
	AutoCompCancel();

	// Hide current line marker
	MarkerDeleteAll(1);
	MarkerDeleteAll(2);

	// Clear word matches
	SetIndicatorCurrent(8);
	IndicatorClearRange(0, GetTextLength());
	prev_word_match_ = "";

	e.Skip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onActivate
//
// Called when the text editor is activated/deactivated
// ----------------------------------------------------------------------------
void TextEditorCtrl::onActivate(wxActivateEvent& e)
{
	if (!e.GetActive())
		hideCalltip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onMarginClick
//
// Called when a margin is clicked
// ----------------------------------------------------------------------------
void TextEditorCtrl::onMarginClick(wxStyledTextEvent& e)
{
	if (e.GetMargin() == 1)
	{
		int line = LineFromPosition(e.GetPosition());
		int level = GetFoldLevel(line);
		if ((level & wxSTC_FOLDLEVELHEADERFLAG) > 0)
			ToggleFold(line);
	}
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onJumpToCalculateComplete
//
// Called when the 'Jump To' calculation thread completes
// ----------------------------------------------------------------------------
void TextEditorCtrl::onJumpToCalculateComplete(wxThreadEvent& e)
{
	if (!choice_jump_to_)
	{
		jump_to_calculator_ = nullptr;
		return;
	}

	choice_jump_to_->Clear();
	jump_to_lines_.clear();

	string jump_points = e.GetString();
	wxArrayString split = wxSplit(jump_points, ',');

	wxArrayString items;
	for (unsigned a = 0; a < split.size(); a += 2)
	{
		if (a == split.size() - 1)
			break;

		long line;
		if (!split[a].ToLong(&line))
			line = 0;
		string name = split[a + 1];

		items.push_back(name);
		jump_to_lines_.push_back(line);
	}

	choice_jump_to_->Append(items);
	choice_jump_to_->Enable(true);

	jump_to_calculator_ = nullptr;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onJumpToChoiceSelected
//
// Called when the 'Jump To' dropdown is changed
// ----------------------------------------------------------------------------
void TextEditorCtrl::onJumpToChoiceSelected(wxCommandEvent& e)
{
	// Move to line
	int line = jump_to_lines_[choice_jump_to_->GetSelection()];
	int pos = GetLineEndPosition(line);
	SetCurrentPos(pos);
	SetSelection(pos, pos);
	SetFirstVisibleLine(line);
	SetFocus();
	choice_jump_to_->SetSelection(-1);
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onModified
//
// Called when the text is modified
// ----------------------------------------------------------------------------
void TextEditorCtrl::onModified(wxStyledTextEvent& e)
{
	// (Re)start update timer for jump to list if text has changed
	if (prev_text_length_ != GetTextLength())
	{
		last_modified_ = App::runTimer();
		update_jump_to_ = true;
		timer_update_.Start(1000, true);

		// Send change event
		wxCommandEvent event(wxEVT_TEXT_CHANGED);
		wxPostEvent(this, event);
	}

	e.Skip();
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onUpdateTimer
//
// Called when the update timer finishes
// ----------------------------------------------------------------------------
void TextEditorCtrl::onUpdateTimer(wxTimerEvent& e)
{
	if (update_jump_to_)
		updateJumpToList();
	if (update_word_match_)
		matchWord();

	update_jump_to_ = false;
	update_word_match_ = false;
}

// ----------------------------------------------------------------------------
// TextEditorCtrl::onStyleNeeded
//
// Called when text styling is needed
// ----------------------------------------------------------------------------
void TextEditorCtrl::onStyleNeeded(wxStyledTextEvent& e)
{
	// Get range of lines to be updated
	int line_start = LineFromPosition(GetEndStyled());
	int line_end = LineFromPosition(e.GetPosition());

	// Lex until done (end of lines, end of file or end of block comment)
	int l = line_start;
	bool force_next = false;
	while (l <= GetNumberOfLines() && (l <= line_end || force_next))
	{
		int end = GetLineEndPosition(l) - 1;
		int start = end - GetLineLength(l) + 1;

		if (start > end)
			end = start;

		force_next = lexer_->doStyling(this, start, end);
		l++;
	}

	if (txed_fold_enable)
	{
		auto modified = last_modified_;
		lexer_->updateFolding(this, line_start);
		last_modified_ = modified;
	}
}
