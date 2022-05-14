
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Lexer.cpp
// Description: A lexer class to handle syntax highlighting and code folding
//              for the text editor
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
#include "Lexer.h"
#include "UI/TextEditorCtrl.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, debug_lexer, false, CVar::Flag::Secret)


// -----------------------------------------------------------------------------
//
// Lexer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Lexer class constructor
// -----------------------------------------------------------------------------
Lexer::Lexer() :
	whitespace_chars_{ { ' ', '\n', '\r', '\t' } },
	re_int1_{ "^[+-]?[0-9]+[0-9]*$", wxRE_DEFAULT | wxRE_NOSUB },
	re_int2_{ "^0[0-9]+$", wxRE_DEFAULT | wxRE_NOSUB },
	re_int3_{ "^0x[0-9A-Fa-f]+$", wxRE_DEFAULT | wxRE_NOSUB },
	re_float_{ "^[-+]?[0-9]*.?[0-9]+([eE][-+]?[0-9]+)?$", wxRE_DEFAULT | wxRE_NOSUB }
{
	// Default word characters
	setWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");

	// Default operator characters
	setOperatorChars("+-*/=><|~&!");
}

// -----------------------------------------------------------------------------
// Loads settings and word lists from [language]
// -----------------------------------------------------------------------------
void Lexer::loadLanguage(TextLanguage* language)
{
	language_ = language;
	clearWords();
	comment_blocks_.clear();

	if (!language)
		return;

	// Load language words
	for (const auto& word : language->wordListSorted(TextLanguage::WordType::Constant))
		addWord(word, Lexer::Style::Constant);
	for (const auto& word : language->wordListSorted(TextLanguage::WordType::Property))
		addWord(word, Lexer::Style::Property);
	for (const auto& word : language->functionsSorted())
		addWord(word, Lexer::Style::Function);
	for (const auto& word : language->wordListSorted(TextLanguage::WordType::Type))
		addWord(word, Lexer::Style::Type);
	for (const auto& word : language->wordListSorted(TextLanguage::WordType::Keyword))
		addWord(word, Lexer::Style::Keyword);

	// Load language info
	preprocessor_char_ = language->preprocessor().empty() ? (char)0 : (char)language->preprocessor()[0];
}

// -----------------------------------------------------------------------------
// Performs text styling on [editor], for characters from [start] to [end].
// Returns true if the next line needs to be styled (eg. for multi-line
// comments)
// ----------------------------------------------------------------------------
void Lexer::doStyling(TextEditorCtrl* editor, int start, int end)
{
	if (start < 0)
		start = 0;

	int        line = editor->LineFromPosition(start);
	LexerState state{ start, end, line, State::Unknown, 0, 0, false, editor };

	editor->StartStyling(start);

	if (debug_lexer)
		log::debug(wxString::Format("START STYLING FROM %d TO %d (LINE %d)", start, end, line + 1));

	bool done = false;
	while (!done)
	{
		auto cb = isWithinComment(state.position);
		if (cb >= 0)
		{
			editor->SetStyling(comment_blocks_[cb].end_pos - state.position, Style::Comment);
			state.position = comment_blocks_[cb].end_pos;
			state.line     = editor->LineFromPosition(state.position);
			state.state    = State::Unknown;
			continue;
		}

		switch (state.state)
		{
		case State::Whitespace: done = processWhitespace(state); break;
		case State::String: done = processString(state); break;
		case State::Char: done = processChar(state); break;
		case State::Word: done = processWord(state); break;
		case State::Operator: done = processOperator(state); break;
		default: done = processUnknown(state); break;
		}
	}

	// Set current & next line's info
	lines_[line].fold_increment = state.fold_increment;
	lines_[line].has_word       = state.has_word;
}

// ----------------------------------------------------------------------------
// Updates and styles comments in [editor], for characters from [start] to
// [end].
// ----------------------------------------------------------------------------
void Lexer::updateComments(TextEditorCtrl* editor, int start, int end)
{
	if (!language_)
		return;

	// Block comment handling
	auto& block_begin = language_->commentBeginL();
	auto& block_end   = language_->commentEndL();
	int   token_index;

	// Extend start/end if either is within a comment
	auto cb = isWithinComment(start);
	if (cb >= 0)
		start = comment_blocks_[cb].start_pos;
	cb = isWithinComment(end);
	if (cb >= 0)
		end = comment_blocks_[cb].end_pos;

	// Remove any existing comment blocks within start->end
	for (int i = comment_blocks_.size() - 1; i >= 0; --i)
	{
		if (comment_blocks_[i].start_pos >= start && comment_blocks_[i].end_pos <= end)
			comment_blocks_.erase(comment_blocks_.begin() + i);
	}

	// Do not look for comments if no language is loaded
	if (!language_)
		return;

	// Scan text
	auto pos = start;
	while (pos < end)
	{
		// Skip quoted strings
		if (editor->GetCharAt(pos) == '\"')
		{
			while (pos++ < end)
				if (editor->GetCharAt(pos) == '\"')
					break;

			++pos;
			continue;
		}

		// Line comment
		if (checkToken(editor, pos, language_->lineCommentL()))
		{
			const auto l_end = editor->GetLineEndPosition(editor->LineFromPosition(pos)) + 1;
			comment_blocks_.push_back({ pos, l_end });
			pos = l_end;
			continue;
		}

		// Block comment
		if (checkToken(editor, pos, block_begin, &token_index))
		{
			auto& end_token = block_end[token_index];
			auto  cb_start  = pos;
			pos += block_begin[token_index].size();
			while (pos < end)
			{
				if (checkToken(editor, pos, end_token))
				{
					pos += end_token.size();
					break;
				}
				++pos;
			}

			comment_blocks_.push_back({ cb_start, pos });
			continue;
		}

		++pos;
	}
}

// -----------------------------------------------------------------------------
// Sets the [style] for [word]
// -----------------------------------------------------------------------------
void Lexer::addWord(string_view word, int style)
{
	word_list_[language_->caseSensitive() ? string{ word } : strutil::lower(word)].style = (char)style;
}

// -----------------------------------------------------------------------------
// Applies a style to [word] in [editor], depending on if it is in the word
// list, a number or begins with the preprocessor character
// -----------------------------------------------------------------------------
void Lexer::styleWord(LexerState& state, string_view word)
{
	string word_str{ word };
	if (!language_->caseSensitive())
		strutil::lowerIP(word_str);

	if (word_list_[word_str].style > 0)
		state.editor->SetStyling(word.length(), word_list_[word_str].style);
	else if (strutil::startsWith(word_str, language_->preprocessor()))
		state.editor->SetStyling(word.length(), Style::Preprocessor);
	else
	{
		// Check for number
		if (strutil::isInteger(word_str) || strutil::isFloat(word_str))
			state.editor->SetStyling(word.length(), Style::Number);
		else
			state.editor->SetStyling(word.length(), Style::Default);
	}
}

// -----------------------------------------------------------------------------
// Sets the valid word characters to [chars]
// -----------------------------------------------------------------------------
void Lexer::setWordChars(string_view chars)
{
	word_chars_.clear();
	for (auto&& a : chars)
		word_chars_.push_back((unsigned char)a);
}

// -----------------------------------------------------------------------------
// Sets the valid operator characters to [chars]
// -----------------------------------------------------------------------------
void Lexer::setOperatorChars(string_view chars)
{
	operator_chars_.clear();
	for (auto&& a : chars)
		operator_chars_.push_back((unsigned char)a);
}

// -----------------------------------------------------------------------------
// Process unknown characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processUnknown(LexerState& state)
{
	int    u_length = 0;
	bool   end      = false;
	bool   pp       = false;
	string block_begin;
	string block_end;

	if (language_)
	{
		block_begin = language_->blockBegin();
		block_end   = language_->blockEnd();
	}

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		int c = state.editor->GetCharAt(state.position);

		// Start of string
		if (c == '"')
		{
			state.state = State::String;
			state.position++;
			state.length   = 1;
			state.has_word = true;
			break;
		}

		// No language set, only process strings
		else if (!language_)
		{
			u_length++;
			state.position++;
			continue;
		}

		// Start of char
		else if (c == '\'')
		{
			state.state = State::Char;
			state.position++;
			state.length   = 1;
			state.has_word = true;
			break;
		}

		// Whitespace
		else if (VECTOR_EXISTS(whitespace_chars_, c))
		{
			state.state = State::Whitespace;
			state.position++;
			state.length = 1;
			break;
		}

		// Preprocessor
		else if (c == (unsigned char)language_->preprocessor()[0])
		{
			pp = true;
			u_length++;
			state.position++;
			continue;
		}

		// Operator
		else if (VECTOR_EXISTS(operator_chars_, c))
		{
			state.position++;
			state.state    = State::Operator;
			state.length   = 1;
			state.has_word = true;
			break;
		}

		// Word
		else if (VECTOR_EXISTS(word_chars_, c))
		{
			// Include preprocessor character if it was the previous character
			if (pp)
			{
				state.position--;
				u_length--;
			}

			state.state    = State::Word;
			state.length   = 0;
			state.has_word = true;
			break;
		}

		// Block begin
		else if (checkToken(state.editor, state.position, block_begin))
			state.fold_increment++;

		// Block end
		else if (checkToken(state.editor, state.position, block_end))
			state.fold_increment--;

		// if (debug_lexer)
		// 	log::debug(wxString::Format("unknown char '%c' (%d)", c, c));
		u_length++;
		state.position++;
		pp = false;
	}

	if (debug_lexer && u_length > 0)
		log::debug(wxString::Format("unknown: %d", u_length));
	state.editor->SetStyling(u_length, Style::Default);

	return end;
}

// ----------------------------------------------------------------------------
// Process word characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processWord(LexerState& state)
{
	vector<char> word;
	bool         end = false;

	// Add first letter
	word.push_back((char)state.editor->GetCharAt(state.position++));

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = (char)state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(word_chars_, c))
		{
			word.push_back(c);
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	// Get word as string
	string word_string{ &word[0], word.size() };
	auto   word_lower = strutil::lower(word_string);

	// Check for preprocessor folding word
	if (fold_preprocessor_ && word[0] == preprocessor_char_)
	{
		if (VECTOR_EXISTS(language_->ppBlockBegin(), word_lower))
			state.fold_increment++;
		else if (VECTOR_EXISTS(language_->ppBlockEnd(), word_lower))
			state.fold_increment--;
	}
	else
	{
		if (VECTOR_EXISTS(language_->wordBlockBegin(), word_lower))
			state.fold_increment++;
		else if (VECTOR_EXISTS(language_->wordBlockEnd(), word_lower))
			state.fold_increment--;
	}

	if (debug_lexer)
		log::debug("word: {}", word_string);

	styleWord(state, word_string);

	return end;
}

// -----------------------------------------------------------------------------
// Process string characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processString(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of string
		char c = (char)state.editor->GetCharAt(state.position);
		if (c == '"')
		{
			state.length++;
			state.position++;
			state.state = State::Unknown;
			break;
		}

		state.length++;
		state.position++;
	}

	if (debug_lexer)
		log::debug(wxString::Format("string: %lu", state.length));

	state.editor->SetStyling(state.length, Style::String);

	return end;
}

// -----------------------------------------------------------------------------
// Process char characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processChar(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of string
		char c = (char)state.editor->GetCharAt(state.position);
		if (c == '\'')
		{
			state.length++;
			state.position++;
			state.state = State::Unknown;
			break;
		}

		state.length++;
		state.position++;
	}

	if (debug_lexer)
		log::debug(wxString::Format("char: %lu", state.length));

	state.editor->SetStyling(state.length, Style::Char);

	return end;
}

// -----------------------------------------------------------------------------
// Process operator characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processOperator(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = (char)state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(operator_chars_, c))
		{
			state.length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	if (debug_lexer)
		log::debug(wxString::Format("operator: %lu", state.length));

	state.editor->SetStyling(state.length, Style::Operator);

	return end;
}

// -----------------------------------------------------------------------------
// Process whitespace characters, updating [state].
// Returns true if the end of the current text range was reached
// -----------------------------------------------------------------------------
bool Lexer::processWhitespace(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = (char)state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(whitespace_chars_, c))
		{
			state.length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	if (debug_lexer)
		log::debug(wxString::Format("whitespace: %lu", state.length));

	state.editor->SetStyling(state.length, Style::Default);

	return end;
}

// -----------------------------------------------------------------------------
// Checks if the text in [editor] starting from [pos] matches [token]
// ----------------------------------------------------------------------------
bool Lexer::checkToken(TextEditorCtrl* editor, int pos, string_view token) const
{
	if (!token.empty())
	{
		unsigned long token_size = token.size();
		for (unsigned i = 0; i < token_size; i++)
		{
			if (editor->GetCharAt(pos + i) != (int)token[i])
				return false;
		}
		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
// Checks if the text in [editor] starting from [pos] is present in [tokens].
// Writes the fitst index that matched to [found_index] if a valid pointer
// is passed. Returns true if there's a match, false if not.
// ----------------------------------------------------------------------------
bool Lexer::checkToken(TextEditorCtrl* editor, int pos, const vector<string>& tokens, int* found_idx) const
{
	if (!tokens.empty())
	{
		unsigned idx = 0;
		string   token;
		while (idx < tokens.size())
		{
			token = tokens[idx];
			if (checkToken(editor, pos, token))
			{
				if (found_idx)
					*found_idx = idx;
				return true;
			}
			else
				idx++;
		}
	}
	return false;
}

// ----------------------------------------------------------------------------
// Checks if [pos] is within a block comment, and returns the index for
// comment_blocks_ if it is (-1 otherwise)
// ----------------------------------------------------------------------------
int Lexer::isWithinComment(int pos)
{
	for (unsigned i = 0; i < comment_blocks_.size(); ++i)
		if (pos >= comment_blocks_[i].start_pos && pos < comment_blocks_[i].end_pos)
			return i;

	return -1;
}

// ---------------------------------------------------------------------------
// Updates code folding levels in [editor], starting from line [line_start]
// -----------------------------------------------------------------------------
void Lexer::updateFolding(TextEditorCtrl* editor, int line_start)
{
	int fold_level = editor->GetFoldLevel(line_start) & wxSTC_FOLDLEVELNUMBERMASK;

	for (int l = line_start; l < editor->GetLineCount(); l++)
	{
		// Determine next line's fold level
		int next_level = fold_level + lines_[l].fold_increment;
		if (next_level < wxSTC_FOLDLEVELBASE)
			next_level = wxSTC_FOLDLEVELBASE;

		// Check if we are going up a fold level
		if (next_level > fold_level)
		{
			if (!lines_[l].has_word)
			{
				// Line doesn't have any words (eg. only has an opening brace),
				// move the fold header up a line
				editor->SetFoldLevel(l - 1, fold_level | wxSTC_FOLDLEVELHEADERFLAG);
				editor->SetFoldLevel(l, next_level);
			}
			else
				editor->SetFoldLevel(l, fold_level | wxSTC_FOLDLEVELHEADERFLAG);
		}
		else
			editor->SetFoldLevel(l, fold_level);

		fold_level = next_level;
	}
}

// -----------------------------------------------------------------------------
// Returns true if the word from [start_pos] to [end_pos] in [editor] is a
// function
// -----------------------------------------------------------------------------
bool Lexer::isFunction(TextEditorCtrl* editor, int start_pos, int end_pos)
{
	auto word = editor->GetTextRange(start_pos, end_pos).ToStdString();
	if (!language_->caseSensitive())
		strutil::lowerIP(word);
	return word_list_[word].style == (int)Style::Function;
}


// -----------------------------------------------------------------------------
//
// ZScriptLexer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Sets the [style] for [word], or adds it to the functions list if [style]
// is Function
// -----------------------------------------------------------------------------
void ZScriptLexer::addWord(string_view word, int style)
{
	if (style == Style::Function)
		functions_.push_back(language_->caseSensitive() ? string{ word } : strutil::lower(word));
	else
		Lexer::addWord(word, style);
}

// -----------------------------------------------------------------------------
// ZScript version of Lexer::styleWord - functions require a following '('
// -----------------------------------------------------------------------------
void ZScriptLexer::styleWord(LexerState& state, string_view word)
{
	// Skip whitespace after word
	auto index = state.position;
	while (index < state.end)
	{
		if (!(VECTOR_EXISTS(whitespace_chars_, state.editor->GetCharAt(index))))
			break;
		++index;
	}

	// Check for '(' (possible function)
	if (state.editor->GetCharAt(index) == '(')
	{
		string word_str{ word };
		if (!language_->caseSensitive())
			strutil::lowerIP(word_str);

		if (VECTOR_EXISTS(functions_, word_str))
		{
			state.editor->SetStyling(word.length(), Style::Function);
			return;
		}
	}

	Lexer::styleWord(state, word);
}

// -----------------------------------------------------------------------------
// Clears out all defined words
// -----------------------------------------------------------------------------
void ZScriptLexer::clearWords()
{
	functions_.clear();
	Lexer::clearWords();
}

// -----------------------------------------------------------------------------
// Returns true if the word from [start_pos] to [end_pos] in [editor] is a
// function
// -----------------------------------------------------------------------------
bool ZScriptLexer::isFunction(TextEditorCtrl* editor, int start_pos, int end_pos)
{
	// Check for '(' after word

	// Skip whitespace
	auto index = end_pos;
	auto end   = editor->GetTextLength();
	while (index < end)
	{
		if (!(VECTOR_EXISTS(whitespace_chars_, editor->GetCharAt(index))))
			break;
		++index;
	}
	if (editor->GetCharAt(index) != '(')
		return false;

	// Check if word is a function name
	auto word = editor->GetTextRange(start_pos, end_pos).ToStdString();
	if (!language_->caseSensitive())
		strutil::lowerIP(word);
	return VECTOR_EXISTS(functions_, word);
}
