
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
TLFunction::TLFunction(string name)
{
	this->name = name;
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
TextLanguage::TextLanguage(string id)
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
	for (size_t a = 0; a < text_languages.size(); a++)
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
	copy->k_lookup_url = k_lookup_url;
	copy->c_lookup_url = c_lookup_url;
	copy->f_lookup_url = f_lookup_url;

	// Copy keywords
	for (unsigned a = 0; a < keywords.size(); a++)
		copy->keywords.push_back(keywords[a]);

	// Copy constants
	for (unsigned a = 0; a < constants.size(); a++)
		copy->constants.push_back(constants[a]);

	// Copy functions
	for (unsigned a = 0; a < functions.size(); a++)
	{
		TLFunction* f = functions[a];
		for (unsigned b = 0; b < f->nArgSets(); b++)
			copy->addFunction(f->getName(), f->getArgSet(b));
	}
}

/* TextLanguage::addKeyword
 * Adds a new keyword to the language, if it doesn't exist already
 *******************************************************************/
void TextLanguage::addKeyword(string keyword)
{
	// Add only if it doesn't already exist
	if (std::find(keywords.begin(), keywords.end(), keyword) == keywords.end())
		keywords.push_back(keyword);
}

/* TextLanguage::addConstant
 * Adds a new constant to the language, if it doesn't exist already
 *******************************************************************/
void TextLanguage::addConstant(string constant)
{
	// Add only if it doesn't already exist
	if (std::find(constants.begin(), constants.end(), constant) == constants.end())
		constants.push_back(constant);
}

/* TextLanguage::addFunction
 * Adds a function arg set to the language. If the function [name]
 * exists, [args] will be added to it as a new arg set, otherwise
 * a new function will be added
 *******************************************************************/
void TextLanguage::addFunction(string name, string args)
{
	// Check if the function exists
	TLFunction* func = getFunction(name);

	// If it doesn't, create it
	if (!func)
	{
		func = new TLFunction(name);
		functions.push_back(func);
	}

	// Add the arg set
	func->addArgSet(args);
}

/* TextLanguage::getKeywordsList
 * Returns a string of all keywords in the language, separated by
 * spaces, which can be sent directly to scintilla for syntax
 * hilighting
 *******************************************************************/
string TextLanguage::getKeywordsList()
{
	// Init return string
	string ret = "";

	// Add each keyword to return string (separated by spaces)
	for (size_t a = 0; a < keywords.size(); a++)
		ret += keywords[a] + " ";

	return ret;
}

/* TextLanguage::getConstantsList
 * Returns a string of all constants in the language, separated by
 * spaces, which can be sent directly to scintilla for syntax
 * hilighting
 *******************************************************************/
string TextLanguage::getConstantsList()
{
	// Init return string
	string ret = "";

	// Add each constant to return string (separated by spaces)
	for (size_t a = 0; a < constants.size(); a++)
		ret += constants[a] + " ";

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
 * Returns a string containing all keywords, constants and functions
 * that can be used directly in scintilla for an autocompletion list
 *******************************************************************/
string TextLanguage::getAutocompletionList(string start)
{
	// Firstly, add all functions, constants and keywords to a wxArrayString
	wxArrayString list;
	start = start.Lower();
	for (unsigned a = 0; a < keywords.size(); a++)
	{
		if (keywords[a].Lower().StartsWith(start))
			list.Add(keywords[a] + "?1");
	}
	for (unsigned a = 0; a < constants.size(); a++)
	{
		if (constants[a].Lower().StartsWith(start))
			list.Add(constants[a] + "?2");
	}
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

wxArrayString TextLanguage::getKeywordsSorted()
{
	// Get list
	wxArrayString list;
	for (unsigned a = 0; a < keywords.size(); a++)
		list.Add(keywords[a]);

	// Sort
	list.Sort();

	return list;
}

wxArrayString TextLanguage::getConstantsSorted()
{
	// Get list
	wxArrayString list;
	for (unsigned a = 0; a < constants.size(); a++)
		list.Add(constants[a]);

	// Sort
	list.Sort();

	return list;
}

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

/* TextLanguage::isKeyword
 * Returns true if [word] is a keyword in this language, false
 * otherwise
 *******************************************************************/
bool TextLanguage::isKeyword(string word)
{
	for (unsigned a = 0; a < keywords.size(); a++)
	{
		if (keywords[a] == word)
			return true;
	}

	return false;
}

/* TextLanguage::isConstant
 * Returns true if [word] is a constant in this language, false
 * otherwise
 *******************************************************************/
bool TextLanguage::isConstant(string word)
{
	for (unsigned a = 0; a < constants.size(); a++)
	{
		if (constants[a] == word)
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
	if (case_sensitive)
	{
		for (unsigned a = 0; a < functions.size(); a++)
		{
			if (functions[a]->getName() == name)
				return functions[a];
		}
	}
	else
	{
		for (unsigned a = 0; a < functions.size(); a++)
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
		wxLogMessage("Unable to open file");
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
				wxLogMessage("Warning: Language %s inherits from undefined language %s", node->getName(), node->getInherit());
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
			else if (S_CMPNOCASE(child->getName(), "line_comment"))
				lang->setLineComment(child->getStringValue());

			// Preprocessor
			else if (S_CMPNOCASE(child->getName(), "preprocessor"))
				lang->setPreprocessor(child->getStringValue());

			// Case sensitive
			else if (S_CMPNOCASE(child->getName(), "case_sensitive"))
				lang->setCaseSensitive(child->getBoolValue());

			// Keyword lookup link
			else if (S_CMPNOCASE(child->getName(), "keyword_link"))
				lang->k_lookup_url = child->getStringValue();

			// Constant lookup link
			else if (S_CMPNOCASE(child->getName(), "constant_link"))
				lang->c_lookup_url = child->getStringValue();

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
						lang->clearKeywords();
					}

					// Not a special symbol, add as keyword
					else
						lang->addKeyword(val);
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
						lang->clearConstants();
					}

					// Not a special symbol, add as constant
					else
						lang->addConstant(val);
				}
			}

			// Functions
			else if (S_CMPNOCASE(child->getName(), "functions"))
			{
				// Go through children (functions)
				for (unsigned f = 0; f < child->nChildren(); f++)
				{
					ParseTreeNode* child_func = (ParseTreeNode*)child->getChild(f);

					// Add function
					lang->addFunction(child_func->getName(), child_func->getStringValue(0));

					// Add args
					for (unsigned v = 1; v < child_func->nValues(); v++)
					{
						lang->addFunction(child_func->getName(), child_func->getStringValue(v));
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
			wxLogMessage("Warning: 'config/languages' not found in slade.pk3, no builtin text language definitions loaded");
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
