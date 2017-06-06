
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Tokenizer.cpp
// Description: A string tokenizer
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
#include "Tokenizer.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
const string Tokenizer::DEFAULT_SPECIAL_CHARACTERS = ";,:|={}/";
Tokenizer::Token Tokenizer::invalid_token_{ "", 0, false, 0, 0 };


// ----------------------------------------------------------------------------
//
// Local Functions
//
// ----------------------------------------------------------------------------
namespace
{
	// ------------------------------------------------------------------------
	// isWhitespace
	//
	// Returns true if [p] is a whitespace character
	// ------------------------------------------------------------------------
	bool isWhitespace(char p)
	{
		// Whitespace is either a newline, tab character or space
		if (p == '\n' || p == 13 || p == ' ' || p == '\t')
			return true;
		else
			return false;
	}
}


// ----------------------------------------------------------------------------
//
// Tokenizer Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Tokenizer::Tokenizer
//
// Tokenizer class constructor
// ----------------------------------------------------------------------------
Tokenizer::Tokenizer(CommentTypes comments, const string& special_characters) :
	comment_types_{ comments },
	special_characters_{special_characters.begin(), special_characters.end()},
	decorate_{ false },
	case_sensitive_{ true }
{
}

// ----------------------------------------------------------------------------
// Tokenizer::next
//
// Returns the 'next' token and advances to it if [inc_index] is true
// ----------------------------------------------------------------------------
const Tokenizer::Token& Tokenizer::next(bool inc_index)
{
	if (token_next_.pos_start == token_current_.pos_start)
		return invalid_token_;

	if (inc_index)
	{
		token_current_ = token_next_;
		readNext();
		return token_current_;
	}

	return token_next_;
}

// ----------------------------------------------------------------------------
// Tokenizer::skip
//
// Skips [inc] tokens
// ----------------------------------------------------------------------------
void Tokenizer::skip(int inc)
{
	if (inc <= 0)
		return;

	for (int a = 0; a < inc - 1; a++)
		readNext();

	token_current_ = token_next_;
	readNext();
}

// ----------------------------------------------------------------------------
// Tokenizer::skipToNextLine
//
// Skips to the first token on the next line
// ----------------------------------------------------------------------------
void Tokenizer::skipToNextLine()
{
	// Ignore if we're on the last token already
	if (token_next_.pos_start == token_current_.pos_start)
		return;

	// If the next token is on the next line just move to it
	if (token_next_.line_no > token_current_.line_no)
	{
		token_current_ = token_next_;
		readNext();
		return;
	}

	// Otherwise skip until the line increments or we reach the last token
	unsigned line = token_current_.line_no;
	while (token_next_.pos_start > token_current_.pos_start)
	{
		readNext(&token_current_);

		if (token_current_.line_no > line)
		{
			readNext();
			return;
		}
	}

	// If we got to the end, update the 'next' token
	token_next_ = token_current_;
}

// ----------------------------------------------------------------------------
// Tokenizer::setSpecialCharacters
//
// Sets the special characters to each character in [special_characters].
// Special characters are always read as a single token
// ----------------------------------------------------------------------------
void Tokenizer::setSpecialCharacters(const char* characters)
{
	special_characters_.assign(characters, characters + strlen(characters));
}

// ----------------------------------------------------------------------------
// Tokenizer::openFile
//
// Opens text from a file [filename], reading [length] bytes from [offset].
// If [length] is 0, read to the end of the file
// ----------------------------------------------------------------------------
bool Tokenizer::openFile(const string& filename, unsigned offset, unsigned length)
{
	// Open the file
	wxFile file(filename);

	// Check file opened
	if (!file.IsOpened())
	{
		Log::error(S_FMT("Tokenizer::openFile: Unable to open file %s", filename));
		return false;
	}

	// Use filename as source
	source_ = filename;

	// If length isn't specified or exceeds the file length,
	// only read to the end of the file
	if (offset + length > file.Length() || length == 0)
		length = file.Length() - offset;

	// Read the file portion
	data_.resize(length, 0);
	file.Seek(offset, wxFromStart);
	file.Read(data_.data(), length);

	reset();

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::openString
//
// Opens text from a string [text], reading [length] bytes from [offset].
// If [length] is 0, read to the end of the string
// ----------------------------------------------------------------------------
bool Tokenizer::openString(const string& text, unsigned offset, unsigned length, const string& source)
{
	source_ = source;

	// If length isn't specified or exceeds the string's length,
	// only copy to the end of the string
	string ascii = text.ToAscii();
	if (offset + length > (unsigned)ascii.length() || length == 0)
		length = (unsigned)ascii.length() - offset;

	// Copy the string portion
	data_.assign(ascii.begin() + offset, ascii.begin() + offset + length);

	reset();

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::openMem
//
// Opens text from memory [mem], reading [length] bytes
// ----------------------------------------------------------------------------
bool Tokenizer::openMem(const char* mem, unsigned length, const string& source)
{
	source_ = source;
	data_.assign(mem, mem + length);

	reset();

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::openMem
//
// Opens text from a MemChunk [mc]
// ----------------------------------------------------------------------------
bool Tokenizer::openMem(const MemChunk& mc, const string& source)
{
	source_ = source;
	data_.assign(mc.getData(), mc.getData() + mc.getSize());

	reset();

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::reset
//
// Resets the tokenizer to the beginning of the data
// ----------------------------------------------------------------------------
void Tokenizer::reset()
{
	// Init tokenizing state
	state_ = TokenizeState{};
	state_.size = data_.size();

	// Read first tokens
	readNext(&token_current_);
	readNext(&token_next_);
}

// ----------------------------------------------------------------------------
// Tokenizer::checkCommentBegin
//
// Checks if a comment begins at the current position and returns the comment
// type if one does (0 otherwise)
// ----------------------------------------------------------------------------
unsigned Tokenizer::checkCommentBegin()
{
	// C-Style comment (/*)
	if (comment_types_ & CStyle &&
		state_.position + 1 < state_.size &&
		data_[state_.position] == '/' &&
		data_[state_.position + 1] == '*')
		return CStyle;

	// CPP-Style comment (//)
	if (comment_types_ & CPPStyle &&
		state_.position + 1 < state_.size &&
		data_[state_.position] == '/' &&
		data_[state_.position + 1] == '/')
		return CPPStyle;

	// ## comment
	if (comment_types_ & DoubleHash &&
		state_.position + 1 < state_.size &&
		data_[state_.position] == '#' &&
		data_[state_.position + 1] == '#')
		return DoubleHash;

	// # comment
	if (comment_types_ & Hash && data_[state_.position] == '#')
		return Hash;

	// ; comment
	if (comment_types_ & Shell && data_[state_.position] == ';')
		return Shell;

	// Not a comment
	return 0;
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenizeUnknown
//
// Process the current unknown character
// ----------------------------------------------------------------------------
void Tokenizer::tokenizeUnknown()
{
	// Whitespace
	if (isWhitespace(data_[state_.position]))
	{
		state_.state = TokenizeState::State::Whitespace;
		++state_.position;
		return;
	}

	// Comment
	state_.comment_type = checkCommentBegin();
	if (state_.comment_type > 0)
	{
		state_.state = TokenizeState::State::Comment;
		if (state_.comment_type == Hash || state_.comment_type == Shell)
			++state_.position;
		else
			state_.position += 2;

		return;
	}

	// Special character
	if (isSpecialCharacter(data_[state_.position]))
	{
		// End token
		state_.current_token.line_no = state_.current_line;
		state_.current_token.quoted_string = false;
		state_.current_token.pos_start = state_.position;
		++state_.position;
		state_.done = true;
		return;
	}

	// Quoted string
	if (data_[state_.position] == '\"')
	{
		// Skip "
		++state_.position;

		// Begin token
		state_.current_token.line_no = state_.current_line;
		state_.current_token.quoted_string = true;
		state_.current_token.pos_start = state_.position;
		state_.state = TokenizeState::State::Token;

		return;
	}

	// Token
	state_.current_token.line_no = state_.current_line;
	state_.current_token.quoted_string = false;
	state_.current_token.pos_start = state_.position;
	state_.state = TokenizeState::State::Token;
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenizeToken
//
// Process the current token character
// ----------------------------------------------------------------------------
void Tokenizer::tokenizeToken()
{
	// Quoted string
	if (state_.current_token.quoted_string)
	{
		// Check for closing "
		if (data_[state_.position] == '\"')
		{
			// Skip to character after closing " and end token
			state_.state = TokenizeState::State::Unknown;
			state_.done = true;
			return;
		}

		// Escape backslash
		if (data_[state_.position] == '\\')
			++state_.position;

		// Continue token
		++state_.position;

		return;
	}

	// Check for end of token
	if (isWhitespace(data_[state_.position]) ||			// Whitespace
		isSpecialCharacter(data_[state_.position]) ||	// Special character
		checkCommentBegin() > 0)						// Comment
	{
		// End token
		state_.state = TokenizeState::State::Unknown;
		state_.done = true;

		return;
	}

	// Continue token
	++state_.position;
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenizeComment
//
// Process the current comment character
// ----------------------------------------------------------------------------
void Tokenizer::tokenizeComment()
{
	// Check for decorate //$
	if (decorate_ && state_.comment_type == CPPStyle)
	{
		if (data_[state_.position] == '$' &&
			data_[state_.position - 1] == '/' &&
			data_[state_.position - 2] == '/')
		{
			// We have a token instead
			state_.current_token.line_no = state_.current_line;
			state_.current_token.quoted_string = false;
			state_.current_token.pos_start = state_.position - 2;
			state_.state = TokenizeState::State::Token;
			return;
		}
	}

	// Check for end of line comment
	if (state_.comment_type != CStyle && data_[state_.position] == '\n')
	{
		state_.state = TokenizeState::State::Unknown;
		++state_.position;
		return;
	}

	// Check for end of C-Style multi line comment
	if (state_.comment_type == CStyle)
	{
		if (state_.position + 1 < state_.size &&
			data_[state_.position] == '*' &&
			data_[state_.position + 1] == '/')
		{
			state_.state = TokenizeState::State::Unknown;
			state_.position += 2;
			return;
		}
	}

	// Continue comment
	++state_.position;
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenizeWhitespace
//
// Process the current whitespace character
// ----------------------------------------------------------------------------
void Tokenizer::tokenizeWhitespace()
{
	if (isWhitespace(data_[state_.position]))
		++state_.position;
	else
		state_.state = TokenizeState::State::Unknown;
}

// ----------------------------------------------------------------------------
// Tokenizer::readNext
//
// Reads the next token from the data and writes it to [target] if specified
// ----------------------------------------------------------------------------
bool Tokenizer::readNext(Token* target)
{
	if (data_.size() == 0 || state_.position >= state_.size)
		return false;

	// Process until the end of a token or the end of the data
	state_.done = false;
	while (state_.position < state_.size && !state_.done)
	{
		// Check for newline
		if (data_[state_.position] == '\n')
			++state_.current_line;

		// Process current character depending on state
		switch (state_.state)
		{
		case TokenizeState::State::Unknown:		tokenizeUnknown(); break;
		case TokenizeState::State::Whitespace:	tokenizeWhitespace(); break;
		case TokenizeState::State::Token:		tokenizeToken(); break;
		case TokenizeState::State::Comment:		tokenizeComment(); break;
		default: ++state_.position; break;
		}
	}

	// Write to target token (if specified)
	if (target)
	{
		// How is this slower than using += in a loop as below? Just wxString things >_>
		//target->text.assign(
		//	data_.data() + state_.current_token.pos_start,
		//	state_.position - state_.current_token.pos_start
		//);
		
		target->text.Empty();
		for (unsigned a = state_.current_token.pos_start; a < state_.position; ++a)
			target->text += data_[a];

		target->line_no = state_.current_token.line_no;
		target->quoted_string = state_.current_token.quoted_string;
		target->pos_start = state_.current_token.pos_start;
		target->pos_end = state_.position;
	}

	// Skip closing " if it was a quoted string
	if (state_.current_token.quoted_string)
		++state_.position;
		
	return true;
}



// Testing

#include "General/Console/Console.h"
#include "MainEditor/MainEditor.h"
#include "Archive/ArchiveEntry.h"
#include "App.h"
#include "Utility/TokenizerOld.h"

CONSOLE_COMMAND(test_tokenizer, 0, false)
{
	auto entry = MainEditor::currentEntry();
	if (!entry)
		return;

	long num = 1;
	if (!args.empty())
		args[0].ToLong(&num);

	struct TestToken
	{
		string text;
		bool quoted_string;
		unsigned line_no;
	};

	// Tokenize it
	Tokenizer tz;
	vector<TestToken> t_new;
	//tz.setCaseSensitive(false);
	long time = App::runTimer();
	tz.openMem(entry->getMCData(), entry->getName());
	for (long a = 0; a < num; a++)
	{
		while (!tz.atEnd())
		{
			if (a == 0)
				t_new.push_back({ tz.current().text, tz.current().quoted_string, tz.current().line_no });

			tz.next(true);
		}
		tz.reset();
	}

	time = App::runTimer() - time;

	Log::info(S_FMT("Tokenize x%d took %dms", num, time));


	// Test old tokenizer also
	TokenizerOld tzo;
	vector<TestToken> t_old;
	time = App::runTimer();
	tzo.openMem(&entry->getMCData(), entry->getName());
	for (long a = 0; a < num; a++)
	{
		string token = tzo.getToken();
		while (!tzo.isAtEnd())
		{
			if (a == 0)
				t_old.push_back({ token, tzo.quotedString(), tzo.lineNo() });

			tzo.getToken(&token);
		}
		tzo.reset();
	}
	time = App::runTimer() - time;
	Log::info(S_FMT("Old Tokenize x%d took %dms", num, time));

	for (auto& token : t_new)
		Log::debug(S_FMT("%d: \"%s\"%s", token.line_no, CHR(token.text), token.quoted_string ? " (quoted)" : ""));
}
