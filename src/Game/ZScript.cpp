
#include "Main.h"
#include "ZScript.h"
#include "Archive/Archive.h"
#include "Utility/Tokenizer.h"
#include "Archive/ArchiveManager.h"

namespace ZScript
{
	bool dump_parsed_blocks = false;
	bool dump_parsed_states = false;
	bool dump_parsed_functions = false;
}


using namespace ZScript;

namespace ZScript
{

string parseType(const vector<string>& tokens, unsigned& index)
{
	auto type = tokens[index];

	// Check for 'out'
	if (S_CMPNOCASE(type, "out"))
		type = "out " + tokens[++index];

	// Check for ...
	if (index + 2 < tokens.size() && tokens[index] == "." && tokens[index + 1] == "." && tokens[index + 2] == ".")
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

string parseValue(const vector<string>& tokens, unsigned& index)
{
	string value;
	while (true)
	{
		// Read between ()
		if (tokens[index] == "(")
		{
			int level = 1;
			value += tokens[index++];
			while (level > 0)
			{
				if (tokens[index] == "(")
					++level;
				if (tokens[index] == ")")
					--level;

				value += tokens[index++];
			}

			continue;
		}

		if (tokens[index] == "," || tokens[index] == ";" || tokens[index] == ")")
			break;

		value += tokens[index++];

		if (index >= tokens.size())
			break;
	}

	return value;
}

}

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
		while (index < count)
			if (statement.block[0].tokens[++index] == ",")
				break;

		index++;
	}

	return true;
}

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
	if (index >= tokens.size() || tokens[index] == ")")
		return index;
	name = tokens[index++];

	// Default value
	if (index < tokens.size() && tokens[index] == "=")
	{
		++index;
		default_value = parseValue(tokens, index);
	}

	return index;
}

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
		else if (index > last_qualifier + 2 && statement.tokens[index] == "(")
		{
			name_ = statement.tokens[index - 1];
			return_type_ = statement.tokens[index - 2];
			break;
		}
	}

	if (name_.empty() || return_type_.empty())
		return false;

	// Parse parameters
	while (statement.tokens[index] != "(")
	{
		if (index == statement.tokens.size())
			return true;
		++index;
	}
	++index; // Skip (

	while (statement.tokens[index] != ")" && index < statement.tokens.size())
	{
		parameters_.push_back({});
		index = parameters_.back().parse(statement.tokens, index);

		if (statement.tokens[index] == ",")
			++index;
	}

	if (dump_parsed_functions)
		Log::debug(asString());

	return true;
}

string Function::asString()
{
	string str;
	if (deprecated_)
		str += "deprecated ";
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

bool Function::isFunction(ParsedStatement& statement)
{
	// Need at least type, name, (, )
	if (statement.tokens.size() < 4)
		return false;

	// Check for ( before =
	bool deprecated_func = false;
	for (unsigned a = 0; a < statement.tokens.size(); a++)
	{
		if (statement.tokens[a] == "=")
			return false;

		if (!deprecated_func && statement.tokens[a] == "(")
			return true;

		if (S_CMPNOCASE(statement.tokens[a], "deprecated"))
			deprecated_func = true;
		else if (deprecated_func && statement.tokens[a] == ")")
			deprecated_func = false;
	}

	// No ( found
	return false;
}


string State::editorSprite()
{
	if (frames.empty())
		return "";

	for (auto& f : frames)
		if (!f.sprite_frame.empty())
			return f.sprite_base + f.sprite_frame[0] + "?";

	return "";
}

bool StateTable::parse(ParsedStatement& states)
{
	vector<string> current_states;
	for (auto& statement : states.block)
	{
		auto states_added = false;
		auto index = 0u;

		// Check for state labels
		for (auto a = 0u; a < statement.tokens.size(); ++a)
		{
			if (statement.tokens[a] == ":")
			{
				// Ignore ::
				if (a + 1 < statement.tokens.size() && statement.tokens[a + 1] == ":")
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
				states_[state].frames.push_back({ statement.tokens[index], statement.tokens[index + 1], duration });
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

bool Class::parse(ParsedStatement& class_statement)
{
	if (class_statement.tokens.size() < 2)
		return false;

	name_ = class_statement.tokens[1];

	for (unsigned a = 0; a < class_statement.tokens.size(); a++)
	{
		if (class_statement.tokens[a] == ":" && a < class_statement.tokens.size() - 1)
			inherits_class_ = class_statement.tokens[a + 1];
		else if (S_CMPNOCASE(class_statement.tokens[a], "native"))
			native_ = true;
		else if (S_CMPNOCASE(class_statement.tokens[a], "deprecated"))
			deprecated_ = true;
	}

	if (!parseClassBlock(class_statement.block))
		return false;

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

bool Class::extend(ParsedStatement& block)
{
	return parseClassBlock(block.block);
}

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
		parsed.push_back(Game::ThingType(name_, "ZScript", name_));
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
	def->loadProps(default_properties_);
}

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
		else if (statement.tokens[0].StartsWith("//$"))
		{
			if (statement.tokens.size() > 1)
				db_properties_.push_back({ statement.tokens[0].substr(3),  statement.tokens[1] });
			else
				db_properties_.push_back({ statement.tokens[0].substr(3), "true" });
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

bool Class::parseDefaults(vector<ParsedStatement>& defaults)
{
	for (auto& statement : defaults)
	{
		if (statement.tokens.empty())
			continue;

		// DB property comment
		if (statement.tokens[0].StartsWith("//$"))
		{
			if (statement.tokens.size() > 1)
				db_properties_.push_back({ statement.tokens[0].substr(3),  statement.tokens[1] });
			else
				db_properties_.push_back({ statement.tokens[0].substr(3), "true" });
			continue;
		}

		// Flags
		unsigned t = 0;
		unsigned count = statement.tokens.size();
		while (t < count)
		{
			if (statement.tokens[t] == "+")
				default_properties_[statement.tokens[++t]] = true;
			else if (statement.tokens[t] == "-")
				default_properties_[statement.tokens[++t]] = false;
			else
				break;

			t++;
		}

		// Name + Value
		// For now ignore anything after the first whitespace/special character
		// so stuff like arithmetic expressions or comma separated lists won't
		// really work properly yet
		if (t < count - 1)
			default_properties_[statement.tokens[t]] = statement.tokens[t + 1];

		// Name only
		else if (t < count)
			default_properties_[statement.tokens[t]] = true;
	}

	return true;
}





void parseBlocks(ArchiveEntry* entry, vector<ParsedStatement>& parsed)
{
	Tokenizer tz;
	tz.setSpecialCharacters(Tokenizer::DEFAULT_SPECIAL_CHARACTERS + "()+-[]&!?.");
	tz.enableDecorate(true);
	tz.openMem(entry->getMCData(), "ZScript");

	Log::info(2, S_FMT("Parsing ZScript entry \"%s\"", entry->getPath(true)));

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
							"Unable to find #included entry \"%s\" at line %d, skipping",
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
		ParsedStatement block;
		if (block.parse(tz))
			parsed.push_back(std::move(block));
	}
}

void Definitions::clear()
{
	classes_.clear();
	enumerators_.clear();
	variables_.clear();
	functions_.clear();
}

bool Definitions::parseZScript(ArchiveEntry* entry)
{
	// Parse into tree of expressions and blocks
	auto start = App::runTimer();
	vector<ParsedStatement> parsed;
	parseBlocks(entry, parsed);
	Log::debug(2, S_FMT("parseBlocks: %dms", App::runTimer() - start));
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
			bool found = false;
			for (auto& c : classes_)
				if (S_CMPNOCASE(c.name(), block.tokens[2]))
				{
					found = true;
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

	Log::debug(2, S_FMT("ZScript: %dms", App::runTimer() - start));

	return true;
}

bool Definitions::parseZScript(Archive* archive)
{
	// Get base decorate file
	Archive::SearchOptions opt;
	opt.match_name = "zscript";
	opt.ignore_ext = true;
	vector<ArchiveEntry*> zscript_enries = archive->findAll(opt);
	if (zscript_enries.empty())
		return false;

	Log::info(2, S_FMT("Parsing ZScript entries found in archive %s", archive->filename()));

	// Parse DECORATE entries
	bool ok = true;
	for (auto entry : zscript_enries)
		if (!parseZScript(entry))
			ok = false;

	return ok;
}

void Definitions::exportThingTypes(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed)
{
	for (auto& cdef : classes_)
		cdef.toThingType(types, parsed);
}


bool ParsedStatement::parse(Tokenizer& tz)
{
	auto start_line = tz.lineNo();

	// Tokens
	bool in_initializer = false;
	while (true)
	{
		// End of statement (;)
		if (tz.advIf(";"))
			return true;

		// DB comment
		if (tz.current().text.StartsWith("//$"))
		{
			tokens.push_back(tz.current().text);
			tokens.push_back(tz.getLine());
			return true;
		}

		if (tz.check("}"))
		{
			// End of array initializer
			if (in_initializer)
			{
				in_initializer = false;
				tokens.push_back("}");
				tz.adv();
				continue;
			}

			// End of statement
			return true;
		}

		if (tz.atEnd())
		{
			Log::debug(S_FMT("Failed parsing zscript statement beginning line %d", start_line));
			return false;
		}

		// Beginning of block
		if (tz.advIf("{"))
			break;

		// Array initializer: ... = { ... }
		if (tz.current().text == "=" && tz.peek().text == "{")
		{
			tokens.push_back("=");
			tokens.push_back("{");
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
		if (tz.advIf("}"))
			return true;

		if (tz.atEnd())
		{
			Log::debug(S_FMT("Failed parsing zscript statement beginning line %d", start_line));
			return false;
		}

		ParsedStatement statement;
		if (!statement.parse(tz))
			return false;
		if (!statement.tokens.empty())
			block.push_back(std::move(statement));
	}
}

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
	for (auto& block : block)
		block.dump(indent + 1);
}


// TEMP TESTING CONSOLE COMMANDS
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
