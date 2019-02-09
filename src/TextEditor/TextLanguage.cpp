
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextLanguage.cpp
// Description: Defines a 'language' for use by the TextEditor for syntax
//              hilighting/autocompletion/etc. Contains lists of keywords,
//              constants and functions, with various utility functions for
//              using them.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "TextLanguage.h"
#include "Archive/ArchiveManager.h"
#include "Game/ZScript.h"
#include "Utility/Parser.h"
#include "Utility/Tokenizer.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
vector<TextLanguage*> text_languages;


// -----------------------------------------------------------------------------
//
// TLFunction Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the arg set [index], or an empty string if [index] is out of bounds
// -----------------------------------------------------------------------------
TLFunction::Context TLFunction::context(unsigned long index) const
{
	// Check index
	if (index >= contexts_.size())
		return Context();

	return contexts_[index];
}

// -----------------------------------------------------------------------------
// Parses a function parameter from a list of tokens
// -----------------------------------------------------------------------------
void TLFunction::Parameter::parse(vector<wxString>& tokens)
{
	// Optional
	if (tokens[0] == "[")
	{
		optional = true;
		tokens.erase(tokens.begin());
		tokens.pop_back();
	}

	if (tokens.empty())
		return;

	// (Type) and name
	if (tokens.size() > 1)
	{
		type = tokens[0];
		name = tokens[1];
	}
	else
	{
		name = tokens[0];
	}
}

// -----------------------------------------------------------------------------
// Adds a [context] of the function
// -----------------------------------------------------------------------------
void TLFunction::addContext(
	const wxString& context,
	const wxString& args,
	const wxString& return_type,
	const wxString& description,
	const wxString& deprecated_f)
{
	contexts_.push_back(Context{ context, {}, return_type, description, "", "", "", false });
	auto& ctx = contexts_.back();

	// Parse args
	Tokenizer tz;
	tz.setSpecialCharacters("[],");
	tz.openString(args.ToStdString());

	vector<wxString> arg_tokens;
	while (true)
	{
		while (!tz.check(","))
		{
			arg_tokens.push_back(tz.current().text);
			if (tz.atEnd())
				break;
			tz.adv();
		}

		ctx.params.push_back({});
		ctx.params.back().parse(arg_tokens);
		arg_tokens.clear();

		if (tz.atEnd())
			break;

		tz.adv();
	}

	if (!deprecated_f.empty())
	{
		// Parse deprecated string
		tz.openString(deprecated_f.ToStdString());

		for (unsigned t = 0; t < 2; t++)
		{
			while (tz.check(","))
				tz.adv();

			bool is_replacement = true;
			for (auto&& c : tz.current().text)
			{
				char chr = c;
				if (isdigit(chr) || chr == '.')
				{
					is_replacement = false;
					break;
				}
			}

			if (is_replacement)
				ctx.deprecated_f = tz.current().text;
			else
				ctx.deprecated_v = tz.current().text;

			if (tz.atEnd())
				break;
			tz.adv();
		}
	}
}

// -----------------------------------------------------------------------------
// Adds a [context] of the function from a parsed ZScript function [func]
// -----------------------------------------------------------------------------
void TLFunction::addContext(
	const wxString&          context,
	const ZScript::Function& func,
	bool                     custom,
	const wxString&          desc,
	const wxString&          dep_f)
{
	contexts_.push_back(Context{ context, {}, func.returnType(), desc, "", func.deprecated(), dep_f, custom });
	auto& ctx = contexts_.back();

	if (func.isVirtual())
		ctx.qualifiers += "virtual ";
	if (func.native())
		ctx.qualifiers += "native ";

	if (func.parameters().empty())
		ctx.params.push_back({ "void", "", "", false });
	else
		for (auto& p : func.parameters())
			ctx.params.push_back({ p.type, p.name, p.default_value, !p.default_value.empty() });
}

// -----------------------------------------------------------------------------
// Clears any custom contextx for the function
// -----------------------------------------------------------------------------
void TLFunction::clearCustomContexts()
{
	for (auto i = contexts_.begin(); i != contexts_.end();)
		if (i->custom)
			i = contexts_.erase(i);
		else
			++i;
}

// -----------------------------------------------------------------------------
// Returns true if the function has a context matching [name]
// -----------------------------------------------------------------------------
bool TLFunction::hasContext(const wxString& name)
{
	for (auto& c : contexts_)
		if (S_CMPNOCASE(c.context, name))
			return true;

	return false;
}


// -----------------------------------------------------------------------------
//
// TextLanguage Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextLanguage class constructor
// -----------------------------------------------------------------------------
TextLanguage::TextLanguage(const wxString& id) : id_{ id }
{
	// Add to languages list
	text_languages.push_back(this);
}

// -----------------------------------------------------------------------------
// TextLanguage class destructor
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Copies all language info to [copy]
// -----------------------------------------------------------------------------
void TextLanguage::copyTo(TextLanguage* copy)
{
	// Copy general attributes
	copy->preferred_comments_ = preferred_comments_;
	copy->line_comment_l_     = line_comment_l_;
	copy->comment_begin_l_    = comment_begin_l_;
	copy->comment_end_l_      = comment_end_l_;
	copy->preprocessor_       = preprocessor_;
	copy->case_sensitive_     = case_sensitive_;
	copy->f_lookup_url_       = f_lookup_url_;
	copy->doc_comment_        = doc_comment_;
	copy->block_begin_        = block_begin_;
	copy->block_end_          = block_end_;

	// Copy word lists
	for (unsigned a = 0; a < 4; a++)
		copy->word_lists_[a] = word_lists_[a];

	// Copy functions
	copy->functions_ = functions_;

	// Copy preprocessor/word block begin/end
	copy->pp_block_begin_   = pp_block_begin_;
	copy->pp_block_end_     = pp_block_end_;
	copy->word_block_begin_ = word_block_begin_;
	copy->word_block_end_   = word_block_end_;
}

// -----------------------------------------------------------------------------
// Adds a new word of [type] to the language, if it doesn't exist already
// -----------------------------------------------------------------------------
void TextLanguage::addWord(WordType type, const wxString& keyword, bool custom)
{
	// Add only if it doesn't already exist
	vector<wxString>& list = custom ? word_lists_custom_[type].list : word_lists_[type].list;
	if (std::find(list.begin(), list.end(), keyword) == list.end())
		list.push_back(keyword);
}

// -----------------------------------------------------------------------------
// Adds a function arg set to the language. If the function [name] exists,
// [args] will be added to it as a new arg set, otherwise a new function will
// be added
// -----------------------------------------------------------------------------
void TextLanguage::addFunction(
	wxString        name,
	const wxString& args,
	const wxString& desc,
	const wxString& deprecated,
	bool            replace,
	const wxString& return_type)
{
	// Split out context from name
	wxString context;
	if (name.Contains("."))
	{
		wxString fname;
		context = name.BeforeFirst('.', &fname);
		name    = fname;
	}

	// Check if the function exists
	auto func = function(name);

	// If it doesn't, create it
	if (!func)
	{
		functions_.emplace_back(name);
		func = &functions_.back();
	}
	// Clear the function if we're replacing it
	else if (replace)
	{
		if (!context.empty())
		{
			func->clear();
		}
		else
		{
			func->clearContexts();
		}
	}

	// Add the context
	func->addContext(context, args, return_type, desc, deprecated);
}

// -----------------------------------------------------------------------------
// Loads types (classes) and functions from parsed ZScript definitions [defs]
// -----------------------------------------------------------------------------
void TextLanguage::loadZScript(ZScript::Definitions& defs, bool custom)
{
	// Classes
	for (auto& c : defs.classes())
	{
		// Add class as type
		addWord(Type, c.name(), custom);

		// Add class functions
		for (auto& f : c.functions())
		{
			// Ignore overriding functions
			if (f.isOverride())
				continue;

			// Check if the function exists
			auto func = function(f.name());

			// If it doesn't, create it
			if (!func)
			{
				functions_.emplace_back(f.name());
				func = &functions_.back();
			}

			// Add the context
			if (!func->hasContext(c.name()))
				func->addContext(
					c.name(),
					f,
					custom,
					zfuncs_ex_props_[func->name()].description,
					zfuncs_ex_props_[func->name()].deprecated_f);
		}
	}
}

// -----------------------------------------------------------------------------
// Returns a string of all words of [type] in the language, separated by
// spaces, which can be sent directly to scintilla for syntax hilighting
// -----------------------------------------------------------------------------
wxString TextLanguage::wordList(WordType type, bool include_custom)
{
	// Init return string
	wxString ret = "";

	// Add each word to return string (separated by spaces)
	for (auto& word : word_lists_[type].list)
		ret += word + " ";

	// Include custom words if specified
	if (include_custom)
		for (auto& word : word_lists_custom_[type].list)
			ret += word + " ";

	return ret;
}

// -----------------------------------------------------------------------------
// Returns a string of all functions in the language, separated by spaces,
// which can be sent directly to scintilla for syntax hilighting
// -----------------------------------------------------------------------------
wxString TextLanguage::functionsList()
{
	// Init return string
	wxString ret = "";

	// Add each function name to return string (separated by spaces)
	for (auto& func : functions_)
		ret += func.name() + " ";

	return ret;
}

// -----------------------------------------------------------------------------
// Returns a string containing all words and functions that can be used
// directly in scintilla for an autocompletion list
// -----------------------------------------------------------------------------
wxString TextLanguage::autocompletionList(wxString start, bool include_custom)
{
	// Firstly, add all functions and word lists to a wxArrayString
	wxArrayString list;
	start = start.Lower();

	// Add word lists
	for (unsigned type = 0; type < 4; type++)
	{
		for (auto& word : word_lists_[type].list)
			if (word.Lower().StartsWith(start))
				list.Add(wxString::Format("%s?%d", word, type + 1));

		if (!include_custom)
			continue;

		for (auto& word : word_lists_custom_[type].list)
			if (word.Lower().StartsWith(start))
				list.Add(wxString::Format("%s?%d", word, type + 1));
	}

	// Add functions
	for (auto& func : functions_)
		if (func.name().Lower().StartsWith(start))
			list.Add(wxString::Format("%s%s", func.name(), "?3"));

	// Sort the list
	list.Sort();

	// Now build a string of the list items separated by spaces
	wxString ret;
	for (const auto& a : list)
		ret += a + " ";

	return ret;
}

// -----------------------------------------------------------------------------
// Returns a sorted wxArrayString of all words of [type] in the language
// -----------------------------------------------------------------------------
wxArrayString TextLanguage::wordListSorted(WordType type, bool include_custom)
{
	// Get list
	wxArrayString list;
	for (auto& word : word_lists_[type].list)
		list.Add(word);

	if (include_custom)
		for (auto& word : word_lists_custom_[type].list)
			list.Add(word);

	// Sort
	list.Sort();

	return list;
}

// -----------------------------------------------------------------------------
// Returns a sorted wxArrayString of all functions in the language
// -----------------------------------------------------------------------------
wxArrayString TextLanguage::functionsSorted()
{
	// Get list
	wxArrayString list;
	for (auto& func : functions_)
		list.Add(func.name());

	// Sort
	list.Sort();

	return list;
}

// -----------------------------------------------------------------------------
// Returns true if [word] is a [type] word in this language, false otherwise
// -----------------------------------------------------------------------------
bool TextLanguage::isWord(WordType type, const wxString& word)
{
	for (auto& w : word_lists_[type].list)
		if (w == word)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns true if [word] is a function in this language, false otherwise
// -----------------------------------------------------------------------------
bool TextLanguage::isFunction(const wxString& word)
{
	for (auto& func : functions_)
		if (func.name() == word)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns the function definition matching [name], or NULL if no matching
// function exists
// -----------------------------------------------------------------------------
TLFunction* TextLanguage::function(const wxString& name)
{
	// Find function matching [name]
	if (case_sensitive_)
	{
		for (auto& func : functions_)
			if (func.name() == name)
				return &func;
	}
	else
	{
		for (auto& func : functions_)
			if (S_CMPNOCASE(func.name(), name))
				return &func;
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Clears all custom definitions in the language
// -----------------------------------------------------------------------------
void TextLanguage::clearCustomDefs()
{
	for (auto i = functions_.begin(); i != functions_.end();)
	{
		i->clearCustomContexts();

		// Remove function if only contexts were custom
		if (i->contexts().empty())
			i = functions_.erase(i);
		else
			++i;
	}

	for (auto& a : word_lists_custom_)
		a.list.clear();
}


// -----------------------------------------------------------------------------
//
// TextLanguage Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads in a text definition of a language. See slade.pk3 for
// formatting examples
// -----------------------------------------------------------------------------
bool TextLanguage::readLanguageDefinition(MemChunk& mc, const wxString& source)
{
	Tokenizer tz;

	// Open the given text data
	if (!tz.openMem(mc, source.ToStdString()))
	{
		Log::warning(1, wxString::Format("Warning: Unable to open %s", source));
		return false;
	}

	// Parse the definition text
	ParseTreeNode root;
	if (!root.parse(tz))
		return false;

	// Get parsed data
	for (unsigned a = 0; a < root.nChildren(); a++)
	{
		auto node = root.childPTN(a);

		// Create language
		auto lang = new TextLanguage(node->name());

		// Check for inheritance
		if (!node->inherit().IsEmpty())
		{
			auto inherit = fromId(node->inherit());
			if (inherit)
				inherit->copyTo(lang);
			else
				Log::warning(
					1,
					wxString::Format(
						"Warning: Language %s inherits from undefined language %s", node->name(), node->inherit()));
		}

		// Parse language info
		for (unsigned c = 0; c < node->nChildren(); c++)
		{
			auto child = node->childPTN(c);

			// Language name
			if (S_CMPNOCASE(child->name(), "name"))
				lang->setName(child->stringValue());

			// Comment begin
			else if (S_CMPNOCASE(child->name(), "comment_begin"))
			{
				lang->setCommentBeginList(child->stringValues());
			}

			// Comment end
			else if (S_CMPNOCASE(child->name(), "comment_end"))
			{
				lang->setCommentEndList(child->stringValues());
			}

			// Line comment
			else if (S_CMPNOCASE(child->name(), "comment_line"))
			{
				lang->setLineCommentList(child->stringValues());
			}

			// Preprocessor
			else if (S_CMPNOCASE(child->name(), "preprocessor"))
				lang->setPreprocessor(child->stringValue());

			// Case sensitive
			else if (S_CMPNOCASE(child->name(), "case_sensitive"))
				lang->setCaseSensitive(child->boolValue());

			// Doc comment
			else if (S_CMPNOCASE(child->name(), "comment_doc"))
				lang->setDocComment(child->stringValue());

			// Keyword lookup link
			else if (S_CMPNOCASE(child->name(), "keyword_link"))
				lang->word_lists_[WordType::Keyword].lookup_url = child->stringValue();

			// Constant lookup link
			else if (S_CMPNOCASE(child->name(), "constant_link"))
				lang->word_lists_[WordType::Constant].lookup_url = child->stringValue();

			// Function lookup link
			else if (S_CMPNOCASE(child->name(), "function_link"))
				lang->f_lookup_url_ = child->stringValue();

			// Jump blocks
			else if (S_CMPNOCASE(child->name(), "blocks"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->jump_blocks_.push_back(child->stringValue(v));
			}
			else if (S_CMPNOCASE(child->name(), "blocks_ignore"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->jb_ignore_.push_back(child->stringValue(v));
			}

			// Block begin
			else if (S_CMPNOCASE(child->name(), "block_begin"))
				lang->block_begin_ = child->stringValue();

			// Block end
			else if (S_CMPNOCASE(child->name(), "block_end"))
				lang->block_end_ = child->stringValue();

			// Preprocessor block begin
			else if (S_CMPNOCASE(child->name(), "pp_block_begin"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->pp_block_begin_.push_back(child->stringValue(v));
			}

			// Preprocessor block end
			else if (S_CMPNOCASE(child->name(), "pp_block_end"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->pp_block_end_.push_back(child->stringValue(v));
			}

			// Word block begin
			else if (S_CMPNOCASE(child->name(), "word_block_begin"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->word_block_begin_.push_back(child->stringValue(v));
			}

			// Word block end
			else if (S_CMPNOCASE(child->name(), "word_block_end"))
			{
				for (unsigned v = 0; v < child->nValues(); v++)
					lang->word_block_end_.push_back(child->stringValue(v));
			}

			// Keywords
			else if (S_CMPNOCASE(child->name(), "keywords"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					wxString val = child->stringValue(v);

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
			else if (S_CMPNOCASE(child->name(), "constants"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					wxString val = child->stringValue(v);

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
			else if (S_CMPNOCASE(child->name(), "types"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					wxString val = child->stringValue(v);

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
			else if (S_CMPNOCASE(child->name(), "properties"))
			{
				// Go through values
				for (unsigned v = 0; v < child->nValues(); v++)
				{
					wxString val = child->stringValue(v);

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
			else if (S_CMPNOCASE(child->name(), "functions"))
			{
				bool lang_has_void = lang->isWord(Keyword, "void") || lang->isWord(Type, "void");
				if (lang->id_ != "zscript")
				{
					// Go through children (functions)
					for (unsigned f = 0; f < child->nChildren(); f++)
					{
						auto     child_func = child->childPTN(f);
						wxString params;

						// Simple definition
						if (child_func->nChildren() == 0)
						{
							if (child_func->stringValue(0).empty())
							{
								if (lang_has_void)
									params = "void";
								else
									params = "";
							}
							else
							{
								params = child_func->stringValue(0);
							}

							// Add function
							lang->addFunction(
								child_func->name(),
								params,
								"",
								"",
								!child_func->name().Contains("."),
								child_func->type());

							// Add args
							for (unsigned v = 1; v < child_func->nValues(); v++)
								lang->addFunction(child_func->name(), child_func->stringValue(v));
						}

						// Full definition
						else
						{
							wxString         name = child_func->name();
							vector<wxString> args;
							wxString         desc       = "";
							wxString         deprecated = "";
							for (unsigned p = 0; p < child_func->nChildren(); p++)
							{
								auto child_prop = child_func->childPTN(p);
								if (child_prop->name() == "args")
								{
									for (unsigned v = 0; v < child_prop->nValues(); v++)
										args.push_back(child_prop->stringValue(v));
								}
								else if (child_prop->name() == "description")
									desc = child_prop->stringValue();
								else if (child_prop->name() == "deprecated")
									deprecated = child_prop->stringValue();
							}

							if (args.empty() && lang_has_void)
								args.emplace_back("void");

							for (unsigned as = 0; as < args.size(); as++)
								lang->addFunction(name, args[as], desc, deprecated, as == 0, child_func->type());
						}
					}
				}
				// ZScript function info which cannot be parsed from (g)zdoom.pk3
				else
				{
					ZFuncExProp ex_prop;
					for (unsigned f = 0; f < child->nChildren(); f++)
					{
						auto child_func = child->childPTN(f);
						for (unsigned p = 0; p < child_func->nChildren(); ++p)
						{
							auto child_prop = child_func->childPTN(p);
							if (child_prop->name() == "description")
								ex_prop.description = child_prop->stringValue();
							else if (child_prop->name() == "deprecated_f")
								ex_prop.deprecated_f = child_prop->stringValue();
						}
						lang->zfuncs_ex_props_.emplace(child_func->name(), ex_prop);
					}
				}
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads all text language definitions from slade.pk3
// -----------------------------------------------------------------------------
bool TextLanguage::loadLanguages()
{
	// Get slade resource archive
	auto res_archive = App::archiveManager().programResourceArchive();

	// Read language definitions from resource archive
	if (res_archive)
	{
		// Get 'config/languages' directly
		auto dir = res_archive->dir("config/languages");

		if (dir)
		{
			// Read all entries in this dir
			for (unsigned a = 0; a < dir->numEntries(); a++)
				readLanguageDefinition(dir->entryAt(a)->data(), dir->entryAt(a)->name());
		}
		else
			Log::warning(
				1, "Warning: 'config/languages' not found in slade.pk3, no builtin text language definitions loaded");
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the language definition matching [id], or NULL if no match found
// -----------------------------------------------------------------------------
TextLanguage* TextLanguage::fromId(const wxString& id)
{
	// Find text language matching [id]
	for (auto& text_language : text_languages)
	{
		if (text_language->id_ == id)
			return text_language;
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the language definition at [index], or NULL if [index] is out of
// bounds
// -----------------------------------------------------------------------------
TextLanguage* TextLanguage::fromIndex(unsigned index)
{
	// Check index
	if (index >= text_languages.size())
		return nullptr;

	return text_languages[index];
}

// -----------------------------------------------------------------------------
// Returns the language definition matching [name], or NULL if no match found
// -----------------------------------------------------------------------------
TextLanguage* TextLanguage::fromName(const wxString& name)
{
	// Find text language matching [name]
	for (auto& text_language : text_languages)
	{
		if (S_CMPNOCASE(text_language->name_, name))
			return text_language;
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a list of all language names
// -----------------------------------------------------------------------------
wxArrayString TextLanguage::languageNames()
{
	wxArrayString ret;

	for (auto& text_language : text_languages)
		ret.push_back(text_language->name_);

	return ret;
}
