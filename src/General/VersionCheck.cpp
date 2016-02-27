
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    VersionCheck.cpp
 * Description: VersionCheck class, a thread that checks online for
 *              the latest version info and sends an event when done
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
#include "UI/WxStuff.h"
#include "General/VersionCheck.h"
#include <wx/protocol/http.h>
#include <wx/sstream.h>


/*******************************************************************
 * VERSIONCHECK CLASS FUNCTIONS
 *******************************************************************/
wxDEFINE_EVENT(wxEVT_COMMAND_VERSIONCHECK_COMPLETED, wxThreadEvent);

/* VersionCheck::VersionCheck
 * VersionCheck class constructor
 *******************************************************************/
VersionCheck::VersionCheck(wxEvtHandler* handler) : wxThread()
{
	this->handler = handler;
}

/* VersionCheck::~VersionCheck
 * VersionCheck class constructor
 *******************************************************************/
VersionCheck::~VersionCheck()
{
}

/* VersionCheck::Entry
 * VersionCheck thread entry function
 *******************************************************************/
wxThread::ExitCode VersionCheck::Entry()
{
	LOG_MESSAGE(3, "Starting VersionCheck thread");

	// Init HTTP
	wxHTTP http;
	http.SetHeader("Content-type", "text/html; charset=utf-8");
	http.SetTimeout(10);

	// Wait for connection
	LOG_MESSAGE(3, "VersionCheck: Testing connection...");
	int attempt_count = 0;
	while (!http.Connect("slade.mancubus.net"))
	{
		LOG_MESSAGE(3, "VersionCheck: No connection, waiting 1 sec");
		wxSleep(1);
		
		// Max connection attempts
		if (attempt_count++ > 5)
		{
			// Send (failed) event
			wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_VERSIONCHECK_COMPLETED);
			event->SetString("connect_failed");
			wxQueueEvent(handler, event);

			return NULL;
		}
	}

	// Get version info
	LOG_MESSAGE(3, "VersionCheck: Retrieving version information...");
	wxInputStream* stream = http.GetInputStream("/version.txt");
	string version;
	if (http.GetError() == wxPROTO_NOERR)
	{
		wxStringOutputStream out(&version);
		stream->Read(out);

		LOG_MESSAGE(3, "VersionCheck: Got version info:\n%s", version);
	}
	else
	{
		LOG_MESSAGE(3, "VersionCheck: Error connecting to slade.mancubus.net");
	}

	// Clean up
	delete stream;
	http.Close();

	// Send event
	wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_VERSIONCHECK_COMPLETED);
	event->SetString(version);
	wxQueueEvent(handler, event);

	return NULL;
}
