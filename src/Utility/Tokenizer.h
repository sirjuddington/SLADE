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
	bool			caseSensitive() const { return case_sensitive_; }
	const Token&	current() const { return token_current_; }
	const Token&	next(bool inc_index = false);
	void			skip(int inc = 1);
	void			skipToNextLine();
	bool			atEnd() const { return token_current_.pos_start == token_next_.pos_start; }

	// Modifiers
	void	setCommentTypes(CommentTypes types) { comment_types_ = types; }
	void 	setSpecialCharacters(const char* characters);
	void	setSource(const string& source) { source_ = source; }
	void	setCaseSensitive(bool case_sensitive) { case_sensitive_ = case_sensitive; }

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
	void	reset();

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
	bool			case_sensitive_;		// If false, tokens will all be read in lowercase
											// (except for quoted strings, obviously)

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
};
