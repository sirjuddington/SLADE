
#include "Main.h"
#include "Lexer.h"
#include "TextEditor.h"
#include "TextLanguage.h"

Lexer::Lexer() :
	re_int1{ "^[+-]?[0-9]+[0-9]*$", wxRE_DEFAULT|wxRE_NOSUB },
	re_int2{ "^0[0-9]+$", wxRE_DEFAULT|wxRE_NOSUB },
	re_int3{ "^0x[0-9A-Fa-f]+$", wxRE_DEFAULT|wxRE_NOSUB },
	re_float{ "^[-+]?[0-9]*.?[0-9]+([eE][-+]?[0-9]+)?$", wxRE_DEFAULT|wxRE_NOSUB },
	comment_line{ "//" },
	comment_block_start{ "/*" },
	comment_block_end{ "*/" },
	comment_doc_line{ "///" },
	preprocessor{ '#' },
	whitespace_chars{ { ' ', '\n', '\r', '\t' } },
	basic_mode{ false }
{
	// Default word characters
	setWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");

	// Default operator characters
	setOperatorChars("+-*/=><|~&!");
}

void Lexer::loadLanguage(TextLanguage* language)
{
	clearWords();

	// If no language given, enable basic mode
	if (!language)
	{
		basic_mode = true;
		return;
	}

	// Setup from language
	basic_mode = false;
	comment_line = language->getLineComment();
	comment_block_start = language->getCommentBegin();
	comment_block_end = language->getCommentEnd();
	comment_doc_line = language->getDocComment();
	preprocessor = language->getPreprocessor()[0];

	// Load language words
	for (auto word : language->getWordListSorted(TextLanguage::WordType::Constant))
		addWord(word, Lexer::Style::Constant);
	for (auto word : language->getWordListSorted(TextLanguage::WordType::Property))
		addWord(word, Lexer::Style::Property);
	for (auto word : language->getFunctionsSorted())
		addWord(word, Lexer::Style::Function);
	for (auto word : language->getWordListSorted(TextLanguage::WordType::Type))
		addWord(word, Lexer::Style::Type);
	for (auto word : language->getWordListSorted(TextLanguage::WordType::Keyword))
		addWord(word, Lexer::Style::Keyword);
}

bool Lexer::doStyling(TextEditor* editor, int start, int end)
{
	if (start < 0)
		start = 0;

	int line = editor->LineFromPosition(start);
	LexerState state
	{
		start,
		end,
		line,
		lines[line].commented ? State::Comment : State::Unknown,
		0,
		editor
	};

	editor->StartStyling(start, 0);
	LOG_MESSAGE(3, "START STYLING FROM %d TO %d (LINE %d)", start, end, line + 1);

	bool done = false;
	while (!done)
	{
		switch (state.state)
		{
		case State::Whitespace:
			done = processWhitespace(state); break;
		case State::Comment:
			done = processComment(state); break;
		case State::String:
			done = processString(state); break;
		case State::Char:
			done = processChar(state); break;
		case State::Word:
			done = processWord(state); break;
		case State::Operator:
			done = processOperator(state); break;
		default:
			done = processUnknown(state); break;
		}
	}

	// Set the next line's 'commented' state
	lines[line+1].commented = (state.state == State::Comment);

	// Return true if we are still inside a comment
	return (state.state == State::Comment);
}

void Lexer::addWord(string word, int style)
{
	word_list[word.Lower()].style = style;
}

void Lexer::styleWord(TextEditor* editor, string word)
{
	string wl = word.Lower();

	if (word_list[wl].style > 0)
		editor->SetStyling(word.length(), word_list[wl].style);
	else if (word.StartsWith(preprocessor))
		editor->SetStyling(word.length(), Style::Preprocessor);
	else
	{
		// Check for number
		if (re_int2.Matches(word) || re_int1.Matches(word) || re_float.Matches(word) || re_int3.Matches(word))
			editor->SetStyling(word.length(), Style::Number);
		else
			editor->SetStyling(word.length(), Style::Default);
	}
}

void Lexer::setWordChars(string chars)
{
	word_chars.clear();
	for (unsigned a = 0; a < chars.length(); a++)
		word_chars.push_back(chars[a]);
}

void Lexer::setOperatorChars(string chars)
{
	operator_chars.clear();
	for (unsigned a = 0; a < chars.length(); a++)
		operator_chars.push_back(chars[a]);
}

bool Lexer::processUnknown(LexerState& state)
{
	int u_length = 0;
	bool end = false;
	bool pp = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}
		
		char c = state.editor->GetCharAt(state.position);

		// Start of string
		if (c == '"')
		{
			state.state = State::String;
			state.position++;
			state.length = 1;
			break;
		}

		// Basic mode only supports strings
		else if (basic_mode)
		{
			u_length++;
			state.position++;
			continue;
		}

		// Start of char
		else if (c == '\'')
		{
			state.state = State::Char;
			state.position++;
			state.length = 1;
			break;
		}

		// Start of doc line comment
		else if (checkToken(state.editor, state.position, comment_doc_line))
		{
			// Format as comment to end of line
			state.editor->SetStyling(u_length, Style::Default);
			state.editor->SetStyling((state.end - state.position) + 1, Style::CommentDoc);
			return true;
		}

		// Start of line comment
		else if (checkToken(state.editor, state.position, comment_line))
		{
			// Format as comment to end of line
			state.editor->SetStyling(u_length, Style::Default);
			state.editor->SetStyling((state.end - state.position) + 1, Style::Comment);
			return true;
		}

		// Start of block comment
		else if (checkToken(state.editor, state.position, comment_block_start))
		{
			state.state = State::Comment;
			state.position += comment_block_start.size();
			state.length = comment_block_start.size();
			break;
		}

		// Whitespace
		else if (VECTOR_EXISTS(whitespace_chars, c))
		{
			state.state = State::Whitespace;
			state.position++;
			state.length = 1;
			break;
		}

		// Preprocessor
		else if (c == preprocessor)
		{
			pp = true;
			u_length++;
			state.position++;
			continue;
		}

		// Operator
		else if (VECTOR_EXISTS(operator_chars, c))
		{
			state.position++;
			state.state = State::Operator;
			state.length = 1;
			break;
		}

		// Word
		else if (VECTOR_EXISTS(word_chars, c))
		{
			// Include preprocessor character if it was the previous character
			if (pp)
			{
				state.position--;
				u_length--;
			}

			state.state = State::Word;
			state.length = 0;
			break;
		}

		//LOG_MESSAGE(4, "unknown char '%c' (%d)", c, c);
		u_length++;
		state.position++;
		pp = false;
	}

	//LOG_MESSAGE(4, "unknown:%d", u_length);
	state.editor->SetStyling(u_length, Style::Default);

	return end;
}

bool Lexer::processComment(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of comment
		if (checkToken(state.editor, state.position, comment_block_end))
		{
			state.length += comment_block_end.size();
			state.position += comment_block_end.size();
			state.state = State::Unknown;
			break;
		}

		state.length++;
		state.position++;
	}

	LOG_MESSAGE(4, "comment:%d", state.length);
	state.editor->SetStyling(state.length, Style::Comment);

	return end;
}

bool Lexer::processWord(LexerState& state)
{
	vector<char> word;
	bool end = false;

	// Add first letter
	word.push_back(state.editor->GetCharAt(state.position++));

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(word_chars, c))
		{
			word.push_back(c);
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	string word_string = wxString::FromAscii(&word[0], word.size());
	LOG_MESSAGE(4, "word:%s", word_string);
	styleWord(state.editor, word_string);

	return end;
}

bool Lexer::processString(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of string
		char c = state.editor->GetCharAt(state.position);
		if (c == '"')	
		{
			state.length++;
			state.position++;
			state.state = State::Unknown;
			break;
		}

		state.length++;
		state.position++;
	}

	LOG_MESSAGE(4, "string:%d", state.length);
	state.editor->SetStyling(state.length, Style::String);

	return end;
}

bool Lexer::processChar(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		// End of string
		char c = state.editor->GetCharAt(state.position);
		if (c == '\'')
		{
			state.length++;
			state.position++;
			state.state = State::Unknown;
			break;
		}

		state.length++;
		state.position++;
	}

	LOG_MESSAGE(4, "char:%d", state.length);
	state.editor->SetStyling(state.length, Style::Char);

	return end;
}

bool Lexer::processOperator(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(operator_chars, c))
		{
			state.length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	LOG_MESSAGE(4, "operator:%d", state.length);
	state.editor->SetStyling(state.length, Style::Operator);

	return end;
}

bool Lexer::processWhitespace(LexerState& state)
{
	bool end = false;

	while (true)
	{
		// Check for end of line
		if (state.position > state.end)
		{
			end = true;
			break;
		}

		char c = state.editor->GetCharAt(state.position);
		if (VECTOR_EXISTS(whitespace_chars, c))
		{
			state.length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	LOG_MESSAGE(4, "whitespace:%d", state.length);
	state.editor->SetStyling(state.length, Style::Default);

	return end;
}

bool Lexer::checkToken(TextEditor* editor, int pos, string& token)
{
	if (!token.empty())
	{
		for (int a = 0; a < token.size(); a++)
			if (editor->GetCharAt(pos + a) != (int)token[a])
				return false;

		return true;
	}

	return false;
}
