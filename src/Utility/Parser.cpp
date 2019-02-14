
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Parser.cpp
// Description: Parser/Parse tree classes. Parses formatted text data and
//              generates a tree of ParseTreeNodes containing the parsed data.
//              Currently supports SLADE/DB/UDMF formatting style.
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
#include "Parser.h"
#include "Archive/Archive.h"
#include "Utility/Tokenizer.h"
#include "StringUtils.h"


// -----------------------------------------------------------------------------
//
// ParseTreeNode Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ParseTreeNode class constructor
// -----------------------------------------------------------------------------
ParseTreeNode::ParseTreeNode(
	ParseTreeNode*   parent,
	Parser*          parser,
	ArchiveTreeNode* archive_dir,
	std::string_view type) :
	STreeNode{ parent },
	type_{ type },
	parser_{ parser },
	archive_dir_{ archive_dir }
{
	allowDup(true);
}

// -----------------------------------------------------------------------------
// Returns true if the node's name matches [name] (case-insensitive)
// -----------------------------------------------------------------------------
bool ParseTreeNode::nameIsCI(std::string_view name) const
{
	return StrUtil::equalCI(name_, name);
}

// -----------------------------------------------------------------------------
// Returns the node's value at [index] as a Property.
// If [index] is out of range, returns a false boolean Property
// -----------------------------------------------------------------------------
Property ParseTreeNode::value(unsigned index)
{
	// Check index
	if (index >= values_.size())
		return Property(false);

	return values_[index];
}

// -----------------------------------------------------------------------------
// Returns the node's value at [index] as a string.
// If [index] is out of range, returns an empty string
// -----------------------------------------------------------------------------
std::string ParseTreeNode::stringValue(unsigned index)
{
	// Check index
	if (index >= values_.size())
		return {};

	return values_[index].stringValue().ToStdString();
}

// -----------------------------------------------------------------------------
// Returns the node's values as a string vector.
// -----------------------------------------------------------------------------
vector<std::string> ParseTreeNode::stringValues()
{
	vector<std::string> string_values;
	for (auto& value : values_)
		string_values.push_back(value.stringValue().ToStdString());
	return string_values;
}

// -----------------------------------------------------------------------------
// Returns the node's value at [index] as an integer.
// If [index] is out of range, returns 0
// -----------------------------------------------------------------------------
int ParseTreeNode::intValue(unsigned index)
{
	// Check index
	if (index >= values_.size())
		return 0;

	return (int)values_[index];
}

// -----------------------------------------------------------------------------
// Returns the node's value at [index] as a boolean.
// If [index] is out of range, returns false
// -----------------------------------------------------------------------------
bool ParseTreeNode::boolValue(unsigned index)
{
	// Check index
	if (index >= values_.size())
		return false;

	return (bool)values_[index];
}

// -----------------------------------------------------------------------------
// Returns the node's value at [index] as a float.
// If [index] is out of range, returns 0.0f
// -----------------------------------------------------------------------------
double ParseTreeNode::floatValue(unsigned index)
{
	// Check index
	if (index >= values_.size())
		return 0.0f;

	return (double)values_[index];
}

// -----------------------------------------------------------------------------
// Adds a child ParseTreeNode of [name] and [type]
// -----------------------------------------------------------------------------
ParseTreeNode* ParseTreeNode::addChildPTN(std::string_view name, std::string_view type)
{
	auto node   = dynamic_cast<ParseTreeNode*>(addChild(name));
	node->type_ = type;
	return node;
}

// -----------------------------------------------------------------------------
// Writes an error log message [error], showing the source and current line
// from tokenizer [tz]
// -----------------------------------------------------------------------------
void ParseTreeNode::logError(const Tokenizer& tz, std::string_view error) const
{
	Log::error("Parse Error in {} (Line {}): {}\n", tz.source(), tz.current().line_no, error);
}

// -----------------------------------------------------------------------------
// Parses a preprocessor directive at [tz]'s current token
// -----------------------------------------------------------------------------
bool ParseTreeNode::parsePreprocessor(Tokenizer& tz)
{
	// Log::debug(wxString::Format("Preprocessor %s", CHR(tz.current().text)));

	// #define
	if (tz.current() == "#define")
		parser_->define(tz.next().text);

	// #if(n)def
	else if (tz.current() == "#ifdef" || tz.current() == "#ifndef")
	{
		// Continue if condition succeeds
		bool test = true;
		if (tz.current() == "#ifndef")
			test = false;
		auto define = tz.next().text;
		if (parser_->defined(define) == test)
			return true;

		// Failed condition, skip section
		int skip = 0;
		while (true)
		{
			auto& token = tz.next();
			if (token == "#endif")
				skip--;
			else if (token == "#ifdef")
				skip++;
			else if (token == "#ifndef")
				skip++;
			// TODO: #else

			if (skip < 0)
				break;
		}
	}

	// #include
	else if (tz.current() == "#include")
	{
		// Include entry at the given path if we have an archive dir set
		if (archive_dir_)
		{
			// Get entry to include
			auto inc_path  = tz.next().text;
			auto archive   = archive_dir_->archive();
			auto inc_entry = archive->entryAtPath(archive_dir_->path() + inc_path);
			if (!inc_entry) // Try absolute path
				inc_entry = archive->entryAtPath(inc_path);

			// Log::debug(wxString::Format("Include %s", CHR(inc_path)));

			if (inc_entry)
			{
				// Save the current dir and set it to the included entry's dir
				auto orig_dir = archive_dir_;
				archive_dir_  = inc_entry->parentDir();

				// Parse text in the entry
				Tokenizer inc_tz;
				inc_tz.openMem(inc_entry->data(), inc_entry->name());
				bool ok = parse(inc_tz);

				// Reset dir and abort if parsing failed
				archive_dir_ = orig_dir;
				if (!ok)
					return false;
			}
			else
				logError(tz, fmt::format("Include entry {} not found", inc_path));
		}
		else
			tz.adv(); // Skip include path
	}

	// #endif (ignore)
	else if (tz.current() == "#endif")
		return true;

	// TODO: #else

	// Unrecognised
	else
		logError(tz, fmt::format("Unrecognised preprocessor directive \"{}\"", tz.current().text));

	return true;
}

// -----------------------------------------------------------------------------
// Parses an assignment operation at [tz]'s current token to [child]
// -----------------------------------------------------------------------------
bool ParseTreeNode::parseAssignment(Tokenizer& tz, ParseTreeNode* child) const
{
	// Check type of assignment list
	char list_end = ';';
	if (tz.current() == '{' && !tz.current().quoted_string)
	{
		list_end = '}';
		tz.adv();
	}

	// Parse until ; or }
	while (true)
	{
		auto& token = tz.current();

		// Check for list end
		if (token == list_end && !token.quoted_string)
			break;

		// Setup value
		Property value;

		// Detect value type
		if (token.quoted_string) // Quoted string
			value = token.text;
		else if (token == "true") // Boolean (true)
			value = true;
		else if (token == "false") // Boolean (false)
			value = false;
		else if (token.isInteger()) // Integer
			value = token.asInt();
		else if (token.isHex()) // Hex (0xXXXXXX)
			value = token.asInt();
		else if (token.isFloat()) // Floating point
			value = token.asFloat();
		else // Unknown, just treat as string
			value = token.text;

		// Add value
		child->values_.push_back(value);

		// Check for ,
		if (tz.peek() == ',')
			tz.adv(); // Skip it
		else if (tz.peek() != list_end)
		{
			logError(tz, fmt::format(R"(Expected "," or "{}", got "{}")", list_end, tz.peek().text));
			return false;
		}

		tz.adv();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Parses formatted text data. Current valid formatting is:
// (type) child = value;
// (type) child = value1, value2, ...;
// (type) child = { value1, value2, ... }
// (type) child { grandchild = value; etc... }
// (type) child : inherited { ... }
//
// All values are read as strings, but can be retrieved as string, int, bool
// or float.
// -----------------------------------------------------------------------------
bool ParseTreeNode::parse(Tokenizer& tz)
{
	// Keep parsing until final } is reached (or end of file)
	std::string name, type;
	while (!tz.atEnd() && tz.current() != '}')
	{
		// Check for preprocessor stuff
		if (parser_ && tz.current()[0] == '#')
		{
			if (!parsePreprocessor(tz))
				return false;

			tz.advToNextLine();
			continue;
		}

		// If it's a special character (ie not a valid name), parsing fails
		if (tz.isSpecialCharacter(tz.current().text[0]))
		{
			logError(tz, fmt::format("Unexpected special character '{}'", tz.current().text));
			return false;
		}

		// So we have either a node or property name
		name = tz.current().text;
		type.clear();
		if (name.empty())
		{
			logError(tz, "Unexpected empty string");
			return false;
		}

		// Check for type+name pair
		if (tz.peek() != '=' && tz.peek() != '{' && tz.peek() != ';' && tz.peek() != ':')
		{
			type = name;
			name = tz.next().text;

			if (name.empty())
			{
				logError(tz, "Unexpected empty string");
				return false;
			}
		}

		// Log::debug(wxString::Format("%s \"%s\", op %s", CHR(type), CHR(name), CHR(tz.current().text)));

		// Assignment
		if (tz.advIfNext('=', 2))
		{
			if (!parseAssignment(tz, addChildPTN(name, type)))
				return false;
		}

		// Child node
		else if (tz.advIfNext('{', 2))
		{
			// Parse child node
			if (!addChildPTN(name, type)->parse(tz))
				return false;
		}

		// Child node (with no values/children)
		else if (tz.advIfNext(';', 2))
		{
			// Add child node
			addChildPTN(name, type);
			continue;
		}

		// Child node + inheritance
		else if (tz.advIfNext(':', 2))
		{
			// Check for opening brace
			if (tz.checkNext('{'))
			{
				// Add child node
				auto child      = addChildPTN(name, type);
				child->inherit_ = tz.current().text;

				// Skip {
				tz.adv(2);

				// Parse child node
				if (!child->parse(tz))
					return false;
			}
			else if (tz.checkNext(';')) // Empty child node
			{
				// Add child node
				auto child      = addChildPTN(name, type);
				child->inherit_ = tz.current().text;

				// Skip ;
				tz.adv(2);

				continue;
			}
			else
			{
				logError(tz, fmt::format(R"(Expecting "{{" or ";", got "{}")", tz.next().text));
				return false;
			}
		}

		// Unexpected token
		else
		{
			logError(tz, fmt::format("Unexpected token \"{}\"", tz.next().text));
			return false;
		}

		// Continue parsing
		tz.adv();
	}

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Writes this node and its children as text to [out], indented by [indent] tab
// characters. See the Parser::parseText description below for an example of
// the output.
//
// Strings are written within enclosing "" in the following cases:
// Node names:     only if the name contains a space
// String values:  always
// Node types:     never
// Node 'inherit': never
// -----------------------------------------------------------------------------
void ParseTreeNode::write(std::string& out, int indent) const
{
	// Indentation
	std::string tabs;
	for (int a = 0; a < indent; a++)
		tabs += "\t";

	// Type
	out += tabs;
	if (!type_.empty())
		out += type_ + " ";

	// Name
	if (StrUtil::contains(name_, ' ') || name_.empty())
		out += fmt::format("\"{}\"", name_);
	else
		out += fmt::format("{}", name_);

	// Inherit
	if (!inherit_.empty())
		out += " : " + inherit_;

	// Leaf node - write value(s)
	if (children_.empty())
	{
		out += " = ";

		bool first = true;
		for (auto& value : values_)
		{
			if (!first)
				out += ", ";
			first = false;

			switch (value.type())
			{
			case Property::Type::Boolean:
			case Property::Type::Flag: out += value.boolValue() ? "true" : "false"; break;
			case Property::Type::Int: out += fmt::format("{}", value.intValue()); break;
			case Property::Type::Float: out += fmt::format("{:1.3f}", value.floatValue()); break;
			case Property::Type::UInt: out += fmt::format("{}", value.unsignedValue()); break;
			default: out += fmt::format("\"{}\"", value.stringValue().ToStdString()); break;
			}
		}

		out += ";\n";
	}

	// Otherwise write child nodes
	else
	{
		// Opening brace
		out += "\n" + tabs + "{\n";

		for (auto node : children_)
			dynamic_cast<ParseTreeNode*>(node)->write(out, indent + 1);

		// Closing brace
		out += tabs + "}\n";
	}
}


// -----------------------------------------------------------------------------
//
// Parser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Parser class constructor
// -----------------------------------------------------------------------------
Parser::Parser(ArchiveTreeNode* dir_root) : archive_dir_root_{ dir_root }
{
	// Create parse tree root node
	pt_root_ = std::make_unique<ParseTreeNode>(nullptr, this, archive_dir_root_);
}

// -----------------------------------------------------------------------------
// Parses the given text data to build a tree of ParseTreeNodes.
// Example:
// base
// {
//     child1 = value1;
//     child2 = value2, value3, value4;
//     child3
//     {
// 	       grandchild1 = value5;
// 	       grandchild2 = value6;
//     }
//     child4
//     {
// 	       grandchild3 = value7, value8;
//     }
// }
//
// will generate this tree
// (represented in xml-like format, node names within <>):
// 	<root>
// 		<base>
// 			<child1>value1</child1>
// 			<child2>value2, value3, value4</child2>
// 			<child3>
// 				<grandchild1>value5</grandchild1>
// 				<grandchild2>value6</grandchild2>
// 			</child3>
// 			<child4>
// 				<grandchild3>value7, value8</grandchild3>
// 			</child4>
// 		</base>
// 	</root>
// -----------------------------------------------------------------------------
bool Parser::parseText(MemChunk& mc, const std::string& source) const
{
	Tokenizer tz;

	// Open the given text data
	tz.setReadLowerCase(!case_sensitive_);
	if (!tz.openMem(mc, source))
	{
		Log::error("Unable to open text data for parsing");
		return false;
	}

	// Do parsing
	return pt_root_->parse(tz);
}
bool Parser::parseText(const std::string& text, const std::string& source) const
{
	Tokenizer tz;

	// Open the given text data
	tz.setReadLowerCase(!case_sensitive_);
	if (!tz.openString(text, 0, 0, source))
	{
		Log::error("Unable to open text data for parsing");
		return false;
	}

	// Do parsing
	return pt_root_->parse(tz);
}

// -----------------------------------------------------------------------------
// Adds [def] to the #defines list
// -----------------------------------------------------------------------------
void Parser::define(std::string_view def)
{
	defines_.emplace_back(def);
}

// -----------------------------------------------------------------------------
// Returns true if [def] has been previously #defined (case-insensitive)
// -----------------------------------------------------------------------------
bool Parser::defined(std::string_view def)
{
	for (const auto& defined_str : defines_)
		if (StrUtil::equalCI(defined_str, def))
			return true;

	return false;
}
