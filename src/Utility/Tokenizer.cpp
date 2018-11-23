
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
#include "StringUtils.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
const string Tokenizer::DEFAULT_SPECIAL_CHARACTERS = ";,:|={}/";
Tokenizer::Token Tokenizer::invalid_token_{ "", 0, false, 0, 0, 0, false };


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
		return p == '\n' || p == 13 || p == ' ' || p == '\t';
	}
}


// ----------------------------------------------------------------------------
//
// Tokenizer::Token Struct Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Token::isInteger
//
// Returns true if the token is a valid integer. If [allow_hex] is true, can
// also be a valid hex string
// ----------------------------------------------------------------------------
bool Tokenizer::Token::isInteger(bool allow_hex) const
{
	return StringUtils::isInteger(text, allow_hex);
}

// ----------------------------------------------------------------------------
// Token::isHex
//
// Returns true if the token is a valid hex string
// ----------------------------------------------------------------------------
bool Tokenizer::Token::isHex() const
{
	return StringUtils::isHex(text);
}

// ----------------------------------------------------------------------------
// Token::isFloat
//
// Returns true if the token is a floating point number
// ----------------------------------------------------------------------------
bool Tokenizer::Token::isFloat() const
{
	return StringUtils::isFloat(text);
}

// ----------------------------------------------------------------------------
// Token::asBool
//
// Returns the token as a bool value.
// "false", "no" and "0" are false, anything else is true.
// ----------------------------------------------------------------------------
bool Tokenizer::Token::asBool() const
{
	return !(S_CMPNOCASE(text, "false") ||
			 S_CMPNOCASE(text, "no") ||
			 S_CMPNOCASE(text, "0"));
}

// ----------------------------------------------------------------------------
// Token::toBool
//
// Sets [val] to the token as a bool value.
// "false", "no" and "0" are false, anything else is true.
// ----------------------------------------------------------------------------
void Tokenizer::Token::toBool(bool& val) const
{
	val = !(S_CMPNOCASE(text, "false") ||
			S_CMPNOCASE(text, "no") ||
			S_CMPNOCASE(text, "0"));
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
Tokenizer::Tokenizer(int comments, const string& special_characters) :
	comment_types_{ comments },
	special_characters_{special_characters.begin(), special_characters.end()},
	decorate_{ false },
	read_lowercase_{ false },
	debug_{ false }
{
}

// ----------------------------------------------------------------------------
// Tokenizer::peek
//
// Returns the 'next' token but doesn't advance
// ----------------------------------------------------------------------------
const Tokenizer::Token& Tokenizer::peek() const
{
	if (!token_next_.valid)
		return invalid_token_;

	return token_next_;
}

// ----------------------------------------------------------------------------
// Tokenizer::next
//
// Returns the 'next' token and advances to it
// ----------------------------------------------------------------------------
const Tokenizer::Token& Tokenizer::next()
{
	if (!token_next_.valid)
		return invalid_token_;

	token_current_ = token_next_;
	readNext();
	return token_current_;
}

// ----------------------------------------------------------------------------
// Tokenizer::adv
//
// Advances [inc] tokens
// ----------------------------------------------------------------------------
void Tokenizer::adv(size_t inc)
{
	if (inc <= 0)
		return;

	for (size_t a = 0; a < inc - 1; a++)
		readNext();

	token_current_ = token_next_;
	readNext();
}

// ----------------------------------------------------------------------------
// Tokenizer::advIf
//
// Advances [inc] tokens if the current token matches [check]
// ----------------------------------------------------------------------------
bool Tokenizer::advIf(const char* check, size_t inc)
{
	if (token_current_ == check)
	{
		adv(inc);
		return true;
	}

	return false;
}
bool Tokenizer::advIf(const string& check, size_t inc)
{
	if (token_current_ == check)
	{
		adv(inc);
		return true;
	}

	return false;
}
bool Tokenizer::advIf(char check, size_t inc)
{
	if (token_current_ == check)
	{
		adv(inc);
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Tokenizer::advIfNC
//
// Advances [inc] tokens if the current token matches [check] (Case-Insensitive)
// ----------------------------------------------------------------------------
bool Tokenizer::advIfNC(const char* check, size_t inc)
{
	if (S_CMPNOCASE(token_current_.text, check))
	{
		adv(inc);
		return true;
	}

	return false;
}
bool Tokenizer::advIfNC(const string& check, size_t inc)
{
	if (S_CMPNOCASE(token_current_.text, check))
	{
		adv(inc);
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Tokenizer::advIfNext
//
// Advances [inc] tokens if the next token matches [check]
// ----------------------------------------------------------------------------
bool Tokenizer::advIfNext(const char* check, size_t inc)
{
	if (!token_next_.valid)
		return false;

	if (token_next_ == check)
	{
		adv(inc);
		return true;
	}

	return false;
}
bool Tokenizer::advIfNext(const string& check, size_t inc)
{
	if (!token_next_.valid)
		return false;

	if (token_next_ == check)
	{
		adv(inc);
		return true;
	}

	return false;
}
bool Tokenizer::advIfNext(char check, size_t inc)
{
	if (!token_next_.valid)
		return false;

	if (token_next_ == check)
	{
		adv(inc);
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Tokenizer::advIfNextNC
//
// Advances [inc] tokens if the next token matches [check] (Case-Insensitive)
// ----------------------------------------------------------------------------
bool Tokenizer::advIfNextNC(const char* check, size_t inc)
{
	if (!token_next_.valid)
		return false;

	if (S_CMPNOCASE(token_next_.text, check))
	{
		adv(inc);
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// Tokenizer::advToNextLine
//
// Advances to the first token on the next line
// ----------------------------------------------------------------------------
void Tokenizer::advToNextLine()
{
	// Ignore if we're on the last token already
	if (!token_next_.valid)
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
	bool end = false;
	while (!end)
	{
		end = !readNext(&token_current_);

		if (token_current_.line_no > line)
		{
			readNext();
			return;
		}
	}

	// If we got to the end, update the 'next' token
	token_next_ = token_current_;
}

void Tokenizer::advToEndOfLine()
{
	// Ignore if we're on the last token already
	if (!token_next_.valid)
		return;

	// Ignore if the next token is on the next line
	if (token_next_.line_no > token_current_.line_no)
		return;

	// Otherwise skip until the next token is on the next line
	// (or we reach the last token)
	while (token_next_.pos_start > token_current_.pos_start &&
			token_next_.line_no <= token_current_.line_no)
		adv();
}

// ----------------------------------------------------------------------------
// Tokenizer::skipSection
//
// Advances tokens until [end] is found. Can also handle nesting if [begin] is
// specified. If [allow_quoted] is true, section delimiters will count even if
// within a quoted string.
//
// Relies on the current token being *within* the section to be skipped, and
// will exit on the token after the end of the section.
// ----------------------------------------------------------------------------
void Tokenizer::skipSection(const char* begin, const char* end, bool allow_quoted)
{
	int depth = 1;
	while (depth > 0 && !atEnd())
	{
		if (token_current_.quoted_string && !allow_quoted)
		{
			adv();
			continue;
		}

		if (token_current_ == begin)
			depth++;
		if (token_current_ == end)
			depth--;

		adv();
	}
}

vector<Tokenizer::Token> Tokenizer::getTokensUntil(const char* end)
{
	vector<Token> tokens;
	while (!atEnd())
	{
		tokens.push_back(token_current_);

		adv();

		if (token_current_ == end)
			break;
	}

	return tokens;
}

vector<Tokenizer::Token> Tokenizer::getTokensUntilNC(const char* end)
{
	vector<Token> tokens;
	while (!atEnd())
	{
		tokens.push_back(token_current_);

		adv();

		if (token_current_ == end)
			break;
	}

	return tokens;
}

vector<Tokenizer::Token> Tokenizer::getTokensUntilNextLine(bool from_start)
{
	// Reset to start of line if needed
	if (from_start)
	{
		resetToLineStart();
		readNext(&token_current_);
		readNext(&token_next_);
	}

	vector<Token> tokens;
	while (!atEnd())
	{
		tokens.push_back(token_current_);

		if (token_next_.line_no > token_current_.line_no)
		{
			adv();
			break;
		}

		adv();
	}

	return tokens;
}

string Tokenizer::getLine(bool from_start)
{
	// Reset to start of line if needed
	if (from_start)
		resetToLineStart();

	// Otherwise reset to start of next token
	else
	{
		state_.position = token_next_.pos_start;
		state_.current_line = token_next_.line_no;
	}

	string line;
	while (data_[state_.position] != '\n' && data_[state_.position] != '\r')
		line += data_[state_.position++];

	readNext(&token_current_);
	readNext(&token_next_);

	return line;
}

bool Tokenizer::checkOrEnd(const char* check) const
{
	// At end, return true
	if (!token_next_.valid)
		return true;

	return token_current_ == check;
}
bool Tokenizer::checkOrEnd(const string& check) const
{
	// At end, return true
	if (!token_next_.valid)
		return true;

	return token_current_ == check;
}
bool Tokenizer::checkOrEnd(char check) const
{
	// At end, return true
	if (!token_next_.valid)
		return true;

	return token_current_ == check;
}

bool Tokenizer::checkOrEndNC(const char* check) const
{
	// At end, return true
	if (!token_next_.valid)
		return true;

	return S_CMPNOCASE(token_current_.text, check);
}

// ----------------------------------------------------------------------------
// Tokenizer::checkNext
//
// Returns true if the next token matches [check]
// ----------------------------------------------------------------------------
bool Tokenizer::checkNext(const char* check) const
{
	if (!token_next_.valid)
		return false;

	return token_next_ == check;
}
bool Tokenizer::checkNext(const string& check) const
{
	if (!token_next_.valid)
		return false;

	return token_next_ == check;
}
bool Tokenizer::checkNext(char check) const
{
	if (!token_next_.valid)
		return false;

	return token_next_ == check;
}

bool Tokenizer::checkNextNC(const char* check) const
{
	if (!token_next_.valid)
		return false;

	return S_CMPNOCASE(token_next_.text, check);
}

// ----------------------------------------------------------------------------
// Tokenizer::openFile
//
// Opens text from a file [filename], reading [length] bytes from [offset].
// If [length] is 0, read to the end of the file
// ----------------------------------------------------------------------------
bool Tokenizer::openFile(const string& filename, size_t offset, size_t length)
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
		length = (size_t) file.Length() - offset;

	// Read the file portion
	data_.resize((size_t) length, 0);
	file.Seek(offset, wxFromStart);
	file.Read(data_.data(), (size_t) length);

	reset();

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::openString
//
// Opens text from a string [text], reading [length] bytes from [offset].
// If [length] is 0, read to the end of the string
// ----------------------------------------------------------------------------
bool Tokenizer::openString(const string& text, size_t offset, size_t length, const string& source)
{
	source_ = source;

	// If length isn't specified or exceeds the string's length,
	// only copy to the end of the string
	auto ascii = text.ToAscii();
	if (offset + length > ascii.length() || length == 0)
		length = ascii.length() - offset;

	// Copy the string portion
	data_.assign(ascii.data() + offset, ascii.data() + offset + length);

	reset();

	return true;
}

// ----------------------------------------------------------------------------
// Tokenizer::openMem
//
// Opens text from memory [mem], reading [length] bytes
// ----------------------------------------------------------------------------
bool Tokenizer::openMem(const char* mem, size_t length, const string& source)
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
	if (data_.empty() || state_.position >= state_.size)
	{
		if (target) target->valid = false;
		return false;
	}

	// Process until the end of a token or the end of the data
	state_.done = false;
	while (state_.position < state_.size && !state_.done)
	{
		// Check for newline
		if (data_[state_.position] == '\n' &&
			state_.state != TokenizeState::State::Token)
			++state_.current_line;

		// Process current character depending on state
		switch (state_.state)
		{
		case TokenizeState::State::Unknown:		tokenizeUnknown(); break;
		case TokenizeState::State::Whitespace:	tokenizeWhitespace(); break;
		case TokenizeState::State::Token:		tokenizeToken(); break;
		case TokenizeState::State::Comment:		tokenizeComment(); break;
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
		{
			if (state_.current_token.quoted_string && data_[a] == '\\')
				++a;

			target->text += data_[a];
		}

		target->line_no = state_.current_token.line_no;
		target->quoted_string = state_.current_token.quoted_string;
		target->pos_start = state_.current_token.pos_start;
		target->pos_end = state_.position;
		target->length = target->pos_end - target->pos_start;
		target->valid = true;

		// Convert to lowercase if configured to and it isn't a quoted string
		if (read_lowercase_ && !target->quoted_string)
			target->text.LowerCase();
	}

	// Skip closing " if it was a quoted string
	if (state_.current_token.quoted_string)
		++state_.position;

	if (debug_)
		Log::debug(S_FMT("%d: \"%s\"", token_current_.line_no, CHR(token_current_.text)));
		
	return true;
}

void Tokenizer::resetToLineStart()
{
	// Reset state to start of current token
	state_.position = token_current_.pos_start;
	state_.current_line = token_current_.line_no;
	state_.state = TokenizeState::State::Unknown;

	while (true)
	{
		if (state_.position == 0)
			return;
		if (data_[state_.position] == '\n')
		{
			++state_.position;
			return;
		}

		--state_.position;
	}
}


// Testing

#include "General/Console/Console.h"
#include "MainEditor/MainEditor.h"
#include "Archive/ArchiveEntry.h"
#include "App.h"

CONSOLE_COMMAND(test_tokenizer, 0, false)
{
	auto entry = MainEditor::currentEntry();
	if (!entry)
		return;

	long num = 1;
	if (!args.empty())
		args[0].ToLong(&num);

	bool lower = (VECTOR_EXISTS(args, "lower"));
	bool dump = (VECTOR_EXISTS(args, "dump"));

	struct TestToken
	{
		string text;
		bool quoted_string;
		unsigned line_no;
	};

	// Tokenize it
	Tokenizer tz;
	vector<TestToken> t_new;
	tz.setReadLowerCase(lower);
	long time = App::runTimer();
	tz.openMem(entry->getMCData(), entry->getName());
	for (long a = 0; a < num; a++)
	{
		while (!tz.atEnd())
		{
			if (a == 0)
				t_new.push_back({ tz.current().text, tz.current().quoted_string, tz.current().line_no });

			tz.next();
		}
		tz.reset();
	}

	long new_time = App::runTimer() - time;

	Log::info(S_FMT("Tokenize x%d took %dms", num, new_time));


	// Test old tokenizer also
	/*
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
	Log::info(S_FMT("%1.3fx time", (float)new_time / (float)time));
	*/

	if (dump)
	{
		for (auto& token : t_new)
			Log::debug(S_FMT("%d: \"%s\"%s", token.line_no, CHR(token.text), token.quoted_string ? " (quoted)" : ""));
	}
}
