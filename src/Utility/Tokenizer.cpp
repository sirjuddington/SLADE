
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Tokenizer.cpp
 * Description: My trusty old string tokenizer class.
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
#include "Tokenizer.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
const string Tokenizer::DEFAULT_SPECIAL_CHARS = ";,:|={}/";


/*******************************************************************
 * TOKENIZER CLASS FUNCTIONS
 *******************************************************************/

/* Tokenizer::Tokenizer
 * Tokenizer class constructor
 *******************************************************************/
Tokenizer::Tokenizer(CommentTypes comments_style)
{
	// Initialize variables
	current = NULL;
	start = NULL;
	size = 0;
	comments = comments_style;
	debug = false;
	setSpecialCharacters(DEFAULT_SPECIAL_CHARS);
	name = "nothing";
	line = 1;
	t_start = 0;
	t_end = 0;
	decorate = false;
}

/* Tokenizer::~Tokenizer
 * Tokenizer class destructor
 *******************************************************************/
Tokenizer::~Tokenizer()
{
	// Free memory if used
	if (start) free(start);
}

/* Tokenizer::setSpecialCharacters
* sets special to a value
*******************************************************************/
void Tokenizer::setSpecialCharacters(string special_new)
{
	this->special.clear();
	this->special_length = special_new.size();
	for (unsigned a = 0; a < special_length; a++)
	{
		this->special.push_back(special_new[a]);
	}
}

/* Tokenizer::openFile
 * Reads a portion of a file to the Tokenizer
 *******************************************************************/
bool Tokenizer::openFile(string filename, uint32_t offset, uint32_t length)
{
	// Open the file
	wxFile file(filename);

	name = filename;

	// Check file opened
	if (!file.IsOpened())
	{
		LOG_MESSAGE(1, "Tokenizer::openFile: Unable to open file %s", filename);
		return false;
	}

	// If length isn't specified or exceeds the file length,
	// only read to the end of the file
	if (offset + length > file.Length() || length == 0)
		length = file.Length() - offset;

	// Setup variables & allocate memory
	size = length;
	position = 0;
	start = current = (char*) malloc(size);
	end = start + size + 1;
	line = 1;
	t_start = 0;
	t_end = 0;

	// Read the file portion
	file.Seek(offset, wxFromStart);
	file.Read(start, size);

	return true;
}

/* Tokenizer::openString
 * Reads a portion of a string to the Tokenizer
 *******************************************************************/
bool Tokenizer::openString(string text, uint32_t offset, uint32_t length, string source)
{
	// If length isn't specified or exceeds the string's length,
	// only copy to the end of the string
	if (offset + length > (uint32_t) text.length() || length == 0)
		length = (uint32_t) text.length() - offset;

	// Setup variables & allocate memory
	size = length;
	position = 0;
	line = 1;
	t_start = 0;
	t_end = 0;
	start = current = (char*) malloc(size);
	end = start + size + 1;
	name = source;

	// Copy the string portion
	memcpy(start, ((char*) text.char_str()) + offset, size);

	return true;
}

/* Tokenizer::openMem
 * Reads a chunk of memory to the Tokenizer
 *******************************************************************/
bool Tokenizer::openMem(const char* mem, uint32_t length, string source)
{
	// Length must be specified
	if (length == 0)
	{
		LOG_MESSAGE(1, "Tokenizer::openMem: length not specified");
		return false;
	}

	name = source;

	// Setup variables & allocate memory
	size = length;
	position = 0;
	line = 1;
	t_start = 0;
	t_end = 0;
	start = current = (char*) malloc(size);
	end = start + size + 1;

	// Copy the data
	memcpy(start, mem, size);

	return true;
}

/* Tokenizer::openMem
 * Reads a chunk of memory to the Tokenizer
 *******************************************************************/
bool Tokenizer::openMem(const uint8_t* mem, uint32_t length, string source)
{
	// Length must be specified
	if (length == 0)
	{
		LOG_MESSAGE(1, "Tokenizer::openMem: length not specified");
		return false;
	}

	name = source;

	// Setup variables & allocate memory
	size = length;
	position = 0;
	line = 1;
	t_start = 0;
	t_end = 0;
	start = current = (char*) malloc(size);
	end = start + size + 1;

	// Copy the data
	memcpy(start, mem, size);

	return true;
}

/* Tokenizer::openMem
 * Reads a chunk of memory to the Tokenizer
 *******************************************************************/
bool Tokenizer::openMem(MemChunk* mem, string source)
{
	// Needs to be valid
	if (mem == NULL)
	{
		LOG_MESSAGE(1, "Tokenizer::openMem: invalid MemChunk");
		return false;
	}

	name = source;

	// Setup variables & allocate memory
	size = mem->getSize();
	position = 0;
	line = 1;
	t_start = 0;
	t_end = 0;
	start = current = (char*) malloc(size);
	end = start + size + 1;

	// Copy the data
	memcpy(start, mem->getData(), size);

	return true;
}

/* Tokenizer::isWhitespace
 * Checks if a character is 'whitespace'
 *******************************************************************/
bool Tokenizer::isWhitespace(char p)
{
	// Whitespace is either a newline, tab character or space
	if (p == '\n' || p == 13 || p == ' ' || p == '\t')
		return true;
	else
		return false;
}

/* Tokenizer::isSpecialCharacter
 * Checks if a character is a 'special' character, ie a character
 * that should always be its own token (;, =, | etc)
 *******************************************************************/
bool Tokenizer::isSpecialCharacter(char p)
{
	// Check through special tokens string
	for (unsigned a = 0; a < special_length; a++)
	{
		if (special[a] == p)
			return true;
	}

	return false;
}

/* Tokenizer::incrementCurrent
 * Increments the position pointer, returns false on end of block
 *******************************************************************/
bool Tokenizer::incrementCurrent()
{
	if (position >= size - 1)
	{
		// At end of text, return false
		position = size;
		return false;
	}
	else
	{
		// Check for newline
		if (current[0] == '\n')
			line++;

		// Increment position & current pointer
		position++;
		current++;
		t_end++;
		return true;
	}
}

/* Tokenizer::skipLineComment
 * Skips a '//' comment
 *******************************************************************/
void Tokenizer::skipLineComment()
{
	if (atEnd())
		return;

	// Increment position until a newline or end is found
	while (current[0] != '\n' && current[0] != 13)
	{
		if (!incrementCurrent())
			return;
	}

	// Skip the newline character also
	incrementCurrent();
}

/* Tokenizer::skipMultilineComment
 * Skips a multiline comment (like this one :P)
 *******************************************************************/
void Tokenizer::skipMultilineComment()
{
	if (atEnd())
		return;

	// Increment position until '*/' or end is found
	while (!(current[0] == '*' && current[1] == '/'))
	{
		if (!incrementCurrent())
			return;
	}

	// Skip the ending '*/'
	incrementCurrent();
	incrementCurrent();
}

/* Tokenizer::readToken
 * Reads the next 'token' from the text & moves past it
 *******************************************************************/
void Tokenizer::readToken(bool toeol)
{
	token_current.clear();
	bool ready = false;
	qstring = false;

	// Increment pointer to next token
	while (!ready)
	{
		ready = true;

		// Increment pointer until non-whitespace is found
		while (isWhitespace(current[0]))
		{
			// Return if end of text found
			if (!incrementCurrent())
				return;
		}

		// Skip C-style comments
		if (comments & CCOMMENTS)
		{
			// Check if we have a line comment
			if (current + 1 < end && current[0] == '/' && current[1] == '/')
			{
				ready = false;

				// DECORATE //$ handling
				if (!decorate)
					skipLineComment();
				else if (current + 2 < end && current[2] != '$')
					skipLineComment();
				else
					ready = true;
			}

			// Check if we have a multiline comment
			if (current + 1 != end && current[0] == '/' && current[1] == '*')
			{
				skipMultilineComment(); // Skip it
				ready = false;
			}
		}

		// Skip '##' comments
		if (comments & DCOMMENTS)
		{
			if (current + 1 != end && current[0] == '#' && current[1] == '#')
			{
				skipLineComment(); // Skip it
				ready = false;
			}
		}

		// Skip '#' comments
		if (comments & HCOMMENTS)
		{
			if (current + 1 != end && current[0] == '#')
			{
				skipLineComment(); // Skip it
				ready = false;
			}
		}

		// Skip ';' comments
		if (comments & SCOMMENTS)
		{
			if (current[0] == ';')
			{
				skipLineComment(); // Skip it
				ready = false;
			}
		}

		// Check for end of text
		if (position == size)
			return;
	}

	// Init token delimiters
	t_start = position;
	t_end = position;

	// If we're at a special character, it's our token
	if (isSpecialCharacter(current[0]))
	{
		token_current += current[0];
		t_end = position + 1;
		incrementCurrent();
		return;
	}

	// Now read the token
	if (current[0] == '\"')   // If we have a literal string (enclosed with "")
	{
		qstring = true;

		// Skip opening "
		incrementCurrent();

		// Read literal string (include whitespace)
		while (current[0] != '\"')
		{
			//if (position < size - 1 && current[0] == '\\' && current[1] == '\"')
			if (current[0] == '\\')
				incrementCurrent();

			token_current += current[0];

			if (!incrementCurrent())
				return;
		}

		// Skip closing "
		incrementCurrent();
	}
	else
	{
		// Read token (don't include whitespace)
		while (!((!toeol && isWhitespace(current[0])) || current[0] == '\n'))
		{
			// Return if special character found
			if (!toeol && isSpecialCharacter(current[0]))
				return;

			// Return if a comment starts without whitespace
			if (comments & CCOMMENTS && token_current.size() && current + 1 < end &&
				current[0] == '/' && (current[1] == '/' || current[1] == '*'))
				return;

			// Add current character to the token
			token_current += current[0];

			// Return if end of text found
			if (!incrementCurrent())
				return;
		}
	}

	// Write token to log if debug mode enabled
	if (debug)
		LOG_MESSAGE(1, "%s", token_current);

	// Return the token
	return;
}

/* Tokenizer::skipToken
 * Reads the next 'token' from the text & moves past it
 *******************************************************************/
void Tokenizer::skipToken()
{
	readToken();
}

/* Tokenizer::getToken
 * Gets the next 'token' from the text & moves past it, returns
 * a blank string if we're at the end of the text
 *******************************************************************/
string Tokenizer::getToken()
{
	readToken();
	return token_current;
}

/* Tokenizer::getLine
 * Gets the rest of the line (including whitespace) as a single token
 * and moves past it, returns a blank string if we're at the end of 
 * the text
 *******************************************************************/
string Tokenizer::getLine()
{
	readToken(true);
	return token_current;
}

/* Tokenizer::getToken
 * Gets the next 'token' from the text & moves past it, writes
 * the result to [s]
 *******************************************************************/
void Tokenizer::getToken(string* s)
{
	// Read token
	readToken();

	// Set string value
	*s = token_current;
}

/* Tokenizer::peekToken
 * Returns the next token without actually moving past it
 *******************************************************************/
string Tokenizer::peekToken()
{
	// Backup current position
	char* c = current;
	uint32_t p = position;
	int oline = line;

	// Read the next token
	readToken();

	// Go back to original position
	current = c;
	position = p;
	line = oline;

	// Return the token
	return token_current;
}

/* Tokenizer::checkToken
 * Compares the current token with a string
 *******************************************************************/
bool Tokenizer::checkToken(string check)
{
	readToken();
	return !(token_current.Cmp(check));
}

/* Tokenizer::getInteger
 * Reads a token and returns its integer value
 *******************************************************************/
int Tokenizer::getInteger()
{
	// Read token
	readToken();

	// Return integer value
	return atoi(CHR(token_current));
}

/* Tokenizer::getInteger
 * Reads a token and writes its integer value to [i]
 *******************************************************************/
void Tokenizer::getInteger(int* i)
{
	// Read token
	readToken();

	// Set integer value
	*i = atoi(CHR(token_current));
}

/* Tokenizer::getFloat
 * Reads a token and returns its floating point value
 *******************************************************************/
float Tokenizer::getFloat()
{
	// Read token
	readToken();

	// Return float value
	return (float) atof(CHR(token_current));
}

/* Tokenizer::getFloat
 * Reads a token and writes its float value to [f]
 *******************************************************************/
void Tokenizer::getFloat(float* f)
{
	// Read token
	readToken();

	// Set float value
	*f = (float)atof(CHR(token_current));
}

/* Tokenizer::getDouble
 * Reads a token and returns its double-precision floating point
 * value
 *******************************************************************/
double Tokenizer::getDouble()
{
	// Read token
	readToken();

	// Return double value
	return atof(CHR(token_current));
}

/* Tokenizer::getDouble
 * Reads a token and writes its double value to [d]
 *******************************************************************/
void Tokenizer::getDouble(double* d)
{
	// Read token
	readToken();

	// Set double value
	*d = atof(CHR(token_current));
}

/* Tokenizer::getBool
 * Reads a token and returns its boolean value, anything except
 * "0", "no", or "false" will return true
 *******************************************************************/
bool Tokenizer::getBool()
{
	// Read token
	readToken();

	// If the token is a string "no" or "false", the value is false
	if (S_CMPNOCASE(token_current, "no") || S_CMPNOCASE(token_current, "false"))
		return false;

	// Returns true ("1") or false ("0")
	return !!atoi(CHR(token_current));
}

/* Tokenizer::getBool
 * Reads a token and writes its boolean value to [b], same rules as
 * getBool above
 *******************************************************************/
void Tokenizer::getBool(bool* b)
{
	// Read token
	readToken();

	// If the token is a string "no" or "false", the value is false
	if (S_CMPNOCASE(token_current, "no") || S_CMPNOCASE(token_current, "false"))
		*b = false;
	else
		*b = !!atoi(CHR(token_current));
}

/* Tokenizer::getTokensUntil
 * Reads tokens into [tokens] until either [end] token is found, or
 * the end of the data is reached
 *******************************************************************/
void Tokenizer::getTokensUntil(vector<string>& tokens, string end)
{
	while (1)
	{
		readToken();
		if (token_current.IsEmpty() && !qstring)
			break;
		if (S_CMPNOCASE(token_current, end))
			break;
		tokens.push_back(token_current);
	}
}

/* Tokenizer::skipSection
 * Skips a 'section' of tokens delimited by [open] and [close]. Also
 * handles nested sections
 *******************************************************************/
void Tokenizer::skipSection(string open, string close)
{
	int level = 0;
	readToken();
	while (!(token_current.empty() && !qstring))
	{
		// Increase depth level if another opener
		if (token_current == open)
			level++;

		// Check for section closer
		else if (token_current == close)
		{
			if (level == 0)
				break;
			else
				level--;
		}

		readToken();
	}
}
