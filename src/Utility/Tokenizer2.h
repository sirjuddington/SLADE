#pragma once

class Tokenizer2
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
	};

	// Constructors
	Tokenizer2(
		CommentTypes comments = CommentTypes::Default,
		const string& special_characters = DEFAULT_SPECIAL_CHARACTERS
	);

	// Accessors
	const vector<Token>&	tokens() const { return tokens_; }
	const string&			source() const { return source_; }
	bool					decorate() const { return decorate_; }
	bool					caseSensitive() const { return case_sensitive_; }

	// Modifiers
	void	setCommentTypes(CommentTypes types) { comment_types_ = types; }
	void	setSpecialCharacters(const string& special_characters);
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

	// Tokenize
	void	tokenize();

	static const string DEFAULT_SPECIAL_CHARACTERS;

private:
	vector<char>	data_;
	vector<Token>	tokens_;

	// Configuration
	CommentTypes	comment_types_;			// Types of comments to skip
	vector<char>	special_characters_;	// These will always be read as separate tokens
	string			source_;				// What file/entry/chunk is being tokenized
	bool			decorate_;				// Special handling for //$ comments
	bool			case_sensitive_;		// If false, tokens will all be read in lowercase
											// (except for quoted strings, obviously)

	// Tokenizing
	unsigned	checkCommentBegin(const TokenizeState& state);
	bool		isSpecialCharacter(char p) { return VECTOR_EXISTS(special_characters_, p); }
	void		tokenizeUnknown(TokenizeState& state);
	void		tokenizeToken(TokenizeState& state);
	void		tokenizeComment(TokenizeState& state);
	void		tokenizeWhitespace(TokenizeState& state);
	void		addCurrentToken(const TokenizeState& state);
};
