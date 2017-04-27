
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
	name{ name },
	return_type{ return_type }
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
string TLFunction::getArgSet(unsigned index)
{
	// Check index
	if (index >= arg_sets.size())
		return "";

	return arg_sets[index];
}

/* TLFunction::generateCallTipString
 * Returns a string representation of arg set [arg_set] that can be
 * used directly in a scintilla calltip
 *******************************************************************/
string TLFunction::generateCallTipString(int arg_set)
{
	// Check requested arg set exists
	if (arg_set < 0 || (unsigned)arg_set >= arg_sets.size())
		return "<invalid argset index>";

	string calltip;

	// Add extra buttons for selection if there is more than one arg set
	if (arg_sets.size() > 1)
		calltip += S_FMT("\001 %d of %lu \002 ", arg_set+1, arg_sets.size());

	// Generate scintilla-format calltip string
	calltip += name + "(";
	calltip += arg_sets[arg_set];
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
	if (arg_set < 0 || (unsigned)arg_set >= arg_sets.size())
		return extent;

	// Get start position of args list
	int start_pos = name.Length() + 1;
	if (arg_sets.size() > 1)
	{
		string temp = S_FMT("\001 %d of %lu \002 ", arg_set+1, arg_sets.size());
		start_pos += temp.Length();
	}

	// Check arg
	string args = arg_sets[arg_set];
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
	line_comment{ "//" },
	comment_begin{ "/*" },
	comment_end{ "*/" },
	preprocessor{ "#" },
	block_begin{ "{" },
	block_end{ "}" }
{
	// Init variables
	this->id = id;

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
	copy->line_comment = line_comment;
	copy->comment_begin = comment_begin;
	copy->comment_end = comment_end;
	copy->preprocessor = preprocessor;
	copy->case_sensitive = case_sensitive;
	copy->f_lookup_url = f_lookup_url;
	copy->doc_comment = doc_comment;
	copy->block_begin = block_begin;
	copy->block_end = block_end;

	// Copy word lists
	for (unsigned a = 0; a < 4; a++)
		copy->word_lists[a] = word_lists[a];

	// Copy functions
	size_t functions_size = functions.size();
	for (unsigned a = 0; a < functions_size; a++)
	{
		TLFunction* f = functions[a];
		size_t nargsets = f->nArgSets();
		for (unsigned b = 0; b < nargsets; b++)
			copy->addFunction(
				f->getName(),
				f->getArgSet(b),
				f->getDescription(),
				false,
				f->getReturnType()
			);
	}

	// Copy preprocessor block begin/end
	copy->pp_block_begin.clear();
	copy->pp_block_end.clear();
	size_t pp_block_begin_size = pp_block_begin.size();

	for (unsigned a = 0; a < pp_block_begin_size; a++)
		copy->pp_block_begin.push_back(pp_block_begin[a]);

	size_t pp_block_end_size = pp_block_end.size();

	for (unsigned a = 0; a < pp_block_end_size; a++)
		copy->pp_block_end.push_back(pp_block_end[a]);
}

/* TextLanguage::addWord
 * Adds a new word of [type] to the language, if it doesn't exist
 * already
 *******************************************************************/
void TextLanguage::addWord(WordType type, string keyword)
{
	// Add only if it doesn't already exist
	vector<string>& list = word_lists[type].list;
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
	// Check if the function exists
	TLFunction* func = getFunction(name);

	// If it doesn't, create it
	if (!func)
	{
		func = new TLFunction(name, return_type.empty() ? "void" : return_type);
		functions.push_back(func);
	}

	// Remove/recreate the function if we're replacing it
	else if (replace)
	{
		VECTOR_REMOVE(functions, func);
		delete func;
		func = new TLFunction(name, return_type.empty() ? "void" : return_type);
		functions.push_back(func);
	}

	// Add the arg set
	func->addArgSet(args);

	// Set description
	func->setDescription(desc);
}

/* TextLanguage::getWordList
 * Returns a string of all words of [type] in the language, separated
 * by spaces, which can be sent directly to scintilla for syntax
 * hilighting
 *******************************************************************/
string TextLanguage::getWordList(WordType type)
{
	// Init return string
	string ret = "";

	// Add each word to return string (separated by spaces)
	vector<string>& list = word_lists[type].list;
	for (size_t a = 0; a < list.size(); a++)
		ret += list[a] + " ";

	return ret;
}

/* TextLanguage::getFunctionsList
 * Returns a string of all functions in the language, separated by
 * spaces, which can be sent directly to scintilla for syntax
 * hilighting
 *******************************************************************/
string TextLanguage::getFunctionsList()
{
	// Init return string
	string ret = "";

	// Add each function name to return string (separated by spaces)
	for (unsigned a = 0; a < functions.size(); a++)
		ret += functions[a]->getName() + " ";

	return ret;
}

/* TextLanguage::getAutocompletionList
 * Returns a string containing all words and functions that can be
 * used directly in scintilla for an autocompletion list
 *******************************************************************/
string TextLanguage::getAutocompletionList(string start)
{
	// Firstly, add all functions and word lists to a wxArrayString
	wxArrayString list;
	start = start.Lower();

	// Add word lists
	for (unsigned type = 0; type < 4; type++)
		for (unsigned a = 0; a < word_lists[type].list.size(); a++)
			if (word_lists[type].list[a].Lower().StartsWith(start))
				list.Add(word_lists[type].list[a] + S_FMT("?%d", type + 1));

	// Add functions
	for (unsigned a = 0; a < functions.size(); a++)
	{
		if (functions[a]->getName().Lower().StartsWith(start))
			list.Add(functions[a]->getName() + "?3");
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
wxArrayString TextLanguage::getWordListSorted(WordType type)
{
	// Get list
	wxArrayString list;
	for (unsigned a = 0; a < word_lists[type].list.size(); a++)
		list.Add(word_lists[type].list[a]);

	// Sort
	list.Sort();

	return list;
}

/* TextLanguage::getFunctionsSorted
 * Returns a sorted wxArrayString of all functions in the language
 *******************************************************************/
wxArrayString TextLanguage::getFunctionsSorted()
{
	// Get list
	wxArrayString list;
	for (unsigned a = 0; a < functions.size(); a++)
		list.Add(functions[a]->getName());

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
	vector<string>& list = word_lists[type].list;
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
	for (unsigned a = 0; a < functions.size(); a++)
	{
		if (functions[a]->getName() == word)
			return true;
	}

	return false;
}

/* TextLanguage::getFunction
 * Returns the function definition matching [name], or NULL if no
 * matching function exists
 *******************************************************************/
TLFunction* TextLanguage::getFunction(string name)
{
	// Find function matching [name]
	size_t functions_size = functions.size();
	if (case_sensitive)
	{
		for (unsigned a = 0; a < functions_size; a++)
		{
			if (functions[a]->getName() == name)
				return functions[a];
		}
	}
	else
	{
		for (unsigned a = 0; a < functions_size; a++)
		{
			if (S_CMPNOCASE(functions[a]->getName(), name))
				return functions[a];
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
		ParseTreeNode* node = (ParseTreeNode*)root.getChild(a);

		// Create language
		TextLanguage* lang = new TextLanguage(node->getName());

		// Check for inheritance
		if (!node->getInherit().IsEmpty())
		{
			TextLanguage* inherit = getLanguage(node->getInherit());
			if (inherit)
				inherit->copyTo(lang);
			else
				LOG_MESSAGE(1, "Warning: Language %s inherits from undefined language %s", node->getName(), node->getInherit());
		}

		// Parse language info
		for (unsigned c = 0; c < node->nChildren(); c++)
		{
			ParseTreeNode* child = (ParseTreeNode*)node->getChild(c);

			// Language name
			if (S_CMPNOCASE(child->getName(), "name"))
				lang->setName(child->getStringValue());

			// Comment begin
			else if (S_CMPNOCASE(child->getName(), "comment_begin"))
				lang->setCommentBegin(child->getStringValue());

			// Comment end
			else if (S_CMPNOCASE(child->getName(), "comment_end"))
				lang->setCommentEnd(child->getStringValue());

			// Line comment
			else if (S_CMPNOCASE(child->getName(), "comment_line"))
				lang->setLineComment(child->getStringValue());

			// Preprocessor
			else if (S_CMPNOCASE(child->getName(), "preprocessor"))
				lang->setPreprocessor(child->getStringValue());

			// Case sensitive
			else if (S_CMPNOCASE(child->getName(), "case_sensitive"))
				lang->setCaseSensitive(child->getBoolValue());

			// Doc comment
			else if (S_CMPNOCASE(child->getName(), "comment_doc"))
				lang->setDocComment(child->getStringValue());

			// Keyword lookup link
			else if (S_CMPNOCASE(child->getName(), "keyword_link"))
				lang->word_lists[WordType::Keyword].lookup_url = child->getStringValue();

			// Constant lookup link
			else if (S_CMPNOCASE(child->getName(), "constant_link"))
				lang->word_lists[WordType::Constant].lookup_url = child->getStringValue();

			// Function lookup link
			else if (S_CMPNOCASE(child->getName(), "function_link"))
				lang->f_lookup_url = child->getStringValue();

			// Jump blocks
			else if (S_CMPNOCASE(child->getName(), "blocks"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->jump_blocks.push_back(child->getStringValue(v));
			}
			else if (S_CMPNOCASE(child->getName(), "blocks_ignore"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->jb_ignore.push_back(child->getStringValue(v));
			}

			// Block begin
			else if (S_CMPNOCASE(child->getName(), "block_begin"))
				lang->block_begin = child->getStringValue();

			// Block end
			else if (S_CMPNOCASE(child->getName(), "block_end"))
				lang->block_end = child->getStringValue();

			// Preprocessor block begin
			else if (S_CMPNOCASE(child->getName(), "pp_block_begin"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->pp_block_begin.push_back(child->getStringValue(v));
			}

			// Preprocessor block end
			else if (S_CMPNOCASE(child->getName(), "pp_block_end"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->pp_block_end.push_back(child->getStringValue(v));
			}

			// Keywords
			else if (S_CMPNOCASE(child->getName(), "keywords"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					string val = child->getStringValue(v);

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
					string val = child->getStringValue(v);

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
					string val = child->getStringValue(v);

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
					string val = child->getStringValue(v);

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
					ParseTreeNode* child_func = (ParseTreeNode*)child->getChild(f);

					// Simple definition
					if (child_func->nChildren() == 0)
					{
						// Add function
						lang->addFunction(
							child_func->getName(),
							child_func->getStringValue(0),
							"",
							true,
							child_func->getType());

						// Add args
						for (unsigned v = 1; v < child_func->nValues(); v++)
							lang->addFunction(child_func->getName(), child_func->getStringValue(v));
					}

					// Full definition
					else
					{
						string name = child_func->getName();
						string desc = "";
						vector<string> args;
						for (unsigned p = 0; p < child_func->nChildren(); p++)
						{
							ParseTreeNode* child_prop = (ParseTreeNode*)child_func->getChild(p);
							if (child_prop->getName() == "args")
							{
								for (unsigned v = 0; v < child_prop->nValues(); v++)
									args.push_back(child_prop->getStringValue(v));
							}
							else if (child_prop->getName() == "description")
								desc = child_prop->getStringValue();
						}

						for (unsigned as = 0; as < args.size(); as++)
							lang->addFunction(name, args[as], desc, as == 0, child_func->getType());
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
	Archive* res_archive = theArchiveManager->programResourceArchive();

	// Read language definitions from resource archive
	if (res_archive)
	{
		// Get 'config/languages' directlry
		ArchiveTreeNode* dir = res_archive->getDir("config/languages");

		if (dir)
		{
			// Read all entries in this dir
			for (unsigned a = 0; a < dir->numEntries(); a++)
				readLanguageDefinition(dir->getEntry(a)->getMCData(), dir->getEntry(a)->getName());
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
TextLanguage* TextLanguage::getLanguage(string id)
{
	// Find text language matching [id]
	for (unsigned a = 0; a < text_languages.size(); a++)
	{
		if (text_languages[a]->id == id)
			return text_languages[a];
	}

	// Not found
	return NULL;
}

/* TextLanguage::getLanguage
 * Returns the language definition at [index], or NULL if [index] is
 * out of bounds
 *******************************************************************/
TextLanguage* TextLanguage::getLanguage(unsigned index)
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
TextLanguage* TextLanguage::getLanguageByName(string name)
{
	// Find text language matching [name]
	for (unsigned a = 0; a < text_languages.size(); a++)
	{
		if (S_CMPNOCASE(text_languages[a]->name, name))
			return text_languages[a];
	}

	// Not found
	return NULL;
}

/* TextLanguage::getLanguageNames
 * Returns a list of all language names
 *******************************************************************/
wxArrayString TextLanguage::getLanguageNames()
{
	wxArrayString ret;

	for (unsigned a = 0; a < text_languages.size(); a++)
		ret.push_back(text_languages[a]->name);

	return ret;
}
