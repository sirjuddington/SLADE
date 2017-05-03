
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Lexer.cpp
 * Description: A lexer class to handle syntax highlighting and code
 *              folding for the text editor
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
#include "Lexer.h"
#include "TextEditor.h"
#include "TextLanguage.h"


/*******************************************************************
 * LEXER CLASS FUNCTIONS
 *******************************************************************/

/* Lexer::Lexer
 * Lexer class constructor
 *******************************************************************/
Lexer::Lexer() :
	language{ nullptr },
	re_int1{ "^[+-]?[0-9]+[0-9]*$", wxRE_DEFAULT|wxRE_NOSUB },
	re_int2{ "^0[0-9]+$", wxRE_DEFAULT|wxRE_NOSUB },
	re_int3{ "^0x[0-9A-Fa-f]+$", wxRE_DEFAULT|wxRE_NOSUB },
	re_float{ "^[-+]?[0-9]*.?[0-9]+([eE][-+]?[0-9]+)?$", wxRE_DEFAULT|wxRE_NOSUB },
	whitespace_chars{ { ' ', '\n', '\r', '\t' } },
	fold_comments{ false },
	fold_preprocessor{ false }
{
	// Default word characters
	setWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");

	// Default operator characters
	setOperatorChars("+-*/=><|~&!");
}

/* Lexer::loadLanguage
 * Loads settings and word lists from [language]
 *******************************************************************/
void Lexer::loadLanguage(TextLanguage* language)
{
	this->language = language;
	clearWords();

	if (!language)
		return;

	// Load language words
	for (auto word : language->getWordListSorted(TextLanguage::WordType::Constant))
		addWord(word, Lexer::Style::Constant);
	for (auto word : language->getWordListSorted(TextLanguage::WordType::Property))
		addWord(word, Lexer::Style::Property);
	for (auto word : language->getFunctionsSorted())
		addWord(word, Lexer::Style::Function);
	for (auto word : language->getWordListSorted(TextLanguage::WordType::Type))
		addWord(word, Lexer::Style::Type);
	for (auto word : language->getWordListSorted(TextLanguage::WordType::Keyword))
		addWord(word, Lexer::Style::Keyword);
}

/* Lexer::doStyling
 * Performs text styling on [editor], for characters from [start] to
 * [end]. Returns true if the next line needs to be styled (eg. for
 * multi-line comments)
 *******************************************************************/
bool Lexer::doStyling(TextEditor* editor, int start, int end)
{
	if (start < 0)
		start = 0;

	int line = editor->LineFromPosition(start);
	LexerState state
	{
		start,
		end,
		line,
		lines[line].commented ? State::Comment : State::Unknown,
		0,
		0,
		false,
		editor
	};

	editor->StartStyling(start, 31);
	LOG_MESSAGE(3, "START STYLING FROM %d TO %d (LINE %d)", start, end, line + 1);

	bool done = false;
	while (!done)
	{
		switch (state.state)
		{
		case State::Whitespace:
			done = processWhitespace(state); break;
		case State::Comment:
			done = processComment(state); break;
		case State::String:
			done = processString(state); break;
		case State::Char:
			done = processChar(state); break;
		case State::Word:
			done = processWord(state); break;
		case State::Operator:
			done = processOperator(state); break;
		default:
			done = processUnknown(state); break;
		}
	}

	// Set current & next line's info
	lines[line].fold_increment = state.fold_increment;
	lines[line].has_word = state.has_word;
	lines[line+1].commented = (state.state == State::Comment);

	// Return true if we are still inside a comment
	return (state.state == State::Comment);
}

/* Lexer::addWord
 * Sets the [style] for [word]
 *******************************************************************/
void Lexer::addWord(string word, int style)
{
	word_list[word.Lower()].style = style;
}

/* Lexer::styleWord
 * Applies a style to [word] in [editor], depending on if it is in
 * the word list, a number or begins with the preprocessor character
 *******************************************************************/
void Lexer::styleWord(TextEditor* editor, string word)
{
	string wl = word.Lower();

	if (word_list[wl].style > 0)
		editor->SetStyling(word.length(), word_list[wl].style);
	else if (word.StartsWith(language->getPreprocessor()))
		editor->SetStyling(word.length(), Style::Preprocessor);
	else
	{
		// Check for number
		if (re_int2.Matches(word) || re_int1.Matches(word) || re_float.Matches(word) || re_int3.Matches(word))
			editor->SetStyling(word.length(), Style::Number);
		else
			editor->SetStyling(word.length(), Style::Default);
	}
}

/* Lexer::setWordChars
 * Sets the valid word characters to [chars]
 *******************************************************************/
void Lexer::setWordChars(string chars)
{
	word_chars.clear();
	for (unsigned a = 0; a < chars.length(); a++)
		word_chars.push_back(chars[a]);
}

/* Lexer::setOperatorChars
 * Sets the valid operator characters to [chars]
 *******************************************************************/
void Lexer::setOperatorChars(string chars)
{
	operator_chars.clear();
	for (unsigned a = 0; a < chars.length(); a++)
		operator_chars.push_back(chars[a]);
}

/* Lexer::processUnknown
 * Process unknown characters, updating [state]. Returns true if the
 * end of the current text range was reached
 *******************************************************************/
bool Lexer::processUnknown(LexerState& state)
{
	int u_length = 0;
	bool end = false;
	bool pp = false;
	string comment_begin;
	string comment_doc;
	string comment_line;
	string block_begin;
	string block_end;

	if (language)
	{
		comment_begin = language->getCommentBegin();
		comment_doc = language->getDocComment();
		comment_line = language->getLineComment();
		block_begin = language->getBlockBegin();
		block_end = language->getBlockEnd();
	}
	else {
		comment_begin = "";
		comment_doc = "";
		comment_line = "";
		block_begin = "";
		block_end = "";
	}

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}
		
		char c = state.editor->GetCharAt(state.position);

		// Start of string
		if (c == '"')
		{
			state.state = State::String;
			state.position++;
			state.length = 1;
			state.has_word = true;
			break;
		}

		// No language set, only process strings
		else if (!language)
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
			state.length = 1;
			state.has_word = true;
			break;
		}

		// Start of doc line comment
		else if (checkToken(state.editor, state.position, comment_doc))
		{
			// Format as comment to end of line
			state.editor->SetStyling(u_length, Style::Default);
			state.editor->SetStyling((state.end - state.position) + 1, Style::CommentDoc);
			return true;
		}

		// Start of line comment
		else if (checkToken(state.editor, state.position, comment_line))
		{
			// Format as comment to end of line
			state.editor->SetStyling(u_length, Style::Default);
			state.editor->SetStyling((state.end - state.position) + 1, Style::Comment);
			return true;
		}

		// Start of block comment
		else if (checkToken(state.editor, state.position, comment_begin))
		{
			state.state = State::Comment;
			state.position += comment_begin.size();
			state.length = comment_begin.size();
			if (fold_comments)
			{
				state.fold_increment++;
				state.has_word = true;
			}
			break;
		}

		// Whitespace
		else if (VECTOR_EXISTS(whitespace_chars, c))
		{
			state.state = State::Whitespace;
			state.position++;
			state.length = 1;
			break;
		}

		// Preprocessor
		else if (c == language->getPreprocessor()[0])
		{
			pp = true;
			u_length++;
			state.position++;
			continue;
		}

		// Operator
		else if (VECTOR_EXISTS(operator_chars, c))
		{
			state.position++;
			state.state = State::Operator;
			state.length = 1;
			state.has_word = true;
			break;
		}

		// Word
		else if (VECTOR_EXISTS(word_chars, c))
		{
			// Include preprocessor character if it was the previous character
			if (pp)
			{
				state.position--;
				u_length--;
			}

			state.state = State::Word;
			state.length = 0;
			state.has_word = true;
			break;
		}

		// Block begin
		else if (checkToken(state.editor, state.position, block_begin))
		{
			state.fold_increment++;
		}

		// Block end
		else if (checkToken(state.editor, state.position, block_end))
		{
			state.fold_increment--;
		}

		//LOG_MESSAGE(4, "unknown char '%c' (%d)", c, c);
		u_length++;
		state.position++;
		pp = false;
	}

	//LOG_MESSAGE(4, "unknown:%d", u_length);
	state.editor->SetStyling(u_length, Style::Default);

	return end;
}

/* Lexer::processComment
 * Process comment characters, updating [state]. Returns true if the
 * end of the current text range was reached
 *******************************************************************/
bool Lexer::processComment(LexerState& state)
{
	bool end = false;
	string comment_block_end = language ? language->getCommentEnd() : "";

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of comment
		if (checkToken(state.editor, state.position, comment_block_end))
		{
			state.length += comment_block_end.size();
			state.position += comment_block_end.size();
			state.state = State::Unknown;
			if (fold_comments)
				state.fold_increment--;
			break;
		}

		state.length++;
		state.position++;
	}

	LOG_MESSAGE(4, "comment:%d", state.length);
	state.editor->SetStyling(state.length, Style::Comment);

	return end;
}

/* Lexer::processWord
 * Process word characters, updating [state]. Returns true if the
 * end of the current text range was reached
 *******************************************************************/
bool Lexer::processWord(LexerState& state)
{
	vector<char> word;
	bool end = false;

	// Add first letter
	word.push_back(state.editor->GetCharAt(state.position++));

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(word_chars, c))
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
	string word_string = wxString::FromAscii(&word[0], word.size());

	// Check for preprocessor folding word
	if (fold_preprocessor)
	{
		char preprocessor = language->getPreprocessor()[0];
		if (word_string.StartsWith(preprocessor))
		{
			string word_lower = word_string.Lower().After(preprocessor);
			if (VECTOR_EXISTS(language->getPPBlockBegin(), word_lower))
				state.fold_increment++;
			else if (VECTOR_EXISTS(language->getPPBlockEnd(), word_lower))
				state.fold_increment--;
		}
	}

	LOG_MESSAGE(4, "word:%s", word_string);
	styleWord(state.editor, word_string);

	return end;
}

/* Lexer::processString
 * Process string characters, updating [state]. Returns true if the
 * end of the current text range was reached
 *******************************************************************/
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
		char c = state.editor->GetCharAt(state.position);
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

	LOG_MESSAGE(4, "string:%d", state.length);
	state.editor->SetStyling(state.length, Style::String);

	return end;
}

/* Lexer::processChar
 * Process char characters, updating [state]. Returns true if the
 * end of the current text range was reached
 *******************************************************************/
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
		char c = state.editor->GetCharAt(state.position);
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

	LOG_MESSAGE(4, "char:%d", state.length);
	state.editor->SetStyling(state.length, Style::Char);

	return end;
}

/* Lexer::processOperator
 * Process operator characters, updating [state]. Returns true if the
 * end of the current text range was reached
 *******************************************************************/
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

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(operator_chars, c))
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

	LOG_MESSAGE(4, "operator:%d", state.length);
	state.editor->SetStyling(state.length, Style::Operator);

	return end;
}

/* Lexer::processWhitespace
 * Process whitespace characters, updating [state]. Returns true if 
 * the end of the current text range was reached
 *******************************************************************/
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

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(whitespace_chars, c))
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

	LOG_MESSAGE(4, "whitespace:%d", state.length);
	state.editor->SetStyling(state.length, Style::Default);

	return end;
}

/* Lexer::checkToken
 * Checks if the text in [editor] starting from [pos] matches [token]
 *******************************************************************/
bool Lexer::checkToken(TextEditor* editor, int pos, string& token)
{
	if (!token.empty())
	{
		size_t token_size = token.size();
		for (unsigned a = 0; a < token_size; a++)
			if (editor->GetCharAt(pos + a) != (int)token[a])
				return false;

		return true;
	}

	return false;
}

/* Lexer::updateFolding
 * Updates code folding levels in [editor], starting from line
 * [line_start]
 *******************************************************************/
void Lexer::updateFolding(TextEditor* editor, int line_start)
{
	int fold_level = editor->GetFoldLevel(line_start) & wxSTC_FOLDLEVELNUMBERMASK;

	for (int l = line_start; l < editor->GetLineCount(); l++)
	{
		// Determine next line's fold level
		int next_level = fold_level + lines[l].fold_increment;
		if (next_level < wxSTC_FOLDLEVELBASE)
			next_level = wxSTC_FOLDLEVELBASE;

		// Check if we are going up a fold level
		if (next_level > fold_level)
		{
			if (!lines[l].has_word)
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
