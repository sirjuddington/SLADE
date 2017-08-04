#pragma once

class TextEditorCtrl;
class TextLanguage;
class Lexer
{
public:
	Lexer();
	~Lexer() {}

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

	void	loadLanguage(TextLanguage* language);

	bool	doStyling(TextEditorCtrl* editor, int start, int end);
	void	addWord(string word, int style);
	void	clearWords() { word_list_.clear(); }

	void	setWordChars(string chars);
	void	setOperatorChars(string chars);

	void	updateFolding(TextEditorCtrl* editor, int line_start);
	void	foldComments(bool fold) { fold_comments_ = fold; }
	void	foldPreprocessor(bool fold) { fold_preprocessor_ = fold; }

private:
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

	vector<char>	word_chars_;
	vector<char>	operator_chars_;
	vector<char>	whitespace_chars_;
	TextLanguage*	language_;
	wxRegEx			re_int1_;
	wxRegEx			re_int2_;
	wxRegEx			re_int3_;
	wxRegEx			re_float_;
	bool			fold_comments_;
	bool			fold_preprocessor_;
	char			preprocessor_char_;

	struct WLIndex
	{
		char style;
		WLIndex() : style(0) {}
	};
	std::map<string, WLIndex>	word_list_;

	struct LineInfo
	{
		bool	commented;
		int		fold_increment;
		bool	has_word;
		LineInfo() : commented{ false }, fold_increment{ 0 }, has_word{ false } {}
	};
	std::map<int, LineInfo>	lines_;

	struct LexerState
	{
		int				position;
		int				end;
		int				line;
		State			state;
		int				length;
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

	void	styleWord(TextEditorCtrl* editor, string word);
	bool	checkToken(TextEditorCtrl* editor, int pos, string& token);
};
