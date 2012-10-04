
#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__
#include "MemChunk.h"

enum CommentTypes {
	CCOMMENTS = 1<<0,		// If true C comments are skipped (// and /* */)
	HCOMMENTS = 1<<1,		// If true hash comments are skipped (##)
	SCOMMENTS = 1<<2,		// If true shell comments are skipped (;)
};

class Tokenizer {
private:
	char*		current;		// Current position
	char*		start;			// Start of text
	uint32_t	size;			// Size of text
	uint32_t	position;		// Current position
	uint8_t		comments;		// See CommentTypes enum
	bool		debug;			// If true every getToken() is printed to the console
	string		special;		// A string defining the 'special characters'. These will always be parsed as separate tokens
	string		name;			// What file/entry/chunk is being tokenized
	bool		qstring;		// True if the last read token was a quoted string
	uint32_t	line;			// The current line number
	uint32_t	t_start;		// The starting position of the last-read token
	uint32_t	t_end;			// The ending position of the last-read token
	string		token_current;	// Current token data


	void	readToken();

public:
	Tokenizer(bool c_comments = true, bool h_comments = true, bool s_comments = false);
	~Tokenizer();

	void	setSpecialCharacters(string special) { this->special = special; }
	void	enableDebug(bool debug = true) { this->debug = debug; }

	string	getName() { return name; }
	bool	openFile(string filename, uint32_t offset = 0, uint32_t length = 0);
	bool	openString(string text, uint32_t offset = 0, uint32_t length = 0, string source = "unknown");
	bool	openMem(const char* mem, uint32_t length, string source);
	bool	openMem(const uint8_t* mem, uint32_t length, string source);
	bool	openMem(MemChunk * mc, string source);
	bool	isWhitespace(char p);
	bool	isSpecialCharacter(char p);
	bool	incrementCurrent();
	void	skipLineComment();
	void	skipMultilineComment();
	void	skipToken();
	string	getToken();
	string	peekToken();
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

	bool		quotedString() { return qstring; }
	uint32_t	lineNo() { return line; }
	uint32_t	tokenStart() { return t_start; }
	uint32_t	tokenEnd() { return t_end; }
};

#endif //__TOKENIZER_H__
