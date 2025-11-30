
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Log.cpp
// Description: The SLADE logging implementation
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
#include "App.h"
#include <SFML/System/Err.hpp>
#include <fmt/chrono.h>
#include <fstream>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::log
{
vector<Message> log;
std::ofstream   log_file;
} // namespace slade::log
CVAR(Int, log_verbosity, 1, CVar::Flag::Save)


// -----------------------------------------------------------------------------
// Formatter for fmt so that log::MessageType can be written to a string
// -----------------------------------------------------------------------------
template<> struct fmt::formatter<log::MessageType> : formatter<string_view>
{
	auto format(const log::MessageType& type, format_context& ctx) const -> format_context::iterator
	{
		string_view prefix;
		switch (type)
		{
		case log::MessageType::Info:    prefix = " [Info]"; break;
		case log::MessageType::Warning: prefix = " [Warn]"; break;
		case log::MessageType::Error:   prefix = "[Error]"; break;
		case log::MessageType::Debug:   prefix = "[Debug]"; break;
		case log::MessageType::Script:  prefix = "[Script]"; break;
		default:                        prefix = "  [Log]"; break;
		}
		return formatter<string_view>::format(prefix, ctx);
	}
};


// -----------------------------------------------------------------------------
//
// log::Message Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the log entry as a formatted string:
// HH:MM:SS: <message>
// -----------------------------------------------------------------------------
string log::Message::formattedMessageLine() const
{
	return fmt::format("{:%H:%M:%S}: {} {}", timestamp, type, message);
}


// -----------------------------------------------------------------------------
//
// Log Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initialises the log file and logging stuff
// -----------------------------------------------------------------------------
void log::init()
{
	// Redirect sf::err output to the log file
	log_file.open(app::path("slade3.log", app::Dir::User));
	sf::err().rdbuf(log_file.rdbuf());

	// Write logfile header
	auto t  = std::time(nullptr);
	auto tm = std::localtime(&t);
	info("SLADE - It's a Doom Editor");
	info(fmt::format("Version {}", app::version().toString()));
	if (!global::sc_rev.empty())
		info(fmt::format("Git Revision {}", global::sc_rev));
	if (app::platform() == app::Platform::Windows)
		info(fmt::format("{} Windows Build", app::isWin64Build() ? "64bit" : "32bit"));
	info(fmt::format("Written by Simon Judd, 2008-{:%Y}", *tm));
#ifdef SFML_VERSION_MAJOR
	info(
		fmt::format(
			"Compiled with wxWidgets {}.{}.{} and SFML {}.{}.{}",
			wxMAJOR_VERSION,
			wxMINOR_VERSION,
			wxRELEASE_NUMBER,
			SFML_VERSION_MAJOR,
			SFML_VERSION_MINOR,
			SFML_VERSION_PATCH));
#else
	info(fmt::format("Compiled with wxWidgets {}.{}.{}", wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER));
#endif
	info("--------------------------------");
}

// -----------------------------------------------------------------------------
// Returns the log message history
// -----------------------------------------------------------------------------
const vector<log::Message>& log::history()
{
	return log;
}

// -----------------------------------------------------------------------------
// Returns the last [n] log messages (as pointers)
// -----------------------------------------------------------------------------
vector<log::Message*> log::last(int n)
{
	vector<Message*> list;

	if (n < 0 || n > log.size())
		n = log.size();

	for (auto i = log.size() - n; i < log.size(); i++)
		list.push_back(&log[i]);

	return list;
}

// -----------------------------------------------------------------------------
// Returns the current log verbosity level, log messages with a higher level
// than the current verbosity will not be logged
// -----------------------------------------------------------------------------
int log::verbosity()
{
	return log_verbosity;
}

// -----------------------------------------------------------------------------
// Sets the log verbosity level to [verbosity]
// -----------------------------------------------------------------------------
void log::setVerbosity(int verbosity)
{
	log_verbosity = verbosity;
}

// -----------------------------------------------------------------------------
// Logs a message [text] of [type]
// -----------------------------------------------------------------------------
void log::message(MessageType type, string_view text)
{
	// Add log message
	auto t = std::time(nullptr);
	log.emplace_back(text, type, *std::localtime(&t));

	// Write to log file
	if (log_file.is_open() && type != MessageType::Console)
	{
		sf::err() << log.back().formattedMessageLine() << "\n";
		sf::err().flush();
	}

	// Log to debugger in windows+debug
#if defined(__WXMSW__) && defined(SLADE_DEBUG)
	OutputDebugStringA(fmt::format("{}\r\n", log.back().formattedMessageLine()).c_str());
#endif
}

void log::message(MessageType type, int level, string_view text, fmt::format_args args)
{
	message(type, level, fmt::vformat(text, args));
}

void log::message(MessageType type, string_view text, fmt::format_args args)
{
	message(type, fmt::vformat(text, args));
}

// -----------------------------------------------------------------------------
// Returns a list of log messages of [type] that have been recorded since [time]
// -----------------------------------------------------------------------------
vector<log::Message*> log::since(time_t time, MessageType type)
{
	vector<Message*> list;
	for (auto& msg : log)
		if (mktime(&msg.timestamp) >= time && (type == MessageType::Any || msg.type == type))
			list.push_back(&msg);
	return list;
}

// -----------------------------------------------------------------------------
// Logs a debug message [text] at verbosity [level] only if debug mode is on
// -----------------------------------------------------------------------------
void log::debug(int level, string_view text)
{
	if (global::debug)
		message(MessageType::Debug, level, text);
}

// -----------------------------------------------------------------------------
// Logs a debug message [text] only if debug mode is on
// -----------------------------------------------------------------------------
void log::debug(string_view text)
{
	if (global::debug)
		message(MessageType::Debug, text);
}

void log::debug(int level, string_view text, fmt::format_args args)
{
	if (global::debug)
		message(MessageType::Debug, level, text, args);
}

void log::debug(string_view text, fmt::format_args args)
{
	if (global::debug)
		message(MessageType::Debug, text, args);
}

// -----------------------------------------------------------------------------
// Logs a message [text] of [type] at verbosity [level]
// -----------------------------------------------------------------------------
void log::message(MessageType type, int level, string_view text)
{
	if (level > log_verbosity)
		return;

	// Add log message
	auto t = std::time(nullptr);
	log.emplace_back(text, type, *std::localtime(&t));

	// Write to log file
	if (log_file.is_open() && type != MessageType::Console)
		sf::err() << log.back().formattedMessageLine() << "\n";
}
