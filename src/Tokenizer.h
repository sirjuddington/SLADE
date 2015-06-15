
#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__
#include "MemChunk.h"

enum CommentTypes
{
	CCOMMENTS = 1<<0,		// C comments are skipped (// and /* */)
	HCOMMENTS = 1<<1,		// hash comments are skipped (#)
	DCOMMENTS = 1<<2,		// double hash comments are skipped (##)
	SCOMMENTS = 1<<4,		// shell comments are skipped (;)

	COMMENTS_DEFAULT = CCOMMENTS|DCOMMENTS,
};

class Tokenizer
{
private:
	char*		current;		// Current position
	char*		start;			// Start of text
	char*		end;			// End of text
	uint32_t	size;			// Size of text
	uint32_t	position;		// Current position
	uint8_t		comments;		// See CommentTypes enum
	bool		debug;			// If true every getToken() is printed to the console
	string		special;		// A string defining the 'special characters',
								// these will always be parsed as separate tokens
	string		name;			// What file/entry/chunk is being tokenized
	bool		qstring;		// True if the last read token was a quoted string
	uint32_t	line;			// The current line number
	uint32_t	t_start;		// The starting position of the last-read token
	uint32_t	t_end;			// The ending position of the last-read token
	string		token_current;	// Current token data
	bool		decorate;		// Whether to parse doom builder //$ decorate comments


	void	readToken(bool toeol = false);

public:
	Tokenizer(CommentTypes comments_style = COMMENTS_DEFAULT);
	~Tokenizer();

	void	setSpecialCharacters(string special) { this->special = special; }
	void	enableDebug(bool debug = true) { this->debug = debug; }
	void	enableDecorate(bool enable) { decorate = enable; }

	string	getName() { return name; }
	bool	openFile(string filename, uint32_t offset = 0, uint32_t length = 0);
	bool	openString(string text, uint32_t offset = 0, uint32_t length = 0, string source = "unknown");
	bool	openMem(const char* mem, uint32_t length, string source);
	bool	openMem(const uint8_t* mem, uint32_t length, string source);
	bool	openMem(MemChunk* mc, string source);
	bool	isWhitespace(char p);
	bool	isSpecialCharacter(char p);
	bool	isAtEnd() { return (token_current.IsEmpty() && !qstring); }
	bool	incrementCurrent();
	void	skipLineComment();
	void	skipMultilineComment();
	void	skipToken();
	string	getToken();
	string	peekToken();
	string	getLine();
	bool	checkToken(string check);
	int		getInteger();
	float	getFloat();
	double	getDouble();
	bool	getBool();

	void	getToken(string* s);
	void	getInteger(int* i);
	void	getFloat(float* f);
	void	getDouble(double* d);
	void	getBool(bool* b);

	void	getTokensUntil(vector<string>& tokens, string end);

	bool		quotedString() { return qstring; }
	uint32_t	lineNo() { return line; }
	uint32_t	tokenStart() { return t_start; }
	uint32_t	tokenEnd() { return t_end; }
	bool		atEnd() { return position == size; }

	void	skipSection(string open, string close);
};

#endif //__TOKENIZER_H__
