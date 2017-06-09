#pragma once

class Tokenizer
{
public:
	enum CommentTypes
	{
		CStyle		= 1,	// C comments (/* */)
		CPPStyle	= 2,	// C++ comments (//)
		Hash		= 4,	// Hash comments (#)
		DoubleHash	= 8,	// Double hash comments (##)
		Shell		= 16,	// Shell comments (;)

		Default = CStyle | CPPStyle | DoubleHash,
	};

	struct Token
	{
		string		text;
		unsigned	line_no;
		bool		quoted_string;
		unsigned	pos_start;
		unsigned	pos_end;

		explicit	operator	string() const { return text; }
		explicit	operator	const string() const { return text; }
		explicit	operator	const char*() const { return CHR(text); }
					operator	char() const { return text[0]; }
		bool		operator	==(const string& cmp) const { return text == cmp; }
		bool		operator	==(const char* cmp) const { return text == cmp; }
		bool		operator	!=(const string& cmp) const { return text != cmp; }
		bool		operator	!=(const char* cmp) const { return text != cmp; }
		char		operator	[](unsigned index) const { return text[index]; }

		bool	isInteger(bool allow_hex = false) const;
		bool	isHex() const;
		bool	isFloat() const;

		int		asInt() const { return wxAtoi(text); }
		void 	asInt(int& val) const { val = wxAtoi(text); }
		bool	asBool() const;
		void 	asBool(bool& val) const;
		double 	asFloat() const { return wxAtof(text); }
		void 	asFloat(double& val) const { val = wxAtof(text); }
	};

	struct TokenizeState
	{
		enum class State
		{
			Unknown,
			Token,
			Comment,
			Whitespace
		};

		State		state = State::Unknown;
		unsigned	position = 0;
		unsigned	size = 0;
		unsigned	current_line = 1;
		unsigned	comment_type = 0;
		Token		current_token;
		bool		done = false;
	};

	// Constructors
	Tokenizer(
		CommentTypes comments = CommentTypes::Default,
		const string& special_characters = DEFAULT_SPECIAL_CHARACTERS
	);

	// Accessors
	const string&	source() const { return source_; }
	bool			decorate() const { return decorate_; }
	bool			readLowerCase() const { return read_lowercase_; }
	const Token&	current() const { return token_current_; }
	const Token&	peek() const;

	// Modifiers
	void	setCommentTypes(CommentTypes types) { comment_types_ = types; }
	void 	setSpecialCharacters(const char* characters)
			{ special_characters_.assign(characters, characters + strlen(characters)); }
	void	setSource(const string& source) { source_ = source; }
	void	setReadLowerCase(bool lower) { read_lowercase_ = lower; }
	void 	enableDecorate(bool enable) { decorate_ = enable; }
	void	enableDebug(bool enable) { debug_ = enable; }

	// Token Iterating
	const Token&	next();
	void			skip(int inc = 1);
	bool			skipIf(const char* check, int inc = 1);
	bool			skipIfNC(const char* check, int inc = 1);
	bool			skipIfNext(const char* check, int inc = 1);
	bool			skipIfNextNC(const char* check, int inc = 1);
	void			skipToNextLine();
	void			skipToEndOfLine();
	void 			skipSection(const char* begin, const char* end, bool allow_quoted = false);
	vector<Token>	getTokensUntil(const char* end);
	vector<Token>	getTokensUntilNC(const char* end);
	vector<Token>	getTokensUntilNextLine(bool from_start = false);
	string			getLine(bool from_start = false);

	// Token Checking
	bool	check(const char* check) const { return token_current_ == check; }
	bool	checkNC(const char* check) const { return S_CMPNOCASE(token_current_.text, check); }
	bool	checkNext(const char* check) const;
	bool	checkNextNC(const char* check) const;

	// Load Data
	bool	openFile(const string& filename, unsigned offset = 0, unsigned length = 0);
	bool	openString(
				const string& text,
				unsigned offset = 0,
				unsigned length = 0,
				const string& source = "unknown"
			);
	bool	openMem(const char* mem, unsigned length, const string& source);
	bool	openMem(const MemChunk& mc, const string& source);

	// General
	bool	isSpecialCharacter(char p) const { return VECTOR_EXISTS(special_characters_, p); }
	bool	atEnd() const { return token_current_.pos_start == token_next_.pos_start; }
	void	reset();

	// Old tokenizer interface bridge (don't use)
	string		getToken()
				{ if (atEnd()) return ""; string t = token_current_.text; skip(); return t; }
	void		getToken(string* str)
				{ if (atEnd()) *str = ""; else *str = token_current_.text; skip(); }
	string		peekToken() const { if (atEnd()) return ""; return token_next_.text; }
	int			getInteger()
				{ if (atEnd()) return 0; int v = token_current_.asInt(); skip(); return v; }
	double		getDouble()
				{ if (atEnd()) return 0; double v = token_current_.asFloat(); skip(); return v; }
	double		getFloat()
				{ if (atEnd()) return 0; double v = token_current_.asFloat(); skip(); return v; }
	void		skipToken() { skip(); }
	bool		checkToken(const string& cmp) { next(); return check(cmp); }
	unsigned	lineNo() const { return token_current_.line_no; }
	unsigned	tokenEnd() const { return token_current_.pos_end; }


	static const string DEFAULT_SPECIAL_CHARACTERS;
	static const Token&	invalidToken() { return invalid_token_; }

private:
	vector<char>	data_;
	Token			token_current_;
	Token			token_next_;
	TokenizeState	state_;

	// Configuration
	CommentTypes	comment_types_;			// Types of comments to skip
	vector<char>	special_characters_;	// These will always be read as separate tokens
	string			source_;				// What file/entry/chunk is being tokenized
	bool			decorate_;				// Special handling for //$ comments
	bool			read_lowercase_;		// If true, tokens will all be read in lowercase
											// (except for quoted strings, obviously)
	bool			debug_;					// Log each token read

	// Static
	static Token	invalid_token_;

	// Tokenizing
	unsigned	checkCommentBegin();
	void		tokenizeUnknown();
	void		tokenizeToken();
	void		tokenizeComment();
	void		tokenizeWhitespace();
	bool		readNext(Token* target);
	bool		readNext() { return readNext(&token_next_); }
	void		resetToLineStart();
};
