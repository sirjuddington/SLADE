#pragma once

class TextEditor;
class Lexer
{
public:
	Lexer();
	~Lexer() {}

	enum Style
	{
		Default = wxSTC_STYLE_DEFAULT,
		Comment = wxSTC_C_COMMENT,
		String	= wxSTC_C_STRING,
		Char	= wxSTC_C_CHARACTER,
	};

	void	doStyling(TextEditor* editor, int start, int end);
	void	addWord(string word, int style);
	void	clearWords() { word_list.clear(); }
	void	styleWord(TextEditor* editor, string word);
	void	setWordChars(string chars);

private:
	enum class State
	{
		Unknown,
		Word,
		Comment,
		String,
		Char,
	};

	struct WLIndex
	{
		char style;
		WLIndex() : style(0) {}
	};
	std::map<string, WLIndex>	word_list;
	vector<char>				word_chars;
};
