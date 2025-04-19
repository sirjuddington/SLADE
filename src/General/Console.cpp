
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Console.cpp
// Description: The SLADE Console implementation
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
#include "Console.h"
#include "App.h"
#include "General/CVar.h"
#include "MainEditor/MainEditor.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Console Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Adds a ConsoleCommand to the Console
// -----------------------------------------------------------------------------
void Console::addCommand(ConsoleCommand& c)
{
	// Add the command to the list
	commands_.push_back(c);

	// Sort the commands alphabetically by name (so the cmdlist command output looks nice :P)
	sort(commands_.begin(), commands_.end());
}

// -----------------------------------------------------------------------------
// Attempts to execute the command line given
// -----------------------------------------------------------------------------
void Console::execute(string_view command)
{
	log::info("> {}", command);

	// Don't bother doing anything else with an empty command
	if (command.empty())
		return;

	// Add the command to the log
	cmd_log_.emplace(cmd_log_.begin(), command);

	// Tokenize the command string
	Tokenizer tz;
	tz.openString(command);

	// Get the command name
	auto cmd_name = tz.current().text;

	// Get all args
	vector<string> args;
	while (!tz.atEnd())
		args.push_back(tz.next().text);

	// Check that it is a valid command
	for (auto& cmd : commands_)
	{
		// Found it, execute and return
		if (cmd.name() == cmd_name)
		{
			cmd.execute(args);
			return;
		}
	}

	// Check if it is a cvar
	auto cvar = CVar::get(cmd_name);
	if (cvar)
	{
		// Arg(s) given, set cvar value
		if (!args.empty())
		{
			if (cvar->type == CVar::Type::Boolean)
			{
				if (args[0] == "0" || args[0] == "false")
					*dynamic_cast<CBoolCVar*>(cvar) = false;
				else
					*dynamic_cast<CBoolCVar*>(cvar) = true;
			}
			else if (cvar->type == CVar::Type::Integer)
				*dynamic_cast<CIntCVar*>(cvar) = strutil::asInt(args[0]);
			else if (cvar->type == CVar::Type::Float)
				*dynamic_cast<CFloatCVar*>(cvar) = strutil::asFloat(args[0]);
			else if (cvar->type == CVar::Type::String)
				*dynamic_cast<CStringCVar*>(cvar) = args[0];
		}

		// Print cvar value
		string value;
		if (cvar->type == CVar::Type::Boolean)
		{
			if (cvar->getValue().Bool)
				value = "true";
			else
				value = "false";
		}
		else if (cvar->type == CVar::Type::Integer)
			value = fmt::format("{}", cvar->getValue().Int);
		else if (cvar->type == CVar::Type::Float)
			value = fmt::format("{:1.4f}", cvar->getValue().Float);
		else
			value = ((CStringCVar*)cvar)->value;

		log::console(fmt::format(R"("{}" = "{}")", cmd_name, value));

		return;
	}

	// Toggle global debug mode
	if (cmd_name == "debug")
	{
		global::debug = !global::debug;
		if (global::debug)
			log::console("Debugging stuff enabled");
		else
			log::console("Debugging stuff disabled");

		return;
	}

	// Command not found
	log::console(fmt::format("Unknown command: \"{}\"", cmd_name));
}

// -----------------------------------------------------------------------------
// Returns the last command sent to the console
// -----------------------------------------------------------------------------
string Console::lastCommand()
{
	return !cmd_log_.empty() ? cmd_log_.back() : "";
}

// -----------------------------------------------------------------------------
// Returns the previous command at [index] from the last entered
// (ie, index=0 will be the directly previous command)
// -----------------------------------------------------------------------------
string Console::prevCommand(int index)
{
	// Check index
	if (index < 0 || (unsigned)index >= cmd_log_.size())
		return "";

	return cmd_log_[index];
}

// -----------------------------------------------------------------------------
// Returns the ConsoleCommand at the specified index
// -----------------------------------------------------------------------------
ConsoleCommand& Console::command(size_t index)
{
	if (index < commands_.size())
		return commands_[index];
	else
		return commands_[0]; // Return first console command on invalid index
}


// -----------------------------------------------------------------------------
//
// ConsoleCommand Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ConsoleCommand class constructor
// -----------------------------------------------------------------------------
ConsoleCommand::ConsoleCommand(
	string_view name,
	void (*command_func)(const vector<string>&),
	int  min_args = 0,
	bool show_in_list)
{
	// Init variables
	name_         = name;
	command_func_ = command_func;
	min_args_     = min_args;
	show_in_list_ = show_in_list;

	// Add this command to the console
	app::console()->addCommand(*this);
}

// -----------------------------------------------------------------------------
// Executes the console command
// -----------------------------------------------------------------------------
void ConsoleCommand::execute(const vector<string>& args) const
{
	// Only execute if we have the minimum args specified
	if (args.size() >= min_args_)
		command_func_(args);
	else
		log::console(fmt::format("Missing command arguments, type \"cmdhelp {}\" for more information", name_));
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// A simple command to print the first given argument to the console.
// Subsequent arguments are ignored.
// -----------------------------------------------------------------------------
CONSOLE_COMMAND(echo, 1, true)
{
	log::console(args[0]);
}

// -----------------------------------------------------------------------------
// Lists all valid console commands
// -----------------------------------------------------------------------------
CONSOLE_COMMAND(cmdlist, 0, true)
{
	log::console(fmt::format("{} Valid Commands:", app::console()->numCommands()));

	for (int a = 0; a < app::console()->numCommands(); a++)
	{
		auto& cmd = app::console()->command(a);
		if (cmd.showInList() || global::debug)
			log::console(fmt::format("\"{}\" ({} args)", cmd.name(), cmd.minArgs()));
	}
}

// -----------------------------------------------------------------------------
// Lists all cvars
// -----------------------------------------------------------------------------
CONSOLE_COMMAND(cvarlist, 0, true)
{
	// Get sorted list of cvars
	vector<string> list;
	CVar::putList(list);
	sort(list.begin(), list.end());

	log::console(fmt::format("{} CVars:", list.size()));

	// Write list to console
	for (const auto& a : list)
		log::console(a);
}

// -----------------------------------------------------------------------------
// Opens the wiki page for a console command
// -----------------------------------------------------------------------------
CONSOLE_COMMAND(cmdhelp, 1, true)
{
	// Check command exists
	for (int a = 0; a < app::console()->numCommands(); a++)
	{
		if (strutil::equalCI(app::console()->command(a).name(), args[0]))
		{
#ifdef USE_WEBVIEW_STARTPAGE
			maineditor::openDocs(fmt::format("{}-Console-Command", args[0]));
#else
			auto url = fmt::format("https://github.com/sirjuddington/SLADE/wiki/{}-Console-Command", args[0]);
			wxLaunchDefaultBrowser(wxString::FromUTF8(url));
#endif
			return;
		}
	}

	// No command found
	log::console(fmt::format("No command \"{}\" exists", args[0]));
}





CONSOLE_COMMAND(testmatch, 2, false)
{
	bool match = strutil::matches(args[1], args[0]);
	if (match)
		log::console("Match");
	else
		log::console("No Match");
}

CONSOLE_COMMAND(testmatchci, 2, false)
{
	bool match = strutil::matchesCI(args[1], args[0]);
	if (match)
		log::console("Match");
	else
		log::console("No Match");
}


// Converts DB-style ACS function definitions to SLADE-style:
// from Function = "Function(Arg1, Arg2, Arg3)";
// to Function = "Arg1", "Arg2", "Arg3";
// Reads from a text file and outputs the result to the console
#if 0
#include "Utility/Parser.h"

CONSOLE_COMMAND (langfuncsplit, 1)
{
	MemChunk mc;
	if (!mc.importFile(args[0]))
		return;

	Parser p;
	if (!p.parseText(mc))
		return;

	ParseTreeNode* root = p.parseTreeRoot();
	for (unsigned a = 0; a < root->nChildren(); a++)
	{
		ParseTreeNode* node = (ParseTreeNode*)root->getChild(a);

		// Get function definition line (eg "Function(arg1, arg2, arg3)")
		string funcline = node->getStringValue();

		// Remove brackets
		funcline.Replace("(", " ");
		funcline.Replace(")", " ");

		// Parse definition
		vector<string> args;
		Tokenizer tz2;
		tz2.setSpecialCharacters(",;");
		tz2.openString(funcline);
		tz2.getToken();	// Skip function name
		string token = tz2.getToken();
		while (token != "")
		{
			if (token != ",")
				args.push_back(token);
			token = tz2.getToken();
		}

		// Print to console
		string lmsg = node->getName();
		if (args.size() > 0)
		{
			lmsg += " = ";
			for (unsigned arg = 0; arg < args.size(); arg++)
			{
				if (arg > 0)
					lmsg += ", ";
				lmsg += "\"" + args[arg] + "\"";
			}
		}
		lmsg += ";";
		log::console(lmsg);
	}
}

#endif
