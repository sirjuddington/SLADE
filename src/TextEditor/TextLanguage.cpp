
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TextLanguage.cpp
 * Description: Defines a 'language' for use by the TextEditor for
 *              syntax hilighting/autocompletion/etc. Contains lists
 *              of keywords, constants and functions, with various
 *              utility functions for using them.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "TextLanguage.h"
#include "Utility/Tokenizer.h"
#include "Utility/Parser.h"
#include "Archive/ArchiveManager.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
vector<TextLanguage*>	text_languages;


/*******************************************************************
 * TLFUNCTION CLASS FUNCTIONS
 *******************************************************************/

/* TLFunction::TLFunction
 * TLFunction class constructor
 *******************************************************************/
TLFunction::TLFunction(string name, string return_type) :
	name_{ name },
	return_type_{ return_type }
{
}

/* TLFunction::~TLFunction
 * TLFunction class destructor
 *******************************************************************/
TLFunction::~TLFunction()
{
}

/* TLFunction::getArgSet
 * Returns the arg set [index], or an empty string if [index] is
 * out of bounds
 *******************************************************************/
TLFunction::ArgSet TLFunction::argSet(unsigned index) const
{
	// Check index
	if (index >= arg_sets_.size())
		return { "", "" };

	return arg_sets_[index];
}

/* TLFunction::generateCallTipString
 * Returns a string representation of arg set [arg_set] that can be
 * used directly in a scintilla calltip
 *******************************************************************/
string TLFunction::generateCallTipString(int arg_set)
{
	// Check requested arg set exists
	if (arg_set < 0 || (unsigned)arg_set >= arg_sets_.size())
		return "<invalid argset index>";

	string calltip;

	// Add extra buttons for selection if there is more than one arg set
	if (arg_sets_.size() > 1)
		calltip += S_FMT("\001 %d of %lu \002 ", arg_set+1, arg_sets_.size());

	// Generate scintilla-format calltip string
	calltip += name_ + "(";
	calltip += arg_sets_[arg_set].args;
	calltip += ")";

	return calltip;
}

/* TLFunction::getArgTextExtent
 * Returns the start and end position of [arg] within [arg_set]
 *******************************************************************/
point2_t TLFunction::getArgTextExtent(int arg, int arg_set)
{
	point2_t extent(-1, -1);

	// Check requested arg set exists
	if (arg_set < 0 || (unsigned)arg_set >= arg_sets_.size())
		return extent;

	// Get start position of args list
	int start_pos = name_.Length() + 1;
	if (arg_sets_.size() > 1)
	{
		string temp = S_FMT("\001 %d of %lu \002 ", arg_set+1, arg_sets_.size());
		start_pos += temp.Length();
	}

	// Check arg
	string args = arg_sets_[arg_set].args;
	if (arg < 0)
	{
		extent.set(start_pos, start_pos + args.Length());
		return extent;
	}

	// Go through arg set string
	int current_arg = 0;
	extent.x = start_pos;
	extent.y = start_pos + args.Length();
	for (unsigned a = 0; a < args.Length(); a++)
	{
		// Check for ,
		if (args.at(a) == ',')
		{
			// ',' found, so increment current arg
			current_arg++;

			// If we're at the start of the arg we want
			if (current_arg == arg)
				extent.x = start_pos + a+1;

			// If we've reached the end of the arg we want
			if (current_arg > arg)
			{
				extent.y = start_pos + a;
				break;
			}
		}
	}

	return extent;
}


/*******************************************************************
 * TEXTLANGUAGE CLASS FUNCTIONS
 *******************************************************************/

/* TextLanguage::TextLanguage
 * TextLanguage class constructor
 *******************************************************************/
TextLanguage::TextLanguage(string id) :
	line_comment_{ "//" },
	comment_begin_{ "/*" },
	comment_end_{ "*/" },
	preprocessor_{ "#" },
	block_begin_{ "{" },
	block_end_{ "}" }
{
	// Init variables
	this->id_ = id;

	// Add to languages list
	text_languages.push_back(this);
}

/* TextLanguage::~TextLanguage
 * TextLanguage class destructor
 *******************************************************************/
TextLanguage::~TextLanguage()
{
	// Remove from languages list
	size_t text_languages_size = text_languages.size();
	for (size_t a = 0; a < text_languages_size; a++)
	{
		if (text_languages[a] == this)
			text_languages.erase(text_languages.begin() + a);
	}
}

/* TextLanguage::copyTo
 * Copies all language info to [copy]
 *******************************************************************/
void TextLanguage::copyTo(TextLanguage* copy)
{
	// Copy general attributes
	copy->line_comment_ = line_comment_;
	copy->comment_begin_ = comment_begin_;
	copy->comment_end_ = comment_end_;
	copy->preprocessor_ = preprocessor_;
	copy->case_sensitive_ = case_sensitive_;
	copy->f_lookup_url_ = f_lookup_url_;
	copy->doc_comment_ = doc_comment_;
	copy->block_begin_ = block_begin_;
	copy->block_end_ = block_end_;

	// Copy word lists
	for (unsigned a = 0; a < 4; a++)
		copy->word_lists_[a] = word_lists_[a];

	// Copy functions
	size_t functions_size = functions_.size();
	for (unsigned a = 0; a < functions_size; a++)
	{
		TLFunction* f = functions_[a];
		size_t nargsets = f->nArgSets();
		for (unsigned b = 0; b < nargsets; b++)
			copy->addFunction(
				f->name(),
				f->argSet(b).args,
				f->description(),
				false,
				f->returnType()
			);
	}

	// Copy preprocessor/word block begin/end
	copy->pp_block_begin_ = pp_block_begin_;
	copy->pp_block_end_ = pp_block_end_;
	copy->word_block_begin_ = word_block_begin_;
	copy->word_block_end_ = word_block_end_;

	/*size_t pp_block_begin_size = pp_block_begin_.size();

	for (unsigned a = 0; a < pp_block_begin_size; a++)
		copy->pp_block_begin_.push_back(pp_block_begin_[a]);

	size_t pp_block_end_size = pp_block_end_.size();

	for (unsigned a = 0; a < pp_block_end_size; a++)
		copy->pp_block_end_.push_back(pp_block_end_[a]);*/
}

/* TextLanguage::addWord
 * Adds a new word of [type] to the language, if it doesn't exist
 * already
 *******************************************************************/
void TextLanguage::addWord(WordType type, string keyword)
{
	// Add only if it doesn't already exist
	vector<string>& list = word_lists_[type].list;
	if (std::find(list.begin(), list.end(), keyword) == list.end())
		list.push_back(keyword);
}

/* TextLanguage::addFunction
 * Adds a function arg set to the language. If the function [name]
 * exists, [args] will be added to it as a new arg set, otherwise
 * a new function will be added
 *******************************************************************/
void TextLanguage::addFunction(string name, string args, string desc, bool replace, string return_type)
{
	// Split out context from name
	string context;
	if (name.Contains("."))
	{
		string fname;
		context = name.BeforeFirst('.', &fname);
		name = fname;
	}

	// Check if the function exists
	TLFunction* func = function(name);

	// If it doesn't, create it
	if (!func)
	{
		func = new TLFunction(name, return_type.empty() ? "void" : return_type);
		functions_.push_back(func);
	}

	// Remove/recreate the function if we're replacing it
	else if (replace)
	{
		VECTOR_REMOVE(functions_, func);
		delete func;
		func = new TLFunction(name, return_type.empty() ? "void" : return_type);
		functions_.push_back(func);
	}

	// Add the arg set
	func->addArgSet(args, context);

	// Set description
	func->setDescription(desc);
}

/* TextLanguage::getWordList
 * Returns a string of all words of [type] in the language, separated
 * by spaces, which can be sent directly to scintilla for syntax
 * hilighting
 *******************************************************************/
string TextLanguage::wordList(WordType type)
{
	// Init return string
	string ret = "";

	// Add each word to return string (separated by spaces)
	vector<string>& list = word_lists_[type].list;
	for (size_t a = 0; a < list.size(); a++)
		ret += list[a] + " ";

	return ret;
}

/* TextLanguage::getFunctionsList
 * Returns a string of all functions in the language, separated by
 * spaces, which can be sent directly to scintilla for syntax
 * hilighting
 *******************************************************************/
string TextLanguage::functionsList()
{
	// Init return string
	string ret = "";

	// Add each function name to return string (separated by spaces)
	for (unsigned a = 0; a < functions_.size(); a++)
		ret += functions_[a]->name() + " ";

	return ret;
}

/* TextLanguage::getAutocompletionList
 * Returns a string containing all words and functions that can be
 * used directly in scintilla for an autocompletion list
 *******************************************************************/
string TextLanguage::autocompletionList(string start)
{
	// Firstly, add all functions and word lists to a wxArrayString
	wxArrayString list;
	start = start.Lower();

	// Add word lists
	for (unsigned type = 0; type < 4; type++)
		for (unsigned a = 0; a < word_lists_[type].list.size(); a++)
			if (word_lists_[type].list[a].Lower().StartsWith(start))
				list.Add(word_lists_[type].list[a] + S_FMT("?%d", type + 1));

	// Add functions
	for (unsigned a = 0; a < functions_.size(); a++)
	{
		if (functions_[a]->name().Lower().StartsWith(start))
			list.Add(functions_[a]->name() + "?3");
	}

	// Sort the list
	list.Sort();

	// Now build a string of the list items separated by spaces
	string ret;
	for (unsigned a = 0; a < list.size(); a++)
		ret += list[a] + " ";

	return ret;
}

/* TextLanguage::getWordListSorted
 * Returns a sorted wxArrayString of all words of [type] in the
 * language
 *******************************************************************/
wxArrayString TextLanguage::wordListSorted(WordType type)
{
	// Get list
	wxArrayString list;
	for (unsigned a = 0; a < word_lists_[type].list.size(); a++)
		list.Add(word_lists_[type].list[a]);

	// Sort
	list.Sort();

	return list;
}

/* TextLanguage::getFunctionsSorted
 * Returns a sorted wxArrayString of all functions in the language
 *******************************************************************/
wxArrayString TextLanguage::functionsSorted()
{
	// Get list
	wxArrayString list;
	for (unsigned a = 0; a < functions_.size(); a++)
		list.Add(functions_[a]->name());

	// Sort
	list.Sort();

	return list;
}

/* TextLanguage::isWord
 * Returns true if [word] is a [type] word in this language, false
 * otherwise
 *******************************************************************/
bool TextLanguage::isWord(WordType type, string word)
{
	vector<string>& list = word_lists_[type].list;
	for (unsigned a = 0; a < list.size(); a++)
	{
		if (list[a] == word)
			return true;
	}

	return false;
}

/* TextLanguage::isFunction
 * Returns true if [word] is a function in this language, false
 * otherwise
 *******************************************************************/
bool TextLanguage::isFunction(string word)
{
	for (unsigned a = 0; a < functions_.size(); a++)
	{
		if (functions_[a]->name() == word)
			return true;
	}

	return false;
}

/* TextLanguage::getFunction
 * Returns the function definition matching [name], or NULL if no
 * matching function exists
 *******************************************************************/
TLFunction* TextLanguage::function(string name)
{
	// Find function matching [name]
	size_t functions_size = functions_.size();
	if (case_sensitive_)
	{
		for (unsigned a = 0; a < functions_size; a++)
		{
			if (functions_[a]->name() == name)
				return functions_[a];
		}
	}
	else
	{
		for (unsigned a = 0; a < functions_size; a++)
		{
			if (S_CMPNOCASE(functions_[a]->name(), name))
				return functions_[a];
		}
	}

	// Not found
	return NULL;
}


/*******************************************************************
 * TEXTLANGUAGE STATIC FUNCTIONS
 *******************************************************************/

/* TextLanguage::readLanguageDefinition
 * Reads in a text definition of a language. See slade.pk3 for
 * formatting examples
 *******************************************************************/
bool TextLanguage::readLanguageDefinition(MemChunk& mc, string source)
{
	Tokenizer tz;

	// Open the given text data
	if (!tz.openMem(&mc, source))
	{
		LOG_MESSAGE(1, "Unable to open file");
		return false;
	}

	// Parse the definition text
	ParseTreeNode root;
	if (!root.parse(tz))
		return false;

	// Get parsed data
	for (unsigned a = 0; a < root.nChildren(); a++)
	{
		auto node = root.getChildPTN(a);

		// Create language
		TextLanguage* lang = new TextLanguage(node->getName());

		// Check for inheritance
		if (!node->inherit().IsEmpty())
		{
			TextLanguage* inherit = fromId(node->inherit());
			if (inherit)
				inherit->copyTo(lang);
			else
				LOG_MESSAGE(1, "Warning: Language %s inherits from undefined language %s", node->getName(), node->inherit());
		}

		// Parse language info
		for (unsigned c = 0; c < node->nChildren(); c++)
		{
			auto child = node->getChildPTN(c);

			// Language name
			if (S_CMPNOCASE(child->getName(), "name"))
				lang->setName(child->stringValue());

			// Comment begin
			else if (S_CMPNOCASE(child->getName(), "comment_begin"))
				lang->setCommentBegin(child->stringValue());

			// Comment end
			else if (S_CMPNOCASE(child->getName(), "comment_end"))
				lang->setCommentEnd(child->stringValue());

			// Line comment
			else if (S_CMPNOCASE(child->getName(), "comment_line"))
				lang->setLineComment(child->stringValue());

			// Preprocessor
			else if (S_CMPNOCASE(child->getName(), "preprocessor"))
				lang->setPreprocessor(child->stringValue());

			// Case sensitive
			else if (S_CMPNOCASE(child->getName(), "case_sensitive"))
				lang->setCaseSensitive(child->boolValue());

			// Doc comment
			else if (S_CMPNOCASE(child->getName(), "comment_doc"))
				lang->setDocComment(child->stringValue());

			// Keyword lookup link
			else if (S_CMPNOCASE(child->getName(), "keyword_link"))
				lang->word_lists_[WordType::Keyword].lookup_url = child->stringValue();

			// Constant lookup link
			else if (S_CMPNOCASE(child->getName(), "constant_link"))
				lang->word_lists_[WordType::Constant].lookup_url = child->stringValue();

			// Function lookup link
			else if (S_CMPNOCASE(child->getName(), "function_link"))
				lang->f_lookup_url_ = child->stringValue();

			// Jump blocks
			else if (S_CMPNOCASE(child->getName(), "blocks"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->jump_blocks_.push_back(child->stringValue(v));
			}
			else if (S_CMPNOCASE(child->getName(), "blocks_ignore"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->jb_ignore_.push_back(child->stringValue(v));
			}

			// Block begin
			else if (S_CMPNOCASE(child->getName(), "block_begin"))
				lang->block_begin_ = child->stringValue();

			// Block end
			else if (S_CMPNOCASE(child->getName(), "block_end"))
				lang->block_end_ = child->stringValue();

			// Preprocessor block begin
			else if (S_CMPNOCASE(child->getName(), "pp_block_begin"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->pp_block_begin_.push_back(child->stringValue(v));
			}

			// Preprocessor block end
			else if (S_CMPNOCASE(child->getName(), "pp_block_end"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->pp_block_end_.push_back(child->stringValue(v));
			}

			// Word block begin
			else if (S_CMPNOCASE(child->getName(), "word_block_begin"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->word_block_begin_.push_back(child->stringValue(v));
			}

			// Word block end
			else if (S_CMPNOCASE(child->getName(), "word_block_end"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->word_block_end_.push_back(child->stringValue(v));
			}

			// Keywords
			else if (S_CMPNOCASE(child->getName(), "keywords"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					string val = child->stringValue(v);

					// Check for '$override'
					if (S_CMPNOCASE(val, "$override"))
					{
						// Clear any inherited keywords
						lang->clearWordList(WordType::Keyword);
					}

					// Not a special symbol, add as keyword
					else
						lang->addWord(WordType::Keyword, val);
				}
			}

			// Constants
			else if (S_CMPNOCASE(child->getName(), "constants"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					string val = child->stringValue(v);

					// Check for '$override'
					if (S_CMPNOCASE(val, "$override"))
					{
						// Clear any inherited constants
						lang->clearWordList(WordType::Constant);
					}

					// Not a special symbol, add as constant
					else
						lang->addWord(WordType::Constant, val);
				}
			}

			// Types
			else if (S_CMPNOCASE(child->getName(), "types"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					string val = child->stringValue(v);

					// Check for '$override'
					if (S_CMPNOCASE(val, "$override"))
					{
						// Clear any inherited constants
						lang->clearWordList(WordType::Type);
					}

					// Not a special symbol, add as constant
					else
						lang->addWord(WordType::Type, val);
				}
			}

			// Properties
			else if (S_CMPNOCASE(child->getName(), "properties"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					string val = child->stringValue(v);

					// Check for '$override'
					if (S_CMPNOCASE(val, "$override"))
					{
						// Clear any inherited constants
						lang->clearWordList(WordType::Property);
					}

					// Not a special symbol, add as constant
					else
						lang->addWord(WordType::Property, val);
				}
			}

			// Functions
			else if (S_CMPNOCASE(child->getName(), "functions"))
			{
				// Go through children (functions)
				for (unsigned f = 0; f < child->nChildren(); f++)
				{
					auto child_func = child->getChildPTN(f);

					// Simple definition
					if (child_func->nChildren() == 0)
					{
						// Add function
						lang->addFunction(
							child_func->getName(),
							child_func->stringValue(0),
							"",
							!child_func->getName().Contains("."),
							child_func->type());

						// Add args
						for (unsigned v = 1; v < child_func->nValues(); v++)
							lang->addFunction(child_func->getName(), child_func->stringValue(v));
					}

					// Full definition
					else
					{
						string name = child_func->getName();
						string desc = "";
						vector<string> args;
						for (unsigned p = 0; p < child_func->nChildren(); p++)
						{
							auto child_prop = child_func->getChildPTN(p);
							if (child_prop->getName() == "args")
							{
								for (unsigned v = 0; v < child_prop->nValues(); v++)
									args.push_back(child_prop->stringValue(v));
							}
							else if (child_prop->getName() == "description")
								desc = child_prop->stringValue();
						}

						for (unsigned as = 0; as < args.size(); as++)
							lang->addFunction(name, args[as], desc, as == 0, child_func->type());
					}
				}
			}
		}
	}

	return true;
}

/* TextLanguage::loadLanguages
 * Loads all text language definitions from slade.pk3
 *******************************************************************/
bool TextLanguage::loadLanguages()
{
	// Get slade resource archive
	Archive* res_archive = App::archiveManager().programResourceArchive();

	// Read language definitions from resource archive
	if (res_archive)
	{
		// Get 'config/languages' directly
		ArchiveTreeNode* dir = res_archive->getDir("config/languages");

		if (dir)
		{
			// Read all entries in this dir
			for (unsigned a = 0; a < dir->numEntries(); a++)
				readLanguageDefinition(dir->entryAt(a)->getMCData(), dir->entryAt(a)->getName());
		}
		else
			LOG_MESSAGE(1, "Warning: 'config/languages' not found in slade.pk3, no builtin text language definitions loaded");
	}

	return true;
}

/* TextLanguage::getLanguage
 * Returns the language definition matching [id], or NULL if no
 * match found
 *******************************************************************/
TextLanguage* TextLanguage::fromId(string id)
{
	// Find text language matching [id]
	for (unsigned a = 0; a < text_languages.size(); a++)
	{
		if (text_languages[a]->id_ == id)
			return text_languages[a];
	}

	// Not found
	return NULL;
}

/* TextLanguage::getLanguage
 * Returns the language definition at [index], or NULL if [index] is
 * out of bounds
 *******************************************************************/
TextLanguage* TextLanguage::fromIndex(unsigned index)
{
	// Check index
	if (index >= text_languages.size())
		return NULL;

	return text_languages[index];
}

/* TextLanguage::getLanguageByName
 * Returns the language definition matching [name], or NULL if no
 * match found
 *******************************************************************/
TextLanguage* TextLanguage::fromName(string name)
{
	// Find text language matching [name]
	for (unsigned a = 0; a < text_languages.size(); a++)
	{
		if (S_CMPNOCASE(text_languages[a]->name_, name))
			return text_languages[a];
	}

	// Not found
	return NULL;
}

/* TextLanguage::getLanguageNames
 * Returns a list of all language names
 *******************************************************************/
wxArrayString TextLanguage::languageNames()
{
	wxArrayString ret;

	for (unsigned a = 0; a < text_languages.size(); a++)
		ret.push_back(text_languages[a]->name_);

	return ret;
}
