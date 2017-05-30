
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Log.cpp
 * Description: The SLADE logging implementation
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
#include <fstream>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace Log
{
	vector<Message>	log;
	std::ofstream	log_file;
}
CVAR(Int, log_verbosity, 1, CVAR_SAVE)


/*******************************************************************
 * FREEIMAGE ERROR HANDLER
 *******************************************************************/

/* FreeImageErrorHandler
 * Allows us to catch FreeImage errors and log them
 *******************************************************************/
void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message)
{
	string error = "FreeImage: ";
	if (fif != FIF_UNKNOWN)
		error += S_FMT("[%s] ", FreeImage_GetFormatFromFIF(fif));
	error += message;

	Log::error(error);
}


/*******************************************************************
 * LOG::MESSAGE STRUCT FUNCTIONS
 *******************************************************************/

/* Log::Message::formattedMessageLine
 * Returns the log entry as a formatted string:
 * HH:MM:SS: <message>
 *******************************************************************/
string Log::Message::formattedMessageLine() const
{
	return S_FMT("%s: %s", wxDateTime(timestamp).FormatISOTime(), CHR(message));
}


/*******************************************************************
 * LOG NAMESPACE FUNCTIONS
 *******************************************************************/

/* Log::init
 * Initialises the log file and logging stuff
 *******************************************************************/
void Log::init()
{
	// Redirect sf::err output to the log file
	log_file.open(CHR(App::path("slade3.log", App::Dir::User)));
	sf::err().rdbuf(log_file.rdbuf());

	// Write logfile header
	string year = wxNow().Right(4);
	info("SLADE - It's a Doom Editor");
	info(S_FMT("Version %s", Global::version));
	if (Global::sc_rev != "") info(S_FMT("Git Revision %s", Global::sc_rev));
	info(S_FMT("Written by Simon Judd, 2008-%s", year));
#ifdef SFML_VERSION_MAJOR
	info(S_FMT(
		"Compiled with wxWidgets %i.%i.%i and SFML %i.%i",
		wxMAJOR_VERSION,
		wxMINOR_VERSION,
		wxRELEASE_NUMBER,
		SFML_VERSION_MAJOR,
		SFML_VERSION_MINOR
	));
#else
	info(S_FMT(
		"Compiled with wxWidgets %i.%i.%i",
		wxMAJOR_VERSION,
		wxMINOR_VERSION,
		wxRELEASE_NUMBER
	));
#endif
	info("--------------------------------");

	// Set up FreeImage to use our log:
	FreeImage_SetOutputMessage(FreeImageErrorHandler);
}

/* Log::history
 * Returns the log message history
 *******************************************************************/
const vector<Log::Message>& Log::history()
{
	return log;
}

/* Log::verbosity
 * Returns the current log verbosity level, log messages with a
 * higher level than the current verbosity will not be logged
 *******************************************************************/
int Log::verbosity()
{
	return log_verbosity;
}

/* Log::setVerbosity
 * Sets the log verbosity level to [verbosity]
 *******************************************************************/
void Log::setVerbosity(int verbosity)
{
	log_verbosity = verbosity;
}

/* Log::message
 * Logs a message [text] of [type]
 *******************************************************************/
void Log::message(MessageType type, const char* text)
{
	// Add log message
	log.push_back({ text, type, wxDateTime::Now().GetTicks() });

	// Write to log file
	if (log_file.is_open() && type != MessageType::Console)
		sf::err() << log.back().formattedMessageLine() << "\n";
}

/* Log::message
 * Logs a message [text] of [type] at verbosity [level]
 *******************************************************************/
void Log::message(MessageType type, int level, const char* text)
{
	if (level > log_verbosity)
		return;

	// Add log message
	log.push_back({ text, type, wxDateTime::Now().GetTicks() });

	// Write to log file
	if (log_file.is_open() && type != MessageType::Console)
		sf::err() << log.back().formattedMessageLine() << "\n";
}
