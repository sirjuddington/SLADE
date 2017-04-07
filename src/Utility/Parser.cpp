
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Parser.cpp
 * Description: Parser/Parse tree classes. Parses formatted text
 *              data and generates a tree of ParseTreeNodes containing
 *              the parsed data. Currently supports SLADE/DB/UDMF
 *              formatting style.
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
#include "Archive/Archive.h"
#include "Parser.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
wxRegEx re_int1("^[+-]?[0-9]+[0-9]*$", wxRE_DEFAULT|wxRE_NOSUB);
wxRegEx re_int2("^0[0-9]+$", wxRE_DEFAULT|wxRE_NOSUB);
wxRegEx re_int3("^0x[0-9A-Fa-f]+$", wxRE_DEFAULT|wxRE_NOSUB);
wxRegEx re_float("^[-+]?[0-9]*.?[0-9]+([eE][-+]?[0-9]+)?$", wxRE_DEFAULT|wxRE_NOSUB);


/*******************************************************************
 * PARSETREENODE CLASS FUNCTIONS
 *******************************************************************/

/* ParseTreeNode::ParseTreeNode
 * ParseTreeNode class constructor
 *******************************************************************/
ParseTreeNode::ParseTreeNode(ParseTreeNode* parent, Parser* parser, ArchiveTreeNode* archive_dir) :
	STreeNode{ parent },
	parser{ parser },
	archive_dir{ archive_dir }
{
	allowDup(true);
}

/* ParseTreeNode::~ParseTreeNode
 * ParseTreeNode class destructor
 *******************************************************************/
ParseTreeNode::~ParseTreeNode()
{
}

Property ParseTreeNode::getValue(unsigned index)
{
	// Check index
	if (index >= values.size())
		return Property(false);

	return values[index];
}

/* ParseTreeNode::getStringValue
 * Returns the node's value at [index] as a string. If [index] is
 * out of range, returns an empty string
 *******************************************************************/
string ParseTreeNode::getStringValue(unsigned index)
{
	// Check index
	if (index >= values.size())
		return wxEmptyString;

	return values[index].getStringValue();
}

/* ParseTreeNode::getStringValue
 * Returns the node's value at [index] as an integer. If [index] is
 * out of range, returns 0
 *******************************************************************/
int ParseTreeNode::getIntValue(unsigned index)
{
	// Check index
	if (index >= values.size())
		return 0;

	return (int)values[index];
}

/* ParseTreeNode::getStringValue
 * Returns the node's value at [index] as a boolean. If [index] is
 * out of range, returns false
 *******************************************************************/
bool ParseTreeNode::getBoolValue(unsigned index)
{
	// Check index
	if (index >= values.size())
		return false;

	return (bool)values[index];
}

/* ParseTreeNode::getStringValue
 * Returns the node's value at [index] as a float. If [index] is
 * out of range, returns 0.0f
 *******************************************************************/
double ParseTreeNode::getFloatValue(unsigned index)
{
	// Check index
	if (index >= values.size())
		return 0.0f;

	return (double)values[index];
}

/* ParseTreeNode::parse
 * Parses formatted text data. Current valid formatting is:
 * (type) child = value;
 * (type) child = value1, value2, ...;
 * (type) child = { value1, value2, ... }
 * (type) child { grandchild = value; etc... }
 * (type) child : inherited { ... }
 * All values are read as strings, but can be retrieved as string,
 * int, bool or float.
 *******************************************************************/
bool ParseTreeNode::parse(Tokenizer& tz)
{
	// Get first token
	string token = tz.getToken();

	// Keep parsing until final } is reached (or end of file)
	while (!(S_CMP(token, "}")) && !token.IsEmpty())
	{
		// Check for preprocessor stuff
		if (parser)
		{
			// #define
			if (S_CMPNOCASE(token, "#define"))
			{
				parser->define(tz.getToken());
				token = tz.getToken();
				continue;
			}

			// #if(n)def
			if (S_CMPNOCASE(token, "#ifdef") || S_CMPNOCASE(token, "#ifndef"))
			{
				bool test = true;
				if (S_CMPNOCASE(token, "#ifndef"))
					test = false;
				string define = tz.getToken();
				if (parser->defined(define) == test)
				{
					// Continue
					token = tz.getToken();
					continue;
				}

				// Skip section
				int skip = 0;
				while (true)
				{
					token = tz.getToken();
					if (S_CMPNOCASE(token, "#endif"))
						skip--;
					else if (S_CMPNOCASE(token, "#ifdef"))
						skip++;
					else if (S_CMPNOCASE(token, "#ifndef"))
						skip++;

					if (skip < 0)
						break;
					if (token.IsEmpty())
					{
						LOG_MESSAGE(1, "Error: found end of file within #if(n)def block");
						break;
					}
				}

				continue;
			}

			// #include
			if (S_CMPNOCASE(token, "#include"))
			{
				// Include entry at the given path if we have an archive dir set
				if (archive_dir)
				{
					// Get entry to include
					auto inc_path = tz.getToken();
					auto archive = archive_dir->getArchive();
					auto inc_entry = archive->entryAtPath(archive_dir->getPath() + inc_path);
					if (!inc_entry) // Try absolute path
						inc_entry = archive->entryAtPath(inc_path);

					if (inc_entry)
					{
						// Save the current dir and set it to the included entry's dir
						auto orig_dir = archive_dir;
						archive_dir = inc_entry->getParentDir();

						// Parse text in the entry
						Tokenizer inc_tz;
						inc_tz.openMem(&inc_entry->getMCData(), inc_entry->getName());
						bool ok = parse(inc_tz);

						// Reset dir and abort if parsing failed
						archive_dir = orig_dir;
						if (!ok)
							return false;
					}
					else
						LOG_MESSAGE(2, "Parsing error: Include entry %s not found", inc_path);
				}
				else
					tz.skipToken();	// Skip include path

				tz.getToken(&token);
				continue;
			}

			// #endif (ignore)
			if (S_CMPNOCASE(token, "#endif"))
			{
				token = tz.getToken();
				continue;
			}
		}

		// If it's a special character (ie not a valid name), parsing fails
		if (tz.isSpecialCharacter(token.at(0)))
		{
			LOG_MESSAGE(1, "Parsing error: Unexpected special character '%s' in %s (line %d)", token, tz.getName(), tz.lineNo());
			return false;
		}

		// So we have either a node or property name
		string name = token;
		if (name.IsEmpty())
		{
			LOG_MESSAGE(1, "Parsing error: Unexpected empty string in %s (line %d)", tz.getName(), tz.lineNo());
			return false;
		}

		// Check next token to determine what we're doing
		string next = tz.peekToken();

		// Check for type+name pair
		string type = "";
		if (next != "=" && next != "{" && next != ";" && next != ":")
		{
			type = name;
			name = tz.getToken();
			next = tz.peekToken();

			if (name.IsEmpty())
			{
				LOG_MESSAGE(1, "Parsing error: Unexpected empty string in %s (line %d)", tz.getName(), tz.lineNo());
				return false;
			}
		}

		// Assignment
		if (S_CMP(next, "="))
		{
			// Skip =
			tz.skipToken();

			// Create item node
			ParseTreeNode* child = (ParseTreeNode*)addChild(name);
			child->type = type;

			// Check type of assignment list
			token = tz.getToken();
			string list_end = ";";
			if (token == "{" && !tz.quotedString())
			{
				list_end = "}";
				token = tz.getToken();
			}

			// Parse until ; or }
			while (1)
			{
				// Check for list end
				if (S_CMP(token, list_end) && !tz.quotedString())
					break;

				// Setup value
				Property value;

				// Detect value type
				if (tz.quotedString())					// Quoted string
					value = token;
				else if (S_CMPNOCASE(token, "true"))	// Boolean (true)
					value = true;
				else if (S_CMPNOCASE(token, "false"))	// Boolean (false)
					value = false;
				else if (re_int1.Matches(token) ||		// Integer
				         re_int2.Matches(token))
				{
					long val;
					token.ToLong(&val);
					value = (int)val;
				}
				else if (re_int3.Matches(token))  		// Hex (0xXXXXXX)
				{
					long val;
					token.ToLong(&val, 0);
					value = (int)val;
					//LOG_MESSAGE(1, "%s: %s is hex %d", name, token, value.getIntValue());
				}
				else if (re_float.Matches(token))  		// Floating point
				{
					double val;
					token.ToDouble(&val);
					value = (double)val;
					//LOG_MESSAGE(3, "%s: %s is float %1.3f", name, token, val);
				}
				else									// Unknown, just treat as string
					value = token;

				// Add value
				child->values.push_back(value);

				// Check for ,
				if (S_CMP(tz.peekToken(), ","))
					tz.skipToken();	// Skip it
				else if (!(S_CMP(tz.peekToken(), list_end)))
				{
					string t = tz.getToken();
					string n = tz.getName();
					LOG_MESSAGE(1, "Parsing error: Expected \",\" or \"%s\", got \"%s\" in %s (line %d)", list_end, t, n, tz.lineNo());
					return false;
				}

				token = tz.getToken();
			}
		}

		// Child node
		else if (S_CMP(next, "{"))
		{
			// Add child node
			ParseTreeNode* child = (ParseTreeNode*)addChild(name);
			child->type = type;

			// Skip {
			tz.skipToken();

			// Parse child node
			if (!child->parse(tz))
				return false;
		}

		// Child node (with no values/children)
		else if (S_CMP(next, ";"))
		{
			// Add child node
			ParseTreeNode* child = (ParseTreeNode*)addChild(name);
			child->type = type;

			// Skip ;
			tz.skipToken();
		}

		// Child node + inheritance
		else if (S_CMP(next, ":"))
		{
			// Skip :
			tz.skipToken();

			// Read inherited name
			string inherit = tz.getToken();

			// Check for opening brace
			if (tz.checkToken("{"))
			{
				// Add child node
				ParseTreeNode* child = (ParseTreeNode*)addChild(name);
				child->type = type;

				// Set its inheritance
				child->inherit = inherit;

				// Parse child node
				if (!child->parse(tz))
					return false;
			}
		}

		// Unexpected token
		else
		{
			LOG_MESSAGE(1, "Parsing error: \"%s\" unexpected in %s (line %d)", next, tz.getName(), tz.lineNo());
			return false;
		}

		// Continue parsing
		token = tz.getToken();
	}

	return true;
}


/*******************************************************************
 * PARSER CLASS FUNCTIONS
 *******************************************************************/

/* Parser::Parser
 * Parser class constructor
 *******************************************************************/
Parser::Parser(ArchiveTreeNode* dir_root) :
	archive_dir_root { dir_root }
{
	// Create parse tree root node
	pt_root = std::make_unique<ParseTreeNode>(nullptr, this, archive_dir_root);
}

/* Parser::~Parser
 * Parser class destructor
 *******************************************************************/
Parser::~Parser()
{
}

/* Parser::parseText
 * Parses the given text data to build a tree of ParseTreeNodes.
 * Example:
 * 	base {
 * 		child1 = value1;
 * 		child2 = value2, value3, value4;
 * 		child3 {
 * 			grandchild1 = value5;
 * 			grandchild2 = value6;
 * 		}
 * 		child4 {
 * 			grandchild3 = value7, value8;
 * 		}
 * 	}
 *
 * will generate this tree (represented in xml-like format, node names within <>):
 * 	<root>
 * 		<base>
 * 			<child1>value1</child1>
 * 			<child2>value2, value3, value4</child2>
 * 			<child3>
 * 				<grandchild1>value5</grandchild1>
 * 				<grandchild2>value6</grandchild2>
 * 			</child3>
 * 			<child4>
 * 				<grandchild3>value7, value8</grandchild3>
 * 			</child4>
 * 		</base>
 * 	</root>
 *******************************************************************/
bool Parser::parseText(MemChunk& mc, string source, bool debug)
{
	Tokenizer tz;
	if (debug) tz.enableDebug(debug);

	// Open the given text data
	if (!tz.openMem(&mc, source))
	{
		LOG_MESSAGE(1, "Unable to open text data for parsing");
		return false;
	}

	// Do parsing
	return pt_root->parse(tz);
}
bool Parser::parseText(string& text, string source, bool debug)
{
	Tokenizer tz;
	if (debug) tz.enableDebug(debug);

	// Open the given text data
	if (!tz.openString(text, 0, 0, source))
	{
		LOG_MESSAGE(1, "Unable to open text data for parsing");
		return false;
	}

	// Do parsing
	return pt_root->parse(tz);
}

/* Parser::define
 * Adds [def] to the defines list
 *******************************************************************/
void Parser::define(string def)
{
	defines.push_back(def);
}

/* Parser::defined
 * Returns true if [def] is in the defines list
 *******************************************************************/
bool Parser::defined(string def)
{
	for (unsigned a = 0; a < defines.size(); a++)
	{
		if (S_CMPNOCASE(defines[a], def))
			return true;
	}

	return false;
}


#if 0
// Console test commands
CONSOLE_COMMAND (testparse, 0) {
	string test = "root { item1 = value1; item2 = value1, value2; child1 { item1 = value1, value2, value3; item2 = value4; } child2 : child1 { item1 = value1; child3 { item1 = value1, value2; } } }";
	Parser tp;
	MemChunk mc((const uint8_t*)CHR(test), test.Length());
	tp.parseText(mc);
}

#include "App.h"
CONSOLE_COMMAND (testregex, 2, false)
{
	wxRegEx re(args[0]);
	if (re.Matches(args[1]))
		Log::console("Matches");
	else
		Log::console("Doesn't match");
}
#endif
