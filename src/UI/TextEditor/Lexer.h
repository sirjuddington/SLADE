#pragma once

class TextEditor;
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
		Property		= wxSTC_C_USERLITERAL,
	};

	void	loadLanguage(TextLanguage* language);

	bool	doStyling(TextEditor* editor, int start, int end);
	void	addWord(string word, int style);
	void	clearWords() { word_list.clear(); }

	void	setWordChars(string chars);
	void	setOperatorChars(string chars);
	void	setLineComment(string token) { comment_line = token; }
	void	setBlockComment(string start, string end) { comment_block_start = start; comment_block_end = end; }
	void	setDocComment(string token) { comment_doc_line = token; }

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

	bool			basic_mode;
	vector<char>	word_chars;
	vector<char>	operator_chars;
	vector<char>	whitespace_chars;
	string			comment_line;
	string			comment_block_start;
	string			comment_block_end;
	string			comment_doc_line;
	char			preprocessor;
	wxRegEx			re_int1;
	wxRegEx			re_int2;
	wxRegEx			re_int3;
	wxRegEx			re_float;

	struct WLIndex
	{
		char style;
		WLIndex() : style(0) {}
	};
	std::map<string, WLIndex>	word_list;

	struct LineInfo
	{
		bool	commented;
		int		fold_level;
		LineInfo() : commented{ false }, fold_level{ 0 } {}
	};
	std::map<int, LineInfo>	lines;

	struct LexerState
	{
		int			position;
		int			end;
		int			line;
		State		state;
		int			length;
		TextEditor*	editor;
	};
	bool	processUnknown(LexerState& state);
	bool	processComment(LexerState& state);
	bool	processWord(LexerState& state);
	bool	processString(LexerState& state);
	bool	processChar(LexerState& state);
	bool	processOperator(LexerState& state);
	bool	processWhitespace(LexerState& state);

	void	styleWord(TextEditor* editor, string word);
	bool	checkToken(TextEditor* editor, int pos, string& token);
};
