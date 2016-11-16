
#include "Main.h"
#include "Lexer.h"
#include "TextEditor.h"

Lexer::Lexer()
{
	// Default word characters
	setWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.$");
}

void Lexer::doStyling(TextEditor* editor, int start, int end)
{
	State state = State::Unknown;
	int length = 0;
	vector<char> current_word;

	editor->StartStyling(start, 0);
	while (start <= end)
	{
		char c = editor->GetCharAt(start);
		char nc = editor->GetCharAt(start + 1);

		// Update state
		switch (state)
		{
		case State::Unknown:
			if (c == '"')
			{
				editor->SetStyling(length, Style::Default);
				state = State::String;
				length = 1;
			}
			else if (c == '\'')
			{
				editor->SetStyling(length, Style::Default);
				state = State::Char;
				length = 1;
			}
			else if (c == '/')
			{
				// Line comment
				if (nc == '/')
				{
					editor->SetStyling(length, Style::Default);
					editor->SetStyling(end - start + 1, Style::Comment);
					return;
				}

				// Block comment
				else if (nc == '*')
				{
					editor->SetStyling(length, Style::Default);
					state = State::Comment;
					length = 2;
					start++; // Skip *
				}
			}
			else if (VECTOR_EXISTS(word_chars, c))
			{
				editor->SetStyling(length, Style::Default);
				state = State::Word;
				current_word.clear();
				current_word.push_back(c);
			}
			else
				length++;
			break;

		case State::String:
			if (c == '"')
			{
				editor->SetStyling(length + 1, Style::String);
				state = State::Unknown;
				length = 0;
			}
			else
				length++;
			break;

		case State::Char:
			if (c == '\'')
			{
				editor->SetStyling(length + 1, Style::Char);
				state = State::Unknown;
				length = 0;
			}
			else
				length++;
			break;

		case State::Comment:
			if (c == '*' && nc == '/')
			{
				editor->SetStyling(length + 2, Style::Comment);
				state = State::Unknown;
				length = 0;
				start++;
			}
			else
				length++;
			break;

		case State::Word:
			if (!(VECTOR_EXISTS(word_chars, c)))
			{
				styleWord(editor, wxString::FromAscii(&current_word[0], current_word.size()));
				state = State::Unknown;
				length = 0;
				start--;
			}
			else
				current_word.push_back(c);
			break;
		}
		
		start++;
	}

	if (length > 0)
	{
		switch (state)
		{
		case State::Comment: editor->SetStyling(length, Style::Comment); break;
		case State::String: editor->SetStyling(length, Style::String); break;
		case State::Char: editor->SetStyling(length, Style::Char); break;
		case State::Word: styleWord(editor, wxString::FromAscii(&current_word[0], current_word.size())); break;
		default: editor->SetStyling(length, Style::Default); break;
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
	else
		editor->SetStyling(word.length(), Style::Default);
}

void Lexer::setWordChars(string chars)
{
	word_chars.clear();
	for (unsigned a = 0; a < chars.length(); a++)
		word_chars.push_back(chars[a]);
}
