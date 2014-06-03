
#include "Main.h"
#include "WxStuff.h"
#include "VersionCheck.h"
#include <wx/protocol/http.h>
#include <wx/sstream.h>

wxDEFINE_EVENT(wxEVT_COMMAND_VERSIONCHECK_COMPLETED, wxThreadEvent);

VersionCheck::VersionCheck(wxEvtHandler* handler) : wxThread()
{
	this->handler = handler;
}

VersionCheck::~VersionCheck()
{
}

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
		wxSleep(1000);
		
		if (attempt_count++ > 5)
		{
			// Send (failed) event
			wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_VERSIONCHECK_COMPLETED);
			event->SetString("connect_failed");
			wxQueueEvent(handler, event);
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
