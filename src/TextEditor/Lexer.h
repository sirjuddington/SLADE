#pragma once

class TextEditorCtrl;
class TextLanguage;
class Lexer
{
public:
	Lexer();
	virtual ~Lexer() {}

	enum Style
	{
		Default			= wxSTC_STYLE_DEFAULT,
		Comment			= wxSTC_C_COMMENT,
		CommentDoc		= wxSTC_C_COMMENTDOC,
		String			= wxSTC_C_STRING,
		Char			= wxSTC_C_CHARACTER,
		Number			= wxSTC_C_NUMBER,
		Operator		= wxSTC_C_OPERATOR,
		Preprocessor	= wxSTC_C_PREPROCESSOR,

		// Words
		Keyword			= wxSTC_C_WORD,
		Function		= wxSTC_C_WORD2,
		Constant		= wxSTC_C_GLOBALCLASS,
		Type			= wxSTC_C_IDENTIFIER,
		Property		= wxSTC_C_UUID,
	};

	virtual void	loadLanguage(TextLanguage* language);

	virtual bool	doStyling(TextEditorCtrl* editor, int start, int end);
	virtual void	addWord(string word, int style);
	virtual void	clearWords() { word_list_.clear(); }

	void	setWordChars(string chars);
	void	setOperatorChars(string chars);

	void	updateFolding(TextEditorCtrl* editor, int line_start);
	void	foldComments(bool fold) { fold_comments_ = fold; }
	void	foldPreprocessor(bool fold) { fold_preprocessor_ = fold; }

	virtual bool isFunction(TextEditorCtrl* editor, int start_pos, int end_pos);

protected:
	enum class State
	{
		Unknown,
		Word,
		Comment,
		String,
		Char,
		Number,
		Operator,
		Whitespace,
	};

	vector<unsigned char>	word_chars_;
	vector<unsigned char>	operator_chars_;
	vector<unsigned char>	whitespace_chars_;
	TextLanguage*			language_;
	wxRegEx					re_int1_;
	wxRegEx					re_int2_;
	wxRegEx					re_int3_;
	wxRegEx					re_float_;
	bool					fold_comments_;
	bool					fold_preprocessor_;
	char					preprocessor_char_;
	int						curr_comment_idx_;

	struct WLIndex
	{
		char style;
		WLIndex() : style(0) {}
	};
	std::map<string, WLIndex>	word_list_;

	struct LineInfo
	{
		int 	comment_idx;
		int		fold_increment;
		bool	has_word;
		LineInfo() : comment_idx { -1 },
					 fold_increment { 0 },
					 has_word { false } {}
	};
	std::map<int, LineInfo> lines_;

	struct LexerState
	{
		int				position;
		int				end;
		int				line;
		State			state;
		size_t			length;
		int				fold_increment;
		bool			has_word;
		TextEditorCtrl*	editor;
	};
	bool	processUnknown(LexerState& state);
	bool	processComment(LexerState& state);
	bool	processWord(LexerState& state);
	bool	processString(LexerState& state);
	bool	processChar(LexerState& state);
	bool	processOperator(LexerState& state);
	bool	processWhitespace(LexerState& state);

	virtual void	styleWord(LexerState& state, string word);
	bool			checkToken(LexerState& state, int pos, string& token);
	bool			checkToken(LexerState &state, int pos,
							   vector<string> &tokens,
							   int *found_idx = nullptr);
};

class ZScriptLexer : public Lexer
{
public:
	ZScriptLexer() {}
	virtual ~ZScriptLexer() {}

protected:
	void addWord(string word, int style) override;
	void styleWord(LexerState& state, string word) override;
	void clearWords() override;
	bool isFunction(TextEditorCtrl* editor, int start_pos, int end_pos) override;

private:
	vector<string>	functions_;
};
