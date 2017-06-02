
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
#include "Tokenizer2.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
const string Tokenizer2::DEFAULT_SPECIAL_CHARACTERS = ";,:|={}/";


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
Tokenizer2::Tokenizer2(
	CommentTypes comments,
	const string& special_characters
) :
	comment_types_{ comments },
	special_characters_{special_characters.begin(), special_characters.end()},
	decorate_{ false },
	case_sensitive_{ true }
{
}

// ----------------------------------------------------------------------------
// Tokenizer::setSpecialCharacters
//
// Sets the special characters to each character in [special_characters].
// Special characters are always read as a single token
// ----------------------------------------------------------------------------
void Tokenizer2::setSpecialCharacters(const string& special_characters)
{
	special_characters_.clear();
	for (auto& c : special_characters)
		special_characters_.push_back(c);
}

// ----------------------------------------------------------------------------
// Tokenizer::openFile
//
// Opens text from a file [filename], reading [length] bytes from [offset].
// If [length] is 0, read to the end of the file
// ----------------------------------------------------------------------------
bool Tokenizer2::openFile(const string& filename, unsigned offset, unsigned length)
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

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::openString
//
// Opens text from a string [text], reading [length] bytes from [offset].
// If [length] is 0, read to the end of the string
// ----------------------------------------------------------------------------
bool Tokenizer2::openString(const string& text, unsigned offset, unsigned length, const string& source)
{
	source_ = source;

	// If length isn't specified or exceeds the string's length,
	// only copy to the end of the string
	string ascii = text.ToAscii();
	if (offset + length > (unsigned)ascii.length() || length == 0)
		length = (unsigned)ascii.length() - offset;

	// Copy the string portion
	data_.assign(ascii.begin() + offset, ascii.begin() + offset + length);

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::openMem
//
// Opens text from memory [mem], reading [length] bytes
// ----------------------------------------------------------------------------
bool Tokenizer2::openMem(const char* mem, unsigned length, const string& source)
{
	source_ = source;
	data_.assign(mem, mem + length);

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::openMem
//
// Opens text from a MemChunk [mc]
// ----------------------------------------------------------------------------
bool Tokenizer2::openMem(const MemChunk& mc, const string& source)
{
	source_ = source;
	data_.assign(mc.getData(), mc.getData() + mc.getSize());

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::checkCommentBegin
//
// Checks if a comment begins at [state]'s current position and returns the
// comment type if one does (0 otherwise)
// ----------------------------------------------------------------------------
unsigned Tokenizer2::checkCommentBegin(const TokenizeState& state)
{
	// C-Style comment (/*)
	if (comment_types_ & CStyle &&
		state.position + 1 < state.size &&
		data_[state.position] == '/' &&
		data_[state.position + 1] == '*')
		return CStyle;

	// CPP-Style comment (//)
	if (comment_types_ & CPPStyle &&
		state.position + 1 < state.size &&
		data_[state.position] == '/' &&
		data_[state.position + 1] == '/')
		return CPPStyle;

	// ## comment
	if (comment_types_ & DoubleHash &&
		state.position + 1 < state.size &&
		data_[state.position] == '#' &&
		data_[state.position + 1] == '#')
		return DoubleHash;

	// # comment
	if (comment_types_ & Hash && data_[state.position] == '#')
		return Hash;

	// ; comment
	if (comment_types_ & Shell && data_[state.position] == ';')
		return Shell;

	// Not a comment
	return 0;
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenizeUnknown
//
// Process the current unknown character at [state]
// ----------------------------------------------------------------------------
void Tokenizer2::tokenizeUnknown(TokenizeState& state)
{
	// Whitespace
	if (isWhitespace(data_[state.position]))
	{
		state.state = TokenizeState::State::Whitespace;
		state.position++;
		return;
	}

	// Comment
	state.comment_type = checkCommentBegin(state);
	if (state.comment_type > 0)
	{
		state.state = TokenizeState::State::Comment;
		if (state.comment_type == Hash || state.comment_type == Shell)
			state.position++;
		else
			state.position += 2;

		return;
	}

	// Special character
	if (isSpecialCharacter(data_[state.position]))
	{
		// Add it as a token
		tokens_.push_back(
		{
			data_[state.position],
			state.current_line,
			false,
			state.position,
			state.position + 1
		});

		// Continue
		state.position++;
		return;
	}

	// Quoted string
	if (data_[state.position] == '\"')
	{
		// Skip "
		state.position++;

		// Begin token
		state.current_token.text.clear();
		state.current_token.quoted_string = true;
		state.current_token.pos_start = state.position;
		state.state = TokenizeState::State::Token;

		return;
	}

	// Token
	state.current_token.text.clear();
	state.current_token.quoted_string = false;
	state.current_token.pos_start = state.position;
	state.state = TokenizeState::State::Token;
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenizeToken
//
// Process the current token character at [state]
// ----------------------------------------------------------------------------
void Tokenizer2::tokenizeToken(TokenizeState& state)
{
	// Quoted string
	if (state.current_token.quoted_string)
	{
		// Check for closing "
		if (data_[state.position] == '\"')
		{
			// Add token
			addCurrentToken(state);

			// Skip to character after closing " and continue
			state.position++;
			state.state = TokenizeState::State::Unknown;
			return;
		}

		// Escape backslash
		if (data_[state.position] == '\\')
			state.position++;

		// Continue token
		state.position++;

		return;
	}

	// Whitespace
	if (isWhitespace(data_[state.position]))
	{
		// Add current token
		addCurrentToken(state);

		// Continue
		state.position++;
		state.state = TokenizeState::State::Whitespace;

		return;
	}

	// Special character
	if (isSpecialCharacter(data_[state.position]))
	{
		// Add current token
		addCurrentToken(state);

		// Add special character as token
		tokens_.push_back(
		{
			data_[state.position],
			state.current_line,
			false,
			state.position,
			state.position + 1
		});

		// Continue
		state.position++;
		state.state = TokenizeState::State::Unknown;

		return;
	}

	// Comment
	unsigned comment_type = checkCommentBegin(state);
	if (comment_type > 0)
	{
		// Add current token
		addCurrentToken(state);

		// Begin comment
		state.state = TokenizeState::State::Comment;
		if (state.comment_type == Hash || state.comment_type == Shell)
			state.position++;
		else
			state.position += 2;

		return;
	}

	// Continue token
	state.position++;
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenizeComment
//
// Process the current comment character at [state]
// ----------------------------------------------------------------------------
void Tokenizer2::tokenizeComment(TokenizeState& state)
{
	// Check for end of line comment
	if (state.comment_type != CStyle && data_[state.position] == '\n')
	{
		state.state = TokenizeState::State::Unknown;
		state.position++;
		return;
	}

	// Check for end of C-Style multi line comment
	else if (state.comment_type == CStyle)
	{
		if (state.position + 1 < state.size &&
			data_[state.position] == '*' &&
			data_[state.position + 1] == '/')
		{
			state.state = TokenizeState::State::Unknown;
			state.position += 2;
			return;
		}
	}

	// Continue comment
	state.position++;
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenizeWhitespace
//
// Process the current whitespace character at [state]
// ----------------------------------------------------------------------------
void Tokenizer2::tokenizeWhitespace(TokenizeState& state)
{
	if (isWhitespace(data_[state.position]))
		state.position++;
	else
		state.state = TokenizeState::State::Unknown;
}

// ----------------------------------------------------------------------------
// Tokenizer::addCurrentToken
//
// Adds the current token at [state] to the tokens list
// ----------------------------------------------------------------------------
void Tokenizer2::addCurrentToken(const TokenizeState& state)
{
	tokens_.push_back(
	{
		wxString::FromAscii(
			data_.data() + state.current_token.pos_start,
			state.position - state.current_token.pos_start
		),
		state.current_line,
		state.current_token.quoted_string,
		state.current_token.pos_start,
		state.position
	});

	// Convert to lower-case if not case sensitive (and not a quoted string)
	if (!case_sensitive_ && !state.current_token.quoted_string)
		tokens_.back().text.MakeLower();
}

// ----------------------------------------------------------------------------
// Tokenizer::tokenize
//
// Tokenizes the currently open text data, adding all tokens to the tokens list
// ----------------------------------------------------------------------------
void Tokenizer2::tokenize()
{
	tokens_.clear();

	// Init tokenizing state
	auto state = TokenizeState{};
	state.size = data_.size();

#ifdef DEBUG
	// Debug stuff
	unsigned prev_position = 0;
	unsigned prev_position_count = 0;
#endif

	while (state.position < state.size)
	{
		// Check for newline
		if (data_[state.position] == '\n')
			state.current_line++;

		// Process character depending on state
		switch (state.state)
		{
		case TokenizeState::State::Unknown:		tokenizeUnknown(state); break;
		case TokenizeState::State::Whitespace:	tokenizeWhitespace(state); break;
		case TokenizeState::State::Token:		tokenizeToken(state); break;
		case TokenizeState::State::Comment:		tokenizeComment(state); break;
		default: state.position++; break;
		}

#ifdef DEBUG
		// (Debug) Check we aren't stuck on a character
		if (state.position != prev_position)
		{
			prev_position = state.position;
			prev_position_count = 0;
		}
		else
		{
			prev_position_count++;
			if (prev_position_count > 5)
			{
				Log::warning(
					S_FMT(
						"Tokenizer stuck on character '%c', line %d, position %d. Skipping",
						data_[state.position],
						state.current_line,
						state.position
				));
				state.position++;
			}
		}
#endif
	}
	
	// Add token if the end of the data was reached during a token
	if (state.state == TokenizeState::State::Token)
		addCurrentToken(state);
}





// Testing

#include "General/Console/Console.h"
#include "MainEditor/MainEditor.h"
#include "Archive/ArchiveEntry.h"

CONSOLE_COMMAND(test_tokenizer, 0, false)
{
	auto entry = MainEditor::currentEntry();
	if (!entry)
		return;

	// Tokenize it
	Tokenizer2 tz;
	tz.setCaseSensitive(false);
	tz.openMem(entry->getMCData(), entry->getName());
	tz.tokenize();

	for (auto& token : tz.tokens())
	{
		Log::info(
			S_FMT("Line %d: [%d - %d] \"%s\" (%s)",
			token.line_no,
			token.pos_start,
			token.pos_end,
			CHR(token.text),
			token.quoted_string ? "quoted" : "normal"
		));
	}
}
