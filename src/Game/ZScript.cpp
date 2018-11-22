
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ZScript.cpp
// Description: ZScript definition classes and parsing
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "ZScript.h"
#include "Archive/Archive.h"
#include "Utility/Tokenizer.h"
#include "Archive/ArchiveManager.h"

using namespace ZScript;


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace ZScript
{
	EntryType*	etype_zscript = nullptr;

	// ZScript keywords (can't be function/variable names)
	vector<string> keywords =
	{
		"class", "default", "private", "static", "native", "return", "if", "else", "for", "while", "do", "break",
		"continue", "deprecated", "state", "null", "readonly", "true", "false", "struct", "extend", "clearscope",
		"vararg", "ui", "play", "virtual", "virtualscope", "meta", "Property", "version", "in", "out", "states",
		"action", "override", "super", "is", "let", "const", "replaces", "protected", "self"
	};

	// For test_parse_zscript console command
	bool	dump_parsed_blocks = false;
	bool	dump_parsed_states = false;
	bool	dump_parsed_functions = false;

	string db_comment = "//$";
}


// ----------------------------------------------------------------------------
//
// Local Functions
//
// ----------------------------------------------------------------------------
namespace ZScript
{

// ----------------------------------------------------------------------------
// logParserMessage
//
// Writes a log [message] of [type] beginning with the location of [statement]
// ----------------------------------------------------------------------------
void logParserMessage(ParsedStatement& statement, Log::MessageType type, const string& message)
{
	string location = "<unknown location>";
	if (statement.entry)
		location = statement.entry->getPath(true);

	Log::message(type, S_FMT("%s:%u: %s", CHR(location), statement.line, CHR(message)));
}

// ----------------------------------------------------------------------------
// parseType
//
// Parses a ZScript type (eg. 'class<Actor>') from [tokens] beginning at
// [index]
// ----------------------------------------------------------------------------
string parseType(const vector<string>& tokens, unsigned& index)
{
	string type;

	// Qualifiers
	while (index < tokens.size())
	{
		if (S_CMPNOCASE(tokens[index], "in") ||
			S_CMPNOCASE(tokens[index], "out"))
			type += tokens[index++] + " ";
		else
			break;
	}

	type += tokens[index];

	// Check for ...
	if (index + 2 < tokens.size() && tokens[index] == '.' && tokens[index + 1] == '.' && tokens[index + 2] == '.')
	{
		type = "...";
		index += 2;
	}
	
	// Check for <>
	if (tokens[index + 1] == "<")
	{
		type += "<";
		index += 2;
		while (index < tokens.size() && tokens[index] != ">")
			type += tokens[index++];
		type += ">";
		++index;
	}

	++index;

	return type;
}

// ----------------------------------------------------------------------------
// parseValue
//
// Parses a ZScript value from [tokens] beginning at [index]
// ----------------------------------------------------------------------------
string parseValue(const vector<string>& tokens, unsigned& index)
{
	string value;
	while (true)
	{
		// Read between ()
		if (tokens[index] == '(')
		{
			int level = 1;
			value += tokens[index++];
			while (level > 0)
			{
				if (tokens[index] == '(')
					++level;
				if (tokens[index] == ')')
					--level;

				value += tokens[index++];
			}

			continue;
		}

		if (tokens[index] == ',' || tokens[index] == ';' || tokens[index] == ')')
			break;

		value += tokens[index++];

		if (index >= tokens.size())
			break;
	}

	return value;
}

// ----------------------------------------------------------------------------
// checkKeywordValueStatement
//
// Checks for a ZScript keyword+value statement in [tokens] beginning at
// [index], eg. deprecated("#.#") or version("#.#")
// Returns true if there is a keyword+value statement and writes the value to
// [value]
// ----------------------------------------------------------------------------
bool checkKeywordValueStatement(const vector<string>& tokens, unsigned index, const string& word, string& value)
{
	if (index + 3 >= tokens.size())
		return false;

	if (S_CMPNOCASE(tokens[index], word) &&
		tokens[index + 1] == '(' &&
		tokens[index + 3] == ')')
	{
		value = tokens[index + 2];
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// parseBlocks
//
// Parses all statements/blocks in [entry], adding them to [parsed]
// ----------------------------------------------------------------------------
void parseBlocks(ArchiveEntry* entry, vector<ParsedStatement>& parsed)
{
	Tokenizer tz;
	tz.setSpecialCharacters(CHR(Tokenizer::DEFAULT_SPECIAL_CHARACTERS + "()+-[]&!?."));
	tz.enableDecorate(true);
	tz.setCommentTypes(Tokenizer::CommentTypes::CPPStyle | Tokenizer::CommentTypes::CStyle);
	tz.openMem(entry->getMCData(), "ZScript");

	//Log::info(2, S_FMT("Parsing ZScript entry \"%s\"", entry->getPath(true)));

	while (!tz.atEnd())
	{
		// Preprocessor
		if (tz.current().text.StartsWith("#"))
		{
			if (tz.checkNC("#include"))
			{
				auto inc_entry = entry->relativeEntry(tz.next().text);

				// Check #include path could be resolved
				if (!inc_entry)
				{
					Log::warning(
						S_FMT(
							"Warning parsing ZScript entry %s: "
							"Unable to find #included entry \"%s\" at line %u, skipping",
							CHR(entry->getName()),
							CHR(tz.current().text),
							tz.current().line_no
						));
				}
				else
					parseBlocks(inc_entry, parsed);
			}

			tz.advToNextLine();
			continue;
		}

		// Version
		else if (tz.checkNC("version"))
		{
			tz.advToNextLine();
			continue;
		}

		// ZScript
		parsed.push_back({});
		parsed.back().entry = entry;
		if (!parsed.back().parse(tz))
			parsed.pop_back();
	}

	// Set entry type
	if (etype_zscript && entry->getType() != etype_zscript)
		entry->setType(etype_zscript);
}

bool isKeyword(const string& word)
{
	for (auto& kw : keywords)
		if (S_CMPNOCASE(word, kw))
			return true;

	return false;
}

} // namespace ZScript


// ----------------------------------------------------------------------------
//
// Enumerator Class Functions
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Enumerator::parse
//
// Parses an enumerator block [statement]
// ----------------------------------------------------------------------------
bool Enumerator::parse(ParsedStatement& statement)
{
	// Check valid statement
	if (statement.block.empty())
		return false;
	if (statement.tokens.size() < 2)
		return false;

	// Parse name
	name_ = statement.tokens[1];

	// Parse values
	unsigned index = 0;
	unsigned count = statement.block[0].tokens.size();
	while (index < count)
	{
		string val_name = statement.block[0].tokens[index];

		// TODO: Parse value

		values_.push_back({ val_name, 0 });

		// Skip past next ,
		while (index + 1 < count)
			if (statement.block[0].tokens[++index] == ',')
				break;

		index++;
	}

	return true;
}


// ----------------------------------------------------------------------------
//
// Function Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Function::Parameter::parse
//
// Parses a function parameter from [tokens] beginning at [index]
// ----------------------------------------------------------------------------
unsigned Function::Parameter::parse(const vector<string>& tokens, unsigned index)
{
	// Type
	type = parseType(tokens, index);
	
	// Special case - '...'
	if (type == "...")
	{
		name = "...";
		type.clear();
		return index;
	}

	// Name
	if (index >= tokens.size() || tokens[index] == ')')
		return index;
	name = tokens[index++];

	// Default value
	if (index < tokens.size() && tokens[index] == '=')
	{
		++index;
		default_value = parseValue(tokens, index);
	}

	return index;
}

// ----------------------------------------------------------------------------
// Function::parse
//
// Parses a function declaration [statement]
// ----------------------------------------------------------------------------
bool Function::parse(ParsedStatement& statement)
{
	unsigned index;
	int last_qualifier = -1;
	for (index = 0; index < statement.tokens.size(); index++)
	{
		if (S_CMPNOCASE(statement.tokens[index], "virtual"))
		{
			virtual_ = true;
			last_qualifier = index;
		}
		else if (S_CMPNOCASE(statement.tokens[index], "static"))
		{
			static_ = true;
			last_qualifier = index;
		}
		else if (S_CMPNOCASE(statement.tokens[index], "native"))
		{
			native_ = true;
			last_qualifier = index;
		}
		else if (S_CMPNOCASE(statement.tokens[index], "action"))
		{
			action_ = true;
			last_qualifier = index;
		}
		else if (S_CMPNOCASE(statement.tokens[index], "override"))
		{
			override_ = true;
			last_qualifier = index;
		}
		else if ((int)index > last_qualifier + 2 && statement.tokens[index] == '(')
		{
			name_ = statement.tokens[index - 1];
			return_type_ = statement.tokens[index - 2];
			break;
		}
		else if (checkKeywordValueStatement(statement.tokens, index, "deprecated", deprecated_))
			index += 3;
		else if (checkKeywordValueStatement(statement.tokens, index, "version", version_))
			index += 3;
	}

	if (name_.empty() || return_type_.empty())
	{
		logParserMessage(statement, Log::MessageType::Warning, "Function parse failed");
		return false;
	}

	// Name can't be a keyword
	if (isKeyword(name_))
	{
		logParserMessage(statement, Log::MessageType::Warning, "Function name can't be a keyword");
		return false;
	}

	// Parse parameters
	while (statement.tokens[index] != '(')
	{
		if (index == statement.tokens.size())
			return true;
		++index;
	}
	++index; // Skip (

	while (statement.tokens[index] != ')' && index < statement.tokens.size())
	{
		parameters_.emplace_back();
		index = parameters_.back().parse(statement.tokens, index);

		if (statement.tokens[index] == ',')
			++index;
	}

	if (dump_parsed_functions)
		Log::debug(asString());

	return true;
}

// ----------------------------------------------------------------------------
// Function::asString
//
// Returns a string representation of the function
// ----------------------------------------------------------------------------
string Function::asString()
{
	string str;
	if (!deprecated_.empty())
		str += S_FMT("deprecated v%s ", CHR(deprecated_));
	if (static_)
		str += "static ";
	if (native_)
		str += "native ";
	if (virtual_)
		str += "virtual ";
	if (action_)
		str += "action ";

	str += S_FMT("%s %s(", CHR(return_type_), CHR(name_));

	for (auto& p : parameters_)
	{
		str += S_FMT("%s %s", CHR(p.type), CHR(p.name));
		if (!p.default_value.empty())
			str += " = " + p.default_value;

		if (&p != &parameters_.back())
			str += ", ";
	}
	str += ")";

	return str;
}

// ----------------------------------------------------------------------------
// Function::isFunction
//
// Returns true if [statement] is a valid function declaration
// ----------------------------------------------------------------------------
bool Function::isFunction(ParsedStatement& statement)
{
	// Need at least type, name, (, )
	if (statement.tokens.size() < 4)
		return false;

	// Check for ( before =
	bool special_func = false;
	for (auto& token : statement.tokens)
	{
		if (token == '=')
			return false;

		if (!special_func && token == '(')
			return true;

		if (S_CMPNOCASE(token, "deprecated") ||
			S_CMPNOCASE(token, "version"))
			special_func = true;
		else if (special_func && token == ')')
			special_func = false;
	}

	// No ( found
	return false;
}


// ----------------------------------------------------------------------------
//
// State Struct Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// State::editorSprite
//
// Returns the first valid frame sprite (eg. TNT1 A -> TNT1A?)
// ----------------------------------------------------------------------------
string State::editorSprite()
{
	if (frames.empty())
		return "";

	for (auto& f : frames)
		if (!f.sprite_frame.empty())
			return f.sprite_base + f.sprite_frame[0] + "?";

	return "";
}


// ----------------------------------------------------------------------------
//
// StateTable Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// StateTable::parse
//
// Parses a states definition statement/block [states]
// ----------------------------------------------------------------------------
bool StateTable::parse(ParsedStatement& states)
{
	vector<string> current_states;
	for (auto& statement : states.block)
	{
		if (statement.tokens.empty())
			continue;

		auto states_added = false;
		auto index = 0u;

		// Check for state labels
		for (auto a = 0u; a < statement.tokens.size(); ++a)
		{
			if (statement.tokens[a] == ':')
			{
				// Ignore ::
				if (a + 1 < statement.tokens.size() && statement.tokens[a + 1] == ':')
				{
					++a;
					continue;
				}

				if (!states_added)
					current_states.clear();

				string state;
				for (auto b = index; b < a; ++b)
					state += statement.tokens[b];

				current_states.push_back(state.Lower());
				if (state_first_.empty())
					state_first_ = state;
				states_added = true;

				index = a + 1;
			}
		}

		if (index >= statement.tokens.size())
		{
			logParserMessage(
				statement,
				Log::MessageType::Warning,
				S_FMT("Failed to parse states block beginning on line %u", states.line)
			);
			continue;
		}

		// Ignore state commands
		if (S_CMPNOCASE(statement.tokens[index], "stop") ||
			S_CMPNOCASE(statement.tokens[index], "goto") ||
			S_CMPNOCASE(statement.tokens[index], "loop") ||
			S_CMPNOCASE(statement.tokens[index], "wait") ||
			S_CMPNOCASE(statement.tokens[index], "fail"))
			continue;

		if (index + 2 < statement.tokens.size())
		{
			// Parse duration
			long duration = 0;
			if (statement.tokens[index + 2] == "-" && index + 3 < statement.tokens.size())
			{
				// Negative number
				statement.tokens[index + 3].ToLong(&duration);
				duration = -duration;
			}
			else
				statement.tokens[index + 2].ToLong(&duration);

			for (auto& state : current_states)
				states_[state].frames.push_back({ statement.tokens[index], statement.tokens[index + 1], (int)duration });
		}
	}

	states_.erase("");

	if (dump_parsed_states)
	{
		for (auto& state : states_)
		{
			Log::debug(S_FMT("State %s:", CHR(state.first)));
			for (auto& frame : state.second.frames)
				Log::debug(S_FMT(
					"Sprite: %s, Frames: %s, Duration: %d",
					CHR(frame.sprite_base),
					CHR(frame.sprite_frame),
					frame.duration
				));
		}
	}

	return true;
}

// ----------------------------------------------------------------------------
// StateTable::editorSprite
//
// Returns the most appropriate sprite from the state table to use for the
// editor.
// Uses a state priority: Idle > See > Inactive > Spawn > [first defined]
// ----------------------------------------------------------------------------
string StateTable::editorSprite()
{
	if (!states_["idle"].frames.empty())
		return states_["idle"].editorSprite();
	if (!states_["see"].frames.empty())
		return states_["see"].editorSprite();
	if (!states_["inactive"].frames.empty())
		return states_["inactive"].editorSprite();
	if (!states_["spawn"].frames.empty())
		return states_["spawn"].editorSprite();
	if (!states_[state_first_].frames.empty())
		return states_[state_first_].editorSprite();

	return "";
}


// ----------------------------------------------------------------------------
//
// Class Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Class::parse
//
// Parses a class definition statement/block [class_statement]
// ----------------------------------------------------------------------------
bool Class::parse(ParsedStatement& class_statement)
{
	if (class_statement.tokens.size() < 2)
	{
		logParserMessage(class_statement, Log::MessageType::Warning, "Class parse failed");
		return false;
	}

	name_ = class_statement.tokens[1];

	for (unsigned a = 0; a < class_statement.tokens.size(); a++)
	{
		// Inherits
		if (class_statement.tokens[a] == ':' && a < class_statement.tokens.size() - 1)
			inherits_class_ = class_statement.tokens[a + 1];

		// Native
		else if (S_CMPNOCASE(class_statement.tokens[a], "native"))
			native_ = true;

		// Deprecated
		else if (checkKeywordValueStatement(class_statement.tokens, a, "deprecated", deprecated_))
			a += 3;

		// Version
		else if (checkKeywordValueStatement(class_statement.tokens, a, "version", version_))
			a += 3;
	}

	if (!parseClassBlock(class_statement.block))
	{
		logParserMessage(class_statement, Log::MessageType::Warning, "Class parse failed");
		return false;
	}

	// Set editor sprite from parsed states
	default_properties_["sprite"] = states_.editorSprite();

	// Add DB comment props to default properties
	for (auto& i : db_properties_)
	{
		// Sprite
		if (S_CMPNOCASE(i.first, "EditorSprite") || S_CMPNOCASE(i.first, "Sprite"))
			default_properties_["sprite"] = i.second;

		// Angled
		else if (S_CMPNOCASE(i.first, "Angled"))
			default_properties_["angled"] = true;
		else if (S_CMPNOCASE(i.first, "NotAngled"))
			default_properties_["angled"] = false;

		// Is Decoration
		else if (S_CMPNOCASE(i.first, "IsDecoration"))
			default_properties_["decoration"] = true;

		// Icon
		else if (S_CMPNOCASE(i.first, "Icon"))
			default_properties_["icon"] = i.second;

		// DB2 Color
		else if (S_CMPNOCASE(i.first, "Color"))
			default_properties_["color"] = i.second;

		// SLADE 3 Colour (overrides DB2 color)
		// Good thing US spelling differs from ABC (Aussie/Brit/Canuck) spelling! :p
		else if (S_CMPNOCASE(i.first, "Colour"))
			default_properties_["colour"] = i.second;

		// Obsolete thing
		else if (S_CMPNOCASE(i.first, "Obsolete"))
			default_properties_["obsolete"] = true;
	}

	return true;
}

// ----------------------------------------------------------------------------
// Class::extend
//
// Parses a class definition block [block] only (ignores the class declaration
// statement line, used for 'extend class')
// ----------------------------------------------------------------------------
bool Class::extend(ParsedStatement& block)
{
	return parseClassBlock(block.block);
}

// ----------------------------------------------------------------------------
// Class::toThingType
//
// Adds this class as a ThingType to [parsed], or updates an existing
// ThingType definition in [types] or [parsed]
// ----------------------------------------------------------------------------
void Class::toThingType(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed)
{
	// Find existing definition
	Game::ThingType* def = nullptr;

	// Check types with ednums first
	for (auto& type : types)
		if (S_CMPNOCASE(name_, type.second.className()))
		{
			def = &type.second;
			break;
		}
	if (!def)
	{
		// Check all parsed types
		for (auto& type : parsed)
			if (S_CMPNOCASE(name_, type.className()))
			{
				def = &type;
				break;
			}
	}

	// Create new type if it didn't exist
	if (!def)
	{
		parsed.emplace_back(name_, "ZScript", name_);
		def = &parsed.back();
	}

	// Set properties from DB comments
	string title = name_;
	string group = "ZScript";
	for (auto& prop : db_properties_)
	{
		if (S_CMPNOCASE(prop.first, "Title"))
			title = prop.second;
		else if (S_CMPNOCASE(prop.first, "Group") || S_CMPNOCASE(prop.first, "Category"))
			group = "ZScript/" + prop.second;
	}
	def->define(def->number(), title, group);

	// Set properties from defaults section
	def->loadProps(default_properties_, true, true);
}

// ----------------------------------------------------------------------------
// Class::parseClassBlock
//
// Parses a class definition from statements in [block]
// ----------------------------------------------------------------------------
bool Class::parseClassBlock(vector<ParsedStatement>& block)
{
	for (auto& statement : block)
	{
		if (statement.tokens.empty())
			continue;

		// Default block
		if (S_CMPNOCASE(statement.tokens[0], "default"))
		{
			if (!parseDefaults(statement.block))
				return false;
		}

		// Enum
		else if (S_CMPNOCASE(statement.tokens[0], "enum"))
		{
			Enumerator e;
			if (!e.parse(statement))
				return false;

			enumerators_.push_back(e);
		}

		// States
		else if (S_CMPNOCASE(statement.tokens[0], "states"))
			states_.parse(statement);

		// DB property comment
		else if (statement.tokens[0].StartsWith(db_comment))
		{
			if (statement.tokens.size() > 1)
				db_properties_.emplace_back(statement.tokens[0].substr(3),  statement.tokens[1]);
			else
				db_properties_.emplace_back(statement.tokens[0].substr(3), "true");
		}

		// Function
		else if (Function::isFunction(statement))
		{
			Function fn;
			if (fn.parse(statement))
				functions_.push_back(fn);
		}

		// Variable
		else if (statement.tokens.size() >= 2 && statement.block.empty())
			continue;
	}

	return true;
}

// ----------------------------------------------------------------------------
// Class::parseDefaults
//
// Parses a 'default' block from statements in [block]
// ----------------------------------------------------------------------------
bool Class::parseDefaults(vector<ParsedStatement>& defaults)
{
	for (auto& statement : defaults)
	{
		if (statement.tokens.empty())
			continue;

		// DB property comment
		if (statement.tokens[0].StartsWith(db_comment))
		{
			if (statement.tokens.size() > 1)
				db_properties_.emplace_back(statement.tokens[0].substr(3),  statement.tokens[1]);
			else
				db_properties_.emplace_back(statement.tokens[0].substr(3), "true");
			continue;
		}

		// Flags
		unsigned t = 0;
		unsigned count = statement.tokens.size();
		while (t < count)
		{
			if (statement.tokens[t] == '+')
				default_properties_[statement.tokens[++t]] = true;
			else if (statement.tokens[t] == '-')
				default_properties_[statement.tokens[++t]] = false;
			else
				break;

			t++;
		}

		if (t >= count)
			continue;

		// Name
		string name = statement.tokens[t];
		if (t + 2 < count && statement.tokens[t + 1] == '.')
		{
			name += "." + statement.tokens[t + 2];
			t += 2;
		}

		// Value
		// For now ignore anything after the first whitespace/special character
		// so stuff like arithmetic expressions or comma separated lists won't
		// really work properly yet
		if (t + 1 < count)
			default_properties_[name] = statement.tokens[t + 1];

		// Name only (no value), set as boolean true
		else if (t < count)
			default_properties_[name] = true;
	}

	return true;
}


// ----------------------------------------------------------------------------
//
// Definitions Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Definitions::clear
//
// Clears all definitions
// ----------------------------------------------------------------------------
void Definitions::clear()
{
	classes_.clear();
	enumerators_.clear();
	variables_.clear();
	functions_.clear();
}

// ----------------------------------------------------------------------------
// Definitions::parseZScript
//
// Parses ZScript in [entry]
// ----------------------------------------------------------------------------
bool Definitions::parseZScript(ArchiveEntry* entry)
{
	// Parse into tree of expressions and blocks
	auto start = App::runTimer();
	vector<ParsedStatement> parsed;
	parseBlocks(entry, parsed);
	Log::debug(2, S_FMT("parseBlocks: %ldms", App::runTimer() - start));
	start = App::runTimer();

	for (auto& block : parsed)
	{
		if (block.tokens.empty())
			continue;

		if (dump_parsed_blocks)
			block.dump();

		// Class
		if (S_CMPNOCASE(block.tokens[0], "class"))
		{
			Class nc(Class::Type::Class);

			if (!nc.parse(block))
				return false;

			classes_.push_back(nc);
		}

		// Struct
		else if (S_CMPNOCASE(block.tokens[0], "struct"))
		{
			Class nc(Class::Type::Struct);

			if (!nc.parse(block))
				return false;

			classes_.push_back(nc);
		}

		// Extend Class
		else if (block.tokens.size() > 2 &&
				S_CMPNOCASE(block.tokens[0], "extend") &&
				S_CMPNOCASE(block.tokens[1], "class"))
		{
			for (auto& c : classes_)
				if (S_CMPNOCASE(c.name(), block.tokens[2]))
				{
					c.extend(block);
					break;
				}
		}

		// Enum
		else if (S_CMPNOCASE(block.tokens[0], "enum"))
		{
			Enumerator e;

			if (!e.parse(block))
				return false;

			enumerators_.push_back(e);
		}
	}

	Log::debug(2, S_FMT("ZScript: %ldms", App::runTimer() - start));

	return true;
}

// ----------------------------------------------------------------------------
// Definitions::parseZScript
//
// Parses all ZScript entries in [archive]
// ----------------------------------------------------------------------------
bool Definitions::parseZScript(Archive* archive)
{
	// Get base ZScript file
	Archive::SearchOptions opt;
	opt.match_name = "zscript";
	opt.ignore_ext = true;
	vector<ArchiveEntry*> zscript_enries = archive->findAll(opt);
	if (zscript_enries.empty())
		return false;

	Log::info(2, S_FMT("Parsing ZScript entries found in archive %s", archive->filename()));

	// Get ZScript entry type (all parsed ZScript entries will be set to this)
	etype_zscript = EntryType::fromId("zscript");
	if (etype_zscript == EntryType::unknownType())
		etype_zscript = nullptr;

	// Parse ZScript entries
	bool ok = true;
	for (auto entry : zscript_enries)
		if (!parseZScript(entry))
			ok = false;

	return ok;
}

// ----------------------------------------------------------------------------
// Definitions::exportThingTypes
//
// Exports all classes to ThingTypes in [types] and [parsed] (from a
// Game::Configuration object)
// ----------------------------------------------------------------------------
void Definitions::exportThingTypes(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed)
{
	for (auto& cdef : classes_)
		cdef.toThingType(types, parsed);
}


// ----------------------------------------------------------------------------
//
// ParsedStatement Struct Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ParsedStatement::parse
//
// Parses a ZScript 'statement'. This isn't technically correct but suits our
// purposes well enough
//
// tokens
// {
//     block[0].tokens
//     {
//         block[0].block[0].tokens;
//         ...
//     }
//
//     block[1].tokens;
//     ...
// }
// ----------------------------------------------------------------------------
bool ParsedStatement::parse(Tokenizer& tz)
{
	// Check for unexpected token
	if (tz.check('}'))
	{
		tz.adv();
		return false;
	}

	line = tz.lineNo();

	// Tokens
	bool in_initializer = false;
	while (true)
	{
		// End of statement (;)
		if (tz.advIf(';'))
			return true;

		// DB comment
		if (tz.current().text.StartsWith(db_comment))
		{
			tokens.push_back(tz.current().text);
			tokens.push_back(tz.getLine());
			return true;
		}

		if (tz.check('}'))
		{
			// End of array initializer
			if (in_initializer)
			{
				in_initializer = false;
				tokens.emplace_back("}");
				tz.adv();
				continue;
			}

			// End of statement
			return true;
		}

		if (tz.atEnd())
		{
			Log::debug(S_FMT("Failed parsing zscript statement/block beginning line %u", line));
			return false;
		}

		// Beginning of block
		if (tz.advIf('{'))
			break;

		// Array initializer: ... = { ... }
		if (tz.current().text.Cmp("=") == 0 && tz.peek() == '{')
		{
			tokens.emplace_back("=");
			tokens.emplace_back("{");
			tz.adv(2);
			in_initializer = true;
			continue;
		}
		
		tokens.push_back(tz.current().text);
		tz.adv();
	}

	// Block
	while (true)
	{
		if (tz.advIf('}'))
			return true;

		if (tz.atEnd())
		{
			Log::debug(S_FMT("Failed parsing zscript statement/block beginning line %u", line));
			return false;
		}

		block.push_back({});
		block.back().entry = entry;
		if (!block.back().parse(tz) || block.back().tokens.empty())
			block.pop_back();
	}
}

// ----------------------------------------------------------------------------
// ParsedStatement::dump
//
// Dumps this statement to the log (debug), indenting by 2*[indent] spaces
// ----------------------------------------------------------------------------
void ParsedStatement::dump(int indent)
{
	string line = "";
	for (int a = 0; a < indent; a++)
		line += "  ";

	// Tokens
	for (auto& token : tokens)
		line += token + " ";
	Log::debug(line);

	// Blocks
	for (auto& b : block)
		b.dump(indent + 1);
}







// TESTING CONSOLE COMMANDS
#include "MainEditor/MainEditor.h"
#include "General/Console/Console.h"

CONSOLE_COMMAND(test_parse_zscript, 0, false)
{
	dump_parsed_blocks = false;
	dump_parsed_states = false;
	dump_parsed_functions = false;
	ArchiveEntry* entry = nullptr;

	for (auto& arg : args)
	{
		if (S_CMPNOCASE(arg, "dump"))
			dump_parsed_blocks = true;
		else if (S_CMPNOCASE(arg, "states"))
			dump_parsed_states = true;
		else if (S_CMPNOCASE(arg, "func"))
			dump_parsed_functions = true;
		else if (!entry)
			entry = MainEditor::currentArchive()->entryAtPath(arg);
	}

	if (!entry)
		entry = MainEditor::currentEntry();

	if (entry)
	{
		Definitions test;
		if (test.parseZScript(entry))
			Log::console("Parsed Successfully");
		else
			Log::console("Parsing failed");
	}
	else
		Log::console("Select an entry or enter a valid entry name/path");

	dump_parsed_blocks = false;
	dump_parsed_states = false;
	dump_parsed_functions = false;
}


CONSOLE_COMMAND(test_parseblocks, 1, false)
{
	long num = 1;
	args[0].ToLong(&num);

	auto entry = MainEditor::currentEntry();
	if (!entry)
		return;

	auto start = App::runTimer();
	vector<ParsedStatement> parsed;
	for (auto a = 0; a < num; ++a)
	{
		parseBlocks(entry, parsed);
		parsed.clear();
	}
	Log::console(S_FMT("Took %ldms", App::runTimer() - start));
}
