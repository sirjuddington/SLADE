
#include "Main.h"
#include "Lexer.h"
#include "TextEditor.h"

vector<char> char_num_start = { '0','1','2','3','4','5','6','7','8','9' };
vector<char> char_num = { '0','1','2','3','4','5','6','7','8','9','.','x','a','b','c','d','e','f' };
vector<char> char_whitespace = { ' ', '\n', '\r', '\t' };

Lexer::Lexer()
{
	// Default word characters
	setWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.$");

	// Default operator characters
	setOperatorChars("+-*/=><|~&!");

	// Default comments
	comment_line = "//";
	comment_block_start = "/*";
	comment_block_end = "*/";

	// Default preprocessor
	preprocessor = '#';
}

void Lexer::doStyling(TextEditor* editor, int start, int end)
{
	if (start < 0)
		start = 0;

	LexerState state{ start, end, State::Unknown, editor };

	editor->StartStyling(start, 0);
	//LOG_MESSAGE(3, "START STYLING FROM %d TO %d", start, end);

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
		case State::Number:
			done = processNumber(state); break;
		case State::Operator:
			done = processOperator(state); break;
		default:
			done = processUnknown(state); break;
		}
	}
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
		editor->SetStyling(word.length(), Style::Default);
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
	int length = 0;
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
			break;
		}

		// Start of char
		else if (c == '\'')
		{
			state.state = State::Char;
			state.position++;
			break;
		}

		// Start of line comment
		else if (!comment_line.IsEmpty() && c == comment_line[0])
		{
			// Check following characters are the line comment string
			bool comment = true;
			for (int a = 1; a < comment_line.size(); a++)
				if (state.editor->GetCharAt(state.position + a) != (int)comment_line[a])
					comment = false;

			if (comment)
			{
				// Format as comment to end of line
				state.editor->SetStyling(length, Style::Default);
				state.editor->SetStyling((state.end - state.position) + 1, Style::Comment);
				return true;
			}
		}

		// Whitespace
		else if (VECTOR_EXISTS(char_whitespace, c))
		{
			state.state = State::Whitespace;
			state.position++;
			break;
		}

		// Preprocessor
		else if (c == preprocessor)
		{
			pp = true;
			length++;
			state.position++;
			continue;
		}

		// Operator
		else if (VECTOR_EXISTS(operator_chars, c))
		{
			state.position++;
			state.state = State::Operator;
			break;
		}

		// Number
		else if (VECTOR_EXISTS(char_num_start, c))
		{
			state.state = State::Number;
			break;
		}

		// Word
		else if (VECTOR_EXISTS(word_chars, c))
		{
			// Include preprocessor character if it was the previous character
			if (pp)
			{
				state.position--;
				length--;
			}

			state.state = State::Word;
			break;
		}

		//LOG_MESSAGE(4, "unknown char '%c' (%d)", c, c);
		length++;
		state.position++;
		pp = false;
	}

	LOG_MESSAGE(4, "unknown:%d", length);
	state.editor->SetStyling(length, Style::Default);

	return end;
}

bool Lexer::processComment(LexerState& state)
{
	return false;
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
	int length = 1;
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
			length++;
			state.position++;
			state.state = State::Unknown;
			break;
		}

		length++;
		state.position++;
	}

	LOG_MESSAGE(4, "string:%d", length);
	state.editor->SetStyling(length, Style::String);

	return end;
}

bool Lexer::processChar(LexerState& state)
{
	int length = 1;
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
			length++;
			state.position++;
			state.state = State::Unknown;
			break;
		}

		length++;
		state.position++;
	}

	LOG_MESSAGE(4, "char:%d", length);
	state.editor->SetStyling(length, Style::Char);

	return end;
}

bool Lexer::processNumber(LexerState& state)
{
	int length = 0;
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
		if (VECTOR_EXISTS(char_num, c))
		{
			length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	LOG_MESSAGE(4, "number:%d", length);
	state.editor->SetStyling(length, Style::Number);

	return end;
}

bool Lexer::processOperator(LexerState& state)
{
	int length = 1;
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
			length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	LOG_MESSAGE(4, "operator:%d", length);
	state.editor->SetStyling(length, Style::Operator);

	return end;
}

bool Lexer::processWhitespace(LexerState& state)
{
	int length = 1;
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
		if (VECTOR_EXISTS(char_whitespace, c))
		{
			length++;
			state.position++;
		}
		else
		{
			state.state = State::Unknown;
			break;
		}
	}

	LOG_MESSAGE(4, "whitespace:%d", length);
	state.editor->SetStyling(length, Style::Default);

	return end;
}
