
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Game/ZScript.h"
#include "Utility/JsonUtils.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;


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
const TLFunction::Context& TLFunction::context(unsigned long index) const
{
	static Context ctx_invalid;

	// Check index
	if (index >= contexts_.size())
		return ctx_invalid;

	return contexts_[index];
}

// -----------------------------------------------------------------------------
// Parses a function parameter from a list of tokens
// -----------------------------------------------------------------------------
void TLFunction::Parameter::parse(vector<string>& tokens)
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
	string_view context,
	string_view args,
	string_view return_type,
	string_view description,
	string_view deprecated_f)
{
	contexts_.emplace_back(context, return_type, description);
	auto& ctx = contexts_.back();

	// Parse args
	Tokenizer tz;
	tz.setSpecialCharacters("[],");
	tz.openString(args);

	vector<string> arg_tokens;
	while (true)
	{
		while (!tz.check(","))
		{
			arg_tokens.push_back(tz.current().text);
			if (tz.atEnd())
				break;
			tz.adv();
		}

		ctx.params.emplace_back();
		ctx.params.back().parse(arg_tokens);
		arg_tokens.clear();

		if (tz.atEnd())
			break;

		tz.adv();
	}

	if (!deprecated_f.empty())
	{
		// Parse deprecated string
		tz.openString(deprecated_f);

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
	string_view              context,
	const zscript::Function& func,
	bool                     custom,
	string_view              desc,
	string_view              dep_f)
{
	contexts_.emplace_back(context, func.returnType(), desc, func.deprecated(), dep_f, custom);
	auto& ctx = contexts_.back();

	if (func.isVirtual())
		ctx.qualifiers += "virtual ";
	if (func.native())
		ctx.qualifiers += "native ";

	if (func.parameters().empty())
		ctx.params.push_back({ "void", "", "", false });
	else
		for (const auto& p : func.parameters())
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
bool TLFunction::hasContext(string_view name) const
{
	for (auto& c : contexts_)
		if (strutil::equalCI(c.context, name))
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
TextLanguage::TextLanguage(string_view id) : id_{ id }
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
void TextLanguage::copyTo(TextLanguage* copy) const
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
void TextLanguage::addWord(WordType type, string_view keyword, bool custom)
{
	// Add only if it doesn't already exist
	auto& list = custom ? word_lists_custom_[type].list : word_lists_[type].list;
	if (std::find(list.begin(), list.end(), keyword) == list.end())
		list.emplace_back(keyword);
}

// -----------------------------------------------------------------------------
// Adds a function arg set to the language. If the function [name] exists,
// [args] will be added to it as a new arg set, otherwise a new function will
// be added
// -----------------------------------------------------------------------------
void TextLanguage::addFunction(
	string_view name,
	string_view args,
	string_view desc,
	string_view deprecated,
	bool        replace,
	string_view return_type)
{
	// Split out context from name
	string context;
	string func_name{ name };
	if (strutil::contains(name, '.'))
	{
		context   = strutil::beforeFirst(name, '.');
		func_name = strutil::afterFirst(name, '.');
	}

	// Check if the function exists
	auto* func = function(func_name);

	// If it doesn't, create it
	if (!func)
	{
		functions_.emplace_back(func_name);
		func = &functions_.back();
	}
	// Clear the function if we're replacing it
	else if (replace)
	{
		if (!context.empty())
			func->clear();
		else
			func->clearContexts();
	}

	// Add the context
	func->addContext(context, args, return_type, desc, deprecated);
}

// -----------------------------------------------------------------------------
// Adds a function [name] to the language, parsed from a JSON object [j].
// If [has_void] is true, empty args/return_type will be set to 'void'
// -----------------------------------------------------------------------------
void TextLanguage::addFunction(string_view name, const Json& j, bool has_void)
{
	// If the function has no context, replace any existing function with the
	// same name (unless... see below)
	bool replace = !strutil::contains(name, '.');

	// If the function name contains ':', it's an additional arg set, so don't
	// replace the function and ignore anything after the colon.
	if (strutil::contains(name, ':'))
	{
		name    = strutil::beforeFirst(name, ':');
		replace = false;
	}

	string return_type, args, description, deprecated;

	// Simple definition (string)
	if (j.is_string())
	{
		args = j.get<string>();

		// Check for return type (after "->")
		if (strutil::contains(args, "->"))
		{
			return_type = strutil::trim(strutil::afterFirstV(args, "->"));
			args        = strutil::trim(strutil::beforeFirstV(args, "->"));
		}
	}

	// Full definition (object)
	else if (j.is_object())
	{
		jsonutil::getIf(j, "return_type", return_type);
		jsonutil::getIf(j, "args", args);
		jsonutil::getIf(j, "description", description);
		jsonutil::getIf(j, "deprecated_f", deprecated);
	}

	// Unexpected format
	else
	{
		log::error("Unexpected function definition format for {}", name);
		return;
	}

	if (has_void)
	{
		if (args.empty())
			args = "void";
		if (return_type.empty())
			return_type = "void";
	}

	addFunction(name, args, description, deprecated, replace, return_type);
}

// -----------------------------------------------------------------------------
// Loads types (classes) and functions from parsed ZScript definitions [defs]
// -----------------------------------------------------------------------------
void TextLanguage::loadZScript(const zscript::Definitions& defs, bool custom)
{
	// Classes
	for (const auto& c : defs.classes())
	{
		// Add class as type
		addWord(Type, c.name(), custom);

		// Add class functions
		for (const auto& f : c.functions())
		{
			// Ignore overriding functions
			if (f.isOverride())
				continue;

			// Check if the function exists
			auto* func = function(f.name());

			// If it doesn't, create it
			if (!func)
			{
				functions_.emplace_back(f.name());
				func = &functions_.back();
			}

			// Add the context
			if (!func->hasContext(f.baseClass()))
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
string TextLanguage::wordList(WordType type, bool include_custom) const
{
	// Init return string
	string ret;

	// Add each word to return string (separated by spaces)
	for (auto& word : word_lists_[type].list)
		ret.append(word).append(" ");

	// Include custom words if specified
	if (include_custom)
		for (auto& word : word_lists_custom_[type].list)
			ret.append(word).append(" ");

	return ret;
}

// -----------------------------------------------------------------------------
// Returns a string of all functions in the language, separated by spaces,
// which can be sent directly to scintilla for syntax hilighting
// -----------------------------------------------------------------------------
string TextLanguage::functionsList() const
{
	// Init return string
	string ret;

	// Add each function name to return string (separated by spaces)
	for (auto& func : functions_)
		ret.append(func.name()).append(" ");

	return ret;
}

// -----------------------------------------------------------------------------
// Returns a string containing all words and functions that can be used
// directly in scintilla for an autocompletion list
// -----------------------------------------------------------------------------
string TextLanguage::autocompletionList(string_view start, bool include_custom)
{
	// Firstly, add all functions and word lists to a vector
	vector<string> list;

	// Add word lists
	for (unsigned type = 0; type < 4; type++)
	{
		for (auto& word : word_lists_[type].list)
			if (strutil::startsWithCI(word, start))
				list.push_back(fmt::format("{}?{}", word, type + 1));

		if (!include_custom)
			continue;

		for (auto& word : word_lists_custom_[type].list)
			if (strutil::startsWithCI(word, start))
				list.push_back(fmt::format("{}?{}", word, type + 1));
	}

	// Add functions
	for (auto& func : functions_)
		if (strutil::startsWithCI(func.name(), start))
			list.push_back(fmt::format("{}{}", func.name(), "?5"));

	// Sort the list
	std::sort(list.begin(), list.end());

	// Now build a string of the list items separated by spaces
	string ret;
	for (const auto& a : list)
		ret.append(a).append(" ");

	return ret;
}

// -----------------------------------------------------------------------------
// Returns a sorted wxArrayString of all words of [type] in the language
// -----------------------------------------------------------------------------
vector<string> TextLanguage::wordListSorted(WordType type, bool include_custom) const
{
	// Get list
	vector<string> list;
	list.reserve(word_lists_[type].list.size());
	for (auto& word : word_lists_[type].list)
		list.push_back(word);

	if (include_custom)
		for (auto& word : word_lists_custom_[type].list)
			list.push_back(word);

	// Sort
	std::sort(list.begin(), list.end());

	return list;
}

// -----------------------------------------------------------------------------
// Returns a sorted wxArrayString of all functions in the language
// -----------------------------------------------------------------------------
vector<string> TextLanguage::functionsSorted() const
{
	// Get list
	vector<string> list;
	list.reserve(functions_.size());
	for (auto& func : functions_)
		list.push_back(func.name());

	// Sort
	std::sort(list.begin(), list.end());

	return list;
}

// -----------------------------------------------------------------------------
// Returns true if [word] is a [type] word in this language, false otherwise
// -----------------------------------------------------------------------------
bool TextLanguage::isWord(WordType type, string_view word) const
{
	for (auto& w : word_lists_[type].list)
		if (w == word)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns true if [word] is a function in this language, false otherwise
// -----------------------------------------------------------------------------
bool TextLanguage::isFunction(string_view word) const
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
TLFunction* TextLanguage::function(string_view name)
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
			if (strutil::equalCI(func.name(), name))
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
	for (auto i = static_cast<int>(functions_.size()) - 1; i >= 0; --i)
	{
		functions_[i].clearCustomContexts();

		// Remove function if only contexts were custom
		if (functions_[i].contexts().empty())
			functions_.erase(functions_.begin() + i);
	}

	for (auto& a : word_lists_custom_)
		a.list.clear();
}

// -----------------------------------------------------------------------------
// Reads a language definition from a JSON object [j]
// -----------------------------------------------------------------------------
void TextLanguage::fromJson(const Json& j)
{
	// Copy from base language, if defined
	if (j.contains("base"))
	{
		auto base_id = j["base"].get<string>();
		if (auto* base = fromId(base_id))
			base->copyTo(this);
		else
			log::warning("Warning: Language {} is based on undefined language {}", id_, base_id);
	}

	// Language info
	name_ = j.value("name", id_);
	jsonutil::getIf(j, "comment_begin", comment_begin_l_);
	jsonutil::getIf(j, "comment_end", comment_end_l_);
	jsonutil::getIf(j, "comment_line", line_comment_l_);
	jsonutil::getIf(j, "comment_doc", doc_comment_);
	jsonutil::getIf(j, "case_sensitive", case_sensitive_);
	jsonutil::getIf(j, "preprocessor", preprocessor_);
	jsonutil::getIf(j, "block_begin", block_begin_);
	jsonutil::getIf(j, "block_end", block_end_);
	jsonutil::getIf(j, "pp_block_begin", pp_block_begin_);
	jsonutil::getIf(j, "pp_block_end", pp_block_end_);
	jsonutil::getIf(j, "word_block_begin", word_block_begin_);
	jsonutil::getIf(j, "word_block_end", word_block_end_);
	jsonutil::getIf(j, "keyword_link", word_lists_[Keyword].lookup_url);
	jsonutil::getIf(j, "constant_link", word_lists_[Constant].lookup_url);
	jsonutil::getIf(j, "function_link", f_lookup_url_);
	jsonutil::getIf(j, "blocks", jump_blocks_);
	jsonutil::getIf(j, "blocks_ignore", jb_ignore_);

	// Keywords
	if (j.value("override_keywords", false))
		clearWordList(Keyword);
	if (j.contains("keywords"))
		for (auto& a : j["keywords"].get<vector<string>>())
			addWord(Keyword, a);

	// Constants
	if (j.value("override_constants", false))
		clearWordList(Constant);
	if (j.contains("constants"))
		for (auto& a : j["constants"].get<vector<string>>())
			addWord(Constant, a);

	// Types
	if (j.value("override_types", false))
		clearWordList(Type);
	if (j.contains("types"))
		for (auto& a : j["types"].get<vector<string>>())
			addWord(Type, a);

	// Properties
	if (j.value("override_properties", false))
		clearWordList(Property);
	if (j.contains("properties"))
		for (auto& a : j["properties"].get<vector<string>>())
			addWord(Property, a);

	// Functions
	if (j.contains("functions"))
	{
		bool has_void = isWord(Keyword, "void") || isWord(Type, "void");
		for (auto& [name, j_func] : j["functions"].items())
			addFunction(name, j_func, has_void);
	}

	// Function info (for ZScript function info that can't be parsed from (g)zdoom.pk3)
	if (j.contains("function_info"))
		for (auto& [name, j_func_info] : j["function_info"].items())
		{
			ZFuncExProp ex_prop;
			if (j_func_info.contains("description"))
				ex_prop.description = j_func_info["description"].get<string>();
			if (j_func_info.contains("deprecated_f"))
				ex_prop.deprecated_f = j_func_info["deprecated_f"].get<string>();
			zfuncs_ex_props_.emplace(name, ex_prop);
		}
}


// -----------------------------------------------------------------------------
//
// TextLanguage Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Loads all text language definitions from slade.pk3
// -----------------------------------------------------------------------------
bool TextLanguage::loadLanguages()
{
	// Get slade resource archive
	auto* res_archive = app::archiveManager().programResourceArchive();

	// Read language definitions from resource archive
	if (res_archive)
	{
		// Get 'config/languages' directly
		auto* dir = res_archive->dirAtPath("config/languages");

		if (dir)
		{
			// Get list of config entries (to sort)
			vector<ArchiveEntry*> defs;
			for (const auto& entry : dir->entries())
				defs.push_back(entry.get());

			// Sort by name
			std::sort(
				defs.begin(),
				defs.end(),
				[](const ArchiveEntry* left, const ArchiveEntry* right) { return left->name() < right->name(); });

			// Read all (sorted) entries in this dir
			for (auto* entry : defs)
			{
				if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
				{
					for (auto& [id, j_lang] : j.items())
					{
						// Create language
						auto* lang = new TextLanguage(id);
						lang->fromJson(j_lang);
					}
				}
			}
		}
		else
			log::warning(
				1, "Warning: 'config/languages' not found in slade.pk3, no builtin text language definitions loaded");
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the language definition matching [id], or NULL if no match found
// -----------------------------------------------------------------------------
TextLanguage* TextLanguage::fromId(string_view id)
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
TextLanguage* TextLanguage::fromName(string_view name)
{
	// Find text language matching [name]
	for (auto& text_language : text_languages)
	{
		if (strutil::equalCI(text_language->name_, name))
			return text_language;
	}

	// Not found
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns a list of all language names
// -----------------------------------------------------------------------------
vector<string> TextLanguage::languageNames()
{
	vector<string> ret;

	ret.reserve(text_languages.size());
	for (auto& text_language : text_languages)
		ret.push_back(text_language->name_);

	return ret;
}
