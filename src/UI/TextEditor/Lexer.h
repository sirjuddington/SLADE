#pragma once

class TextEditor;
class Lexer
{
public:
	Lexer();
	~Lexer() {}

	enum Style
	{
		Default			= wxSTC_STYLE_DEFAULT,
		Comment			= wxSTC_C_COMMENT,
		String			= wxSTC_C_STRING,
		Char			= wxSTC_C_CHARACTER,
		Number			= wxSTC_C_NUMBER,
		Operator		= wxSTC_C_OPERATOR,
		Preprocessor	= wxSTC_C_PREPROCESSOR,
	};

	void	doStyling(TextEditor* editor, int start, int end);
	void	addWord(string word, int style);
	void	clearWords() { word_list.clear(); }
	void	styleWord(TextEditor* editor, string word);
	void	setWordChars(string chars);
	void	setOperatorChars(string chars);

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

	struct WLIndex
	{
		char style;
		WLIndex() : style(0) {}
	};
	std::map<string, WLIndex>	word_list;
	vector<char>				word_chars;
	vector<char>				operator_chars;
	string						comment_line;
	string						comment_block_start;
	string						comment_block_end;
	char						preprocessor;

	struct LexerState
	{
		int			position;
		int			end;
		State		state;
		TextEditor*	editor;
	};
	bool	processUnknown(LexerState& state);
	bool	processComment(LexerState& state);
	bool	processWord(LexerState& state);
	bool	processString(LexerState& state);
	bool	processChar(LexerState& state);
	bool	processNumber(LexerState& state);
	bool	processOperator(LexerState& state);
	bool	processWhitespace(LexerState& state);
};
