
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Tokenizer.h"
#include "FileUtils.h"
#include "StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
const string     Tokenizer::DEFAULT_SPECIAL_CHARACTERS = ";,:|={}/";
Tokenizer::Token Tokenizer::invalid_token_{ "", 0, false, 0, 0, 0, false };


// -----------------------------------------------------------------------------
//
// Local Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns true if [p] is a whitespace character
// -----------------------------------------------------------------------------
bool isWhitespace(char p)
{
	// Whitespace is either a newline, tab character or space
	return p == '\r' || p == '\n' || p == 13 || p == ' ' || p == '\t';
}
} // namespace


// -----------------------------------------------------------------------------
//
// Tokenizer::Token Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Parses a magic editor comment, as introduced by Doom Builder.
// Token should be of the form "//$Key ...".
// Returns a pair of the key (lowercased) and any value, or "true" if none.
// See also: https://zdoom.org/wiki/Editor_keys
// -----------------------------------------------------------------------------
std::pair<string, string> Tokenizer::parseEditorComment(string_view token)
{
	// These aren't documented very well, and it's not really clear what the argument format is intended
	// to be, but none of them seem to take more than one argument.  So take the whole rest of the line,
	// strip whitespace, and drop the quotes if any.

	// Find the first space
	size_t spos = token.find_first_of(" \t");
	if (spos == string_view::npos)
	{
		string key(token.substr(3));
		strutil::lowerIP(key);
		return std::pair{ key, "true" };
	}

	string key(token.substr(3, spos - 3));
	strutil::lowerIP(key);

	string value(token.substr(spos));
	strutil::trimIP(value);

	// Strip quotes, if present
	if (value.front() == '"' && value.back() == '"')
	{
		value.erase(value.begin());
		value.pop_back();
	}
	return std::pair{ key, value };
}

// -----------------------------------------------------------------------------
// Returns true if the token is a valid integer. If [allow_hex] is true, can
// also be a valid hex string
// -----------------------------------------------------------------------------
bool Tokenizer::Token::isInteger(bool allow_hex) const
{
	return strutil::isInteger(text, allow_hex);
}

// -----------------------------------------------------------------------------
// Returns true if the token is a valid hex string
// -----------------------------------------------------------------------------
bool Tokenizer::Token::isHex() const
{
	return strutil::isHex(text);
}

// -----------------------------------------------------------------------------
// Returns true if the token is a floating point number
// -----------------------------------------------------------------------------
bool Tokenizer::Token::isFloat() const
{
	return strutil::isFloat(text);
}

// ----------------------------------------------------------------------------
// Returns the token as an integer value
// ----------------------------------------------------------------------------
int Tokenizer::Token::asInt() const
{
	return strutil::asInt(text);
}

// -----------------------------------------------------------------------------
// Returns the token as a bool value.
// "false", "no" and "0" are false, anything else is true.
// -----------------------------------------------------------------------------
bool Tokenizer::Token::asBool() const
{
	return !(
		text.empty() || strutil::equalCI(text, "false") || strutil::equalCI(text, "no") || strutil::equalCI(text, "0"));
}

// ----------------------------------------------------------------------------
// Returns the token as a floating point value
// ----------------------------------------------------------------------------
double Tokenizer::Token::asFloat() const
{
	return strutil::asDouble(text);
}

// ----------------------------------------------------------------------------
// Sets [val] to the integer value of the token
// ----------------------------------------------------------------------------
void Tokenizer::Token::toInt(int& val) const
{
	val = strutil::asInt(text);
}

// -----------------------------------------------------------------------------
// Sets [val] to the token as a bool value.
// "false", "no" and "0" are false, anything else is true.
// -----------------------------------------------------------------------------
void Tokenizer::Token::toBool(bool& val) const
{
	val = !(
		text.empty() || strutil::equalCI(text, "false") || strutil::equalCI(text, "no") || strutil::equalCI(text, "0"));
}

// ----------------------------------------------------------------------------
// Sets [val] to the floating point (double) value of the token
// ----------------------------------------------------------------------------
void Tokenizer::Token::toFloat(double& val) const
{
	val = strutil::asDouble(text);
}

// ----------------------------------------------------------------------------
// Sets [val] to the floating point (float) value of the token
// ----------------------------------------------------------------------------
void Tokenizer::Token::toFloat(float& val) const
{
	val = strutil::asFloat(text);
}


// -----------------------------------------------------------------------------
//
// Tokenizer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Tokenizer class constructor
// -----------------------------------------------------------------------------
Tokenizer::Tokenizer(int comments, const string& special_characters) :
	comment_types_{ comments },
	special_characters_{ special_characters.begin(), special_characters.end() }
{
}

// -----------------------------------------------------------------------------
// Returns the 'next' token but doesn't advance
// -----------------------------------------------------------------------------
const Tokenizer::Token& Tokenizer::peek() const
{
	if (!token_next_.valid)
		return invalid_token_;

	return token_next_;
}

// -----------------------------------------------------------------------------
// Returns the 'next' token and advances to it
// -----------------------------------------------------------------------------
const Tokenizer::Token& Tokenizer::next()
{
	if (!token_next_.valid)
		return invalid_token_;

	token_current_ = token_next_;
	readNext();
	return token_current_;
}

// -----------------------------------------------------------------------------
// Advances [inc] tokens
// -----------------------------------------------------------------------------
void Tokenizer::adv(size_t inc)
{
	if (inc <= 0)
		return;

	for (size_t a = 0; a < inc - 1; a++)
		readNext();

	token_current_ = token_next_;
	readNext();
}

// -----------------------------------------------------------------------------
// Advances [inc] tokens if the current token matches [check]
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Advances [inc] tokens if the current token matches [check] (Case-Insensitive)
// -----------------------------------------------------------------------------
bool Tokenizer::advIfNC(const char* check, size_t inc)
{
	if (strutil::equalCI(token_current_.text, check))
	{
		adv(inc);
		return true;
	}

	return false;
}
bool Tokenizer::advIfNC(const string& check, size_t inc)
{
	if (strutil::equalCI(token_current_.text, check))
	{
		adv(inc);
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Advances [inc] tokens if the next token matches [check]
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Advances [inc] tokens if the next token matches [check] (Case-Insensitive)
// -----------------------------------------------------------------------------
bool Tokenizer::advIfNextNC(const char* check, size_t inc)
{
	if (!token_next_.valid)
		return false;

	if (strutil::equalCI(token_next_.text, check))
	{
		adv(inc);
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Advances to the first token on the next line
// -----------------------------------------------------------------------------
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
	bool     end  = false;
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
	while (token_next_.pos_start > token_current_.pos_start && token_next_.line_no <= token_current_.line_no)
		adv();
}

// -----------------------------------------------------------------------------
// Advances tokens until [end] is found
// -----------------------------------------------------------------------------
void Tokenizer::advUntil(const char* end)
{
	while (!atEnd())
	{
		adv();

		if (token_current_ == end)
			break;
	}
}

void slade::Tokenizer::advUntilNC(const char* end)
{
	while (!atEnd())
	{
		adv();

		if (strutil::equalCI(token_current_.text, end))
			break;
	}
}

// -----------------------------------------------------------------------------
// Advances tokens until [end] is found. Can also handle nesting if [begin] is
// specified. If [allow_quoted] is true, section delimiters will count even if
// within a quoted string.
//
// Relies on the current token being *within* the section to be skipped, and
// will exit on the token after the end of the section.
// -----------------------------------------------------------------------------
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

		if (strutil::equalCI(token_current_.text, end))
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
		state_.position     = token_next_.pos_start;
		state_.current_line = token_next_.line_no;
	}

	string line;
	while (state_.position < state_.size && data_[state_.position] != '\n' && data_[state_.position] != '\r')
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

bool Tokenizer::checkNC(const char* check) const
{
	return strutil::equalCI(token_current_.text, check);
}

bool Tokenizer::checkOrEndNC(const char* check) const
{
	// At end, return true
	if (!token_next_.valid)
		return true;

	return strutil::equalCI(token_current_.text, check);
}

// -----------------------------------------------------------------------------
// Returns true if the next token matches [check]
// -----------------------------------------------------------------------------
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

	return strutil::equalCI(token_next_.text, check);
}

// -----------------------------------------------------------------------------
// Opens text from a file [filename], reading [length] bytes from [offset].
// If [length] is 0, read to the end of the file
// -----------------------------------------------------------------------------
bool Tokenizer::openFile(string_view filename, size_t offset, size_t length)
{
	// Open the file
	SFile file(filename);

	// Check file opened
	if (!file.isOpen())
	{
		log::error("Tokenizer::openFile: Unable to open file {}", filename);
		return false;
	}

	// Use filename as source
	source_ = filename;

	// If length isn't specified or exceeds the file length,
	// only read to the end of the file
	if (offset + length > file.size() || length == 0)
		length = static_cast<size_t>(file.size()) - offset;

	// Read the file portion
	data_.resize((size_t)length, 0);
	file.seekFromStart(offset);
	file.read(data_.data(), (size_t)length);

	reset();

	return true;
}

// -----------------------------------------------------------------------------
// Opens text from a string [text], reading [length] bytes from [offset].
// If [length] is 0, read to the end of the string
// -----------------------------------------------------------------------------
bool Tokenizer::openString(string_view text, size_t offset, size_t length, string_view source)
{
	source_ = source;

	// If length isn't specified or exceeds the string's length,
	// only copy to the end of the string
	if (offset + length > text.length() || length == 0)
		length = text.length() - offset;

	// Copy the string portion
	data_.assign(text.data() + offset, text.data() + offset + length);

	reset();

	return true;
}

// -----------------------------------------------------------------------------
// Opens text from memory [mem], reading [length] bytes
// -----------------------------------------------------------------------------
bool Tokenizer::openMem(const char* mem, size_t length, string_view source)
{
	source_ = source;
	data_.assign(mem, mem + length);

	reset();

	return true;
}

// -----------------------------------------------------------------------------
// Opens text from a MemChunk [mc]
// -----------------------------------------------------------------------------
bool Tokenizer::openMem(const MemChunk& mc, string_view source)
{
	source_ = source;
	data_.assign(mc.data(), mc.data() + mc.size());

	reset();

	return true;
}

// -----------------------------------------------------------------------------
// Resets the tokenizer to the beginning of the data
// -----------------------------------------------------------------------------
void Tokenizer::reset()
{
	// Init tokenizing state
	state_      = TokenizeState{};
	state_.size = data_.size();

	// Read first tokens
	readNext(&token_current_);
	readNext(&token_next_);
}

// -----------------------------------------------------------------------------
// Checks if a comment begins at the current position and returns the comment
// type if one does (0 otherwise)
// -----------------------------------------------------------------------------
unsigned Tokenizer::checkCommentBegin() const
{
	// C-Style comment (/*)
	if (comment_types_ & CStyle && state_.position + 1 < state_.size && data_[state_.position] == '/'
		&& data_[state_.position + 1] == '*')
		return CStyle;

	// CPP-Style comment (//)
	if (comment_types_ & CPPStyle && state_.position + 1 < state_.size && data_[state_.position] == '/'
		&& data_[state_.position + 1] == '/')
		return CPPStyle;

	// ## comment
	if (comment_types_ & DoubleHash && state_.position + 1 < state_.size && data_[state_.position] == '#'
		&& data_[state_.position + 1] == '#')
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

// -----------------------------------------------------------------------------
// Process the current unknown character
// -----------------------------------------------------------------------------
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
		state_.current_token.line_no       = state_.current_line;
		state_.current_token.quoted_string = false;
		state_.current_token.pos_start     = state_.position;
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
		state_.current_token.line_no       = state_.current_line;
		state_.current_token.quoted_string = true;
		state_.current_token.pos_start     = state_.position;
		state_.state                       = TokenizeState::State::Token;

		return;
	}

	// Token
	state_.current_token.line_no       = state_.current_line;
	state_.current_token.quoted_string = false;
	state_.current_token.pos_start     = state_.position;
	state_.state                       = TokenizeState::State::Token;
}

// -----------------------------------------------------------------------------
// Process the current token character
// -----------------------------------------------------------------------------
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
			state_.done  = true;
			return;
		}

		// Escape backslash+double-quote
		if (state_.position < state_.size && data_[state_.position] == '\\' && data_[state_.position + 1] == '\"')
			++state_.position;

		// Continue token
		++state_.position;

		return;
	}

	// Check for end of token
	if (isEndOfToken())
	{
		// End token
		state_.state  = TokenizeState::State::Unknown;
		state_.done   = true;
		state_.to_eol = false;

		return;
	}

	// Continue token
	++state_.position;
}

// -----------------------------------------------------------------------------
// Process the current comment character
// -----------------------------------------------------------------------------
void Tokenizer::tokenizeComment()
{
	// Check for decorate //$
	if (editor_comments_ && state_.comment_type == CPPStyle)
	{
		if (data_[state_.position] == '$' && data_[state_.position - 1] == '/' && data_[state_.position - 2] == '/')
		{
			// We have a token instead
			state_.current_token.line_no       = state_.current_line;
			state_.current_token.quoted_string = false;
			state_.current_token.pos_start     = state_.position - 2;
			state_.state                       = TokenizeState::State::Token;
			state_.to_eol                      = true;
			return;
		}
	}

	// Check for end of line comment
	if (state_.comment_type != CStyle && (data_[state_.position] == '\n' || data_[state_.position] == '\r'))
	{
		state_.state = TokenizeState::State::Unknown;
		++state_.position;
		return;
	}

	// Check for end of C-Style multi line comment
	if (state_.comment_type == CStyle)
	{
		if (state_.position + 1 < state_.size && data_[state_.position] == '*' && data_[state_.position + 1] == '/')
		{
			state_.state = TokenizeState::State::Unknown;
			state_.position += 2;
			return;
		}
	}

	// Continue comment
	++state_.position;
}

// -----------------------------------------------------------------------------
// Process the current whitespace character
// -----------------------------------------------------------------------------
void Tokenizer::tokenizeWhitespace()
{
	if (isWhitespace(data_[state_.position]))
		++state_.position;
	else
		state_.state = TokenizeState::State::Unknown;
}

// -----------------------------------------------------------------------------
// Reads the next token from the data and writes it to [target] if specified
// -----------------------------------------------------------------------------
bool Tokenizer::readNext(Token* target)
{
	if (data_.empty() || state_.position >= state_.size)
	{
		if (target)
			target->valid = false;
		return false;
	}

	// Process until the end of a token or the end of the data
	state_.done = false;
	while (state_.position < state_.size && !state_.done)
	{
		// Check for newline
		if (data_[state_.position] == '\n' && state_.state != TokenizeState::State::Token)
			++state_.current_line;

		// Process current character depending on state
		switch (state_.state)
		{
		case TokenizeState::State::Unknown:    tokenizeUnknown(); break;
		case TokenizeState::State::Whitespace: tokenizeWhitespace(); break;
		case TokenizeState::State::Token:      tokenizeToken(); break;
		case TokenizeState::State::Comment:    tokenizeComment(); break;
		}
	}

	// Write to target token (if specified)
	if (target)
	{
		target->text.clear();
		for (unsigned a = state_.current_token.pos_start; a < state_.position; ++a)
		{
			if (state_.current_token.quoted_string && a < data_.size() - 1 && data_[a] == '\\' && data_[a + 1] == '\"')
				++a;

			target->text += data_[a];
		}

		target->line_no       = state_.current_token.line_no;
		target->quoted_string = state_.current_token.quoted_string;
		target->pos_start     = state_.current_token.pos_start;
		target->pos_end       = state_.position;
		target->length        = target->pos_end - target->pos_start;
		target->valid         = true;

		// Convert to lowercase if configured to and it isn't a quoted string
		if (read_lowercase_ && !target->quoted_string)
			strutil::lowerIP(target->text);
	}

	// Skip closing " if it was a quoted string
	if (state_.current_token.quoted_string)
		++state_.position;

	if (debug_)
		log::debug("{}: \"{}\"", token_current_.line_no, token_current_.text);

	return true;
}

void Tokenizer::resetToLineStart()
{
	// Reset state to start of current token
	state_.position     = token_current_.pos_start;
	state_.current_line = token_current_.line_no;
	state_.state        = TokenizeState::State::Unknown;

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

bool Tokenizer::isEndOfToken() const
{
	// Token is to end of line, only ends on newline
	if (state_.to_eol)
		return data_[state_.position] == '\n' || data_[state_.position] == '\r';

	// Regular token
	return isWhitespace(data_[state_.position]) ||       // Whitespace
		   isSpecialCharacter(data_[state_.position]) || // Special character
		   checkCommentBegin() > 0;                      // Comment
}


// Testing

#include "App.h"
#include "Archive/ArchiveEntry.h"
#include "General/Console.h"
#include "MainEditor/MainEditor.h"

CONSOLE_COMMAND(test_tokenizer, 0, false)
{
	auto entry = maineditor::currentEntry();
	if (!entry)
		return;

	int num = 1;
	if (!args.empty())
		num = strutil::asInt(args[0]);

	bool lower = (VECTOR_EXISTS(args, "lower"));
	bool dump  = (VECTOR_EXISTS(args, "dump"));

	struct TestToken
	{
		string   text;
		bool     quoted_string;
		unsigned line_no;
	};

	// Tokenize it
	Tokenizer         tz;
	vector<TestToken> t_new;
	tz.setReadLowerCase(lower);
	long time = app::runTimer();
	tz.openMem(entry->data(), entry->name());
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

	long new_time = app::runTimer() - time;

	log::info("Tokenize x{} took {}ms", num, new_time);


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
	log::info(string::Format("Old Tokenize x%d took %dms", num, time));
	log::info(string::Format("%1.3fx time", (float)new_time / (float)time));
	*/

	if (dump)
	{
		for (auto& token : t_new)
			log::debug("{}: \"{}\"{}", token.line_no, token.text, token.quoted_string ? " (quoted)" : "");
	}
}
