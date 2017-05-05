
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Console.cpp
 * Description: The SLADE Console implementation
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
#include "App.h"
#include "Console.h"
#include "Utility/Tokenizer.h"
#include "General/CVar.h"
#include "MainEditor/MainEditor.h"


/*******************************************************************
 * CONSOLE CLASS FUNCTIONS
 *******************************************************************/

/* Console::Console
 * Console class constructor
 *******************************************************************/
Console::Console()
{
}

/* Console::!Console
 * Console class destructor
 *******************************************************************/
Console::~Console()
{
}

/* Console::addCommand
 * Adds a ConsoleCommand to the Console
 *******************************************************************/
void Console::addCommand(ConsoleCommand& c)
{
	// Add the command to the list
	commands.push_back(c);

	// Sort the commands alphabetically by name (so the cmdlist command output looks nice :P)
	sort(commands.begin(), commands.end());
}

/* Console::execute
 * Attempts to execute the command line given
 *******************************************************************/
void Console::execute(string command)
{
	LOG_MESSAGE(1, "> %s", command);

	// Don't bother doing anything else with an empty command
	if (command.size() == 0)
		return;

	// Add the command to the log
	cmd_log.insert(cmd_log.begin(), command);

	// Tokenize the command string
	Tokenizer tz;
	tz.openString(command);

	// Get the command name
	string cmd_name = tz.getToken();

	// Get all args
	string arg = tz.getToken();
	vector<string> args;
	while (arg != "")
	{
		args.push_back(arg);
		arg = tz.getToken();
	}

	// Check that it is a valid command
	for (size_t a = 0; a < commands.size(); a++)
	{
		// Found it, execute and return
		if (commands[a].getName() == cmd_name)
		{
			commands[a].execute(args);
			return;
		}
	}

	// Check if it is a cvar
	CVar* cvar = get_cvar(cmd_name);
	if (cvar)
	{
		// Arg(s) given, set cvar value
		if (args.size() > 0)
		{
			if (cvar->type == CVAR_BOOLEAN)
			{
				if (args[0] == "0" || args[0] == "false")
					*((CBoolCVar*)cvar) = false;
				else
					*((CBoolCVar*)cvar) = true;
			}
			else if (cvar->type == CVAR_INTEGER)
				*((CIntCVar*)cvar) = atoi(CHR(args[0]));
			else if (cvar->type == CVAR_FLOAT)
				*((CFloatCVar*)cvar) = (float)atof(CHR(args[0]));
			else if (cvar->type == CVAR_STRING)
				*((CStringCVar*)cvar) = args[0];
		}

		// Print cvar value
		string value = "";
		if (cvar->type == CVAR_BOOLEAN)
		{
			if (cvar->GetValue().Bool)
				value = "true";
			else
				value = "false";
		}
		else if (cvar->type == CVAR_INTEGER)
			value = S_FMT("%d", cvar->GetValue().Int);
		else if (cvar->type == CVAR_FLOAT)
			value = S_FMT("%1.4f", cvar->GetValue().Float);
		else
			value = ((CStringCVar*)cvar)->value;

		Log::console(S_FMT("\"%s\" = \"%s\"", cmd_name, value));

		return;
	}

	// Toggle global debug mode
	if (cmd_name == "debug")
	{
		Global::debug = !Global::debug;
		if (Global::debug)
			Log::console("Debugging stuff enabled");
		else
			Log::console("Debugging stuff disabled");

		return;
	}

	// Command not found
	Log::console(S_FMT("Unknown command: \"%s\"", cmd_name));
}

/* Console::lastCommand
 * Returns the last command sent to the console
 *******************************************************************/
string Console::lastCommand()
{
	// Init blank string
	string lastCmd = "";

	// Get last command if any exist
	if (cmd_log.size() > 0)
		lastCmd = cmd_log.back();

	return lastCmd;
}

/* Console::prevCommand
 * Returns the previous command at [index] from the last entered (ie,
 * index=0 will be the directly previous command)
 *******************************************************************/
string Console::prevCommand(int index)
{
	// Check index
	if (index < 0 || (unsigned)index >= cmd_log.size())
		return "";

	return cmd_log[index];
}

/* Console::command
 * Returns the ConsoleCommand at the specified index
 *******************************************************************/
ConsoleCommand& Console::command(size_t index)
{
	if (index < commands.size())
		return commands[index];
	else
		return commands[0]; // Return first console command on invalid index
}

/* ConsoleCommand::ConsoleCommand
 * ConsoleCommand class constructor
 *******************************************************************/
ConsoleCommand::ConsoleCommand(string name, void(*commandFunc)(vector<string>), int min_args = 0, bool show_in_list)
{
	// Init variables
	this->name = name;
	this->commandFunc = commandFunc;
	this->min_args = min_args;
	this->show_in_list = show_in_list;

	// Add this command to the console
	App::console()->addCommand(*this);
}

/* ConsoleCommand::execute
 * Executes the console command
 *******************************************************************/
void ConsoleCommand::execute(vector<string> args)
{
	// Only execute if we have the minimum args specified
	if (args.size() >= min_args)
		commandFunc(args);
	else
		Log::console(S_FMT("Missing command arguments, type \"cmdhelp %s\" for more information", name));
}


/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

/* Console Command - "echo"
 * A simple command to print the first given argument to the console.
 * Subsequent arguments are ignored.
 *******************************************************************/
CONSOLE_COMMAND (echo, 1, true)
{
	Log::console(args[0]);
}

/* Console Command - "cmdlist"
 * Lists all valid console commands
 *******************************************************************/
CONSOLE_COMMAND (cmdlist, 0, true)
{
	Log::console(S_FMT("%d Valid Commands:", App::console()->numCommands()));

	for (int a = 0; a < App::console()->numCommands(); a++)
	{
		ConsoleCommand& cmd = App::console()->command(a);
		if (cmd.showInList() || Global::debug)
			Log::console(S_FMT("\"%s\" (%d args)", cmd.getName(), cmd.minArgs()));
	}
}

/* Console Command - "cvarlist"
 * Lists all cvars
 *******************************************************************/
CONSOLE_COMMAND (cvarlist, 0, true)
{
	// Get sorted list of cvars
	vector<string> list;
	get_cvar_list(list);
	sort(list.begin(), list.end());

	Log::console(S_FMT("%lu CVars:", list.size()));

	// Write list to console
	for (unsigned a = 0; a < list.size(); a++)
		Log::console(list[a]);
}

/* Console Command - "cmdhelp"
* Opens the wiki page for a console command
*******************************************************************/
CONSOLE_COMMAND(cmdhelp, 1, true)
{
	// Check command exists
	for (int a = 0; a < App::console()->numCommands(); a++)
	{
		if (App::console()->command(a).getName().Lower() == args[0].Lower())
		{
#ifdef USE_WEBVIEW_STARTPAGE
			MainEditor::openDocs(S_FMT("%s-Console-Command", args[0]));
#else
			string url = S_FMT("https://github.com/sirjuddington/SLADE/wiki/%s-Console-Command", args[0]);
			wxLaunchDefaultBrowser(url);
#endif
			return;
		}
	}

	// No command found
	Log::console(S_FMT("No command \"%s\" exists", args[0]));
}

CONSOLE_COMMAND (testmatch, 0, false)
{
	bool match = args[0].Matches(args[1]);
	if (match)
		Log::console("Match");
	else
		Log::console("No Match");
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
		Log::console(lmsg);
	}
}

#endif
