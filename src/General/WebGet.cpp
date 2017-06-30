
#include "Main.h"
#include "WebGet.h"

wxDEFINE_EVENT(wxEVT_THREAD_WEBGET_COMPLETED, wxThreadEvent);

WebGet::WebGet(wxEvtHandler* handler, string host, string path) :
	wxThread(),
	handler_{ handler },
	host_{ host },
	path_{ path }
{
	// Ensure path begins with /
	if (!path.StartsWith("/"))
		path.Prepend("/");
}

wxThread::ExitCode WebGet::Entry()
{
	// Init HTTP
	wxHTTP http;
	http.SetHeader("Content-type", "text/html; charset=utf-8");
	http.SetTimeout(10);

	// Wait for connection
	Log::info(3, S_FMT("WebGet: Testing connection to %s...", CHR(host_)));
	int attempt_count = 0;
	while (!http.Connect(host_))
	{
		Log::info(3, "WebGet: No connection, waiting 1 sec");
		wxSleep(1);

		// Max connection attempts
		if (attempt_count++ > 5)
		{
			// Send (failed) event
			auto event = new wxThreadEvent(wxEVT_THREAD_WEBGET_COMPLETED);
			event->SetString("connect_failed");
			wxQueueEvent(handler_, event);

			return nullptr;
		}
	}

	// Get data
	Log::info(3, "WebGet: Retrieving data...");
	auto stream = http.GetInputStream(path_);
	string data;
	if (http.GetError() == wxPROTO_NOERR)
	{
		wxStringOutputStream out(&data);
		stream->Read(out);

		Log::info(3, S_FMT("WebGet: Got data successfully:\n%s", CHR(data)));
	}
	else
	{
		Log::info(3, S_FMT("WebGet: Error connecting to %s", CHR(host_)));
	}

	// Clean up
	delete stream;
	http.Close();

	// Send event
	auto event = new wxThreadEvent(wxEVT_THREAD_WEBGET_COMPLETED);
	event->SetString(data);
	wxQueueEvent(handler_, event);

	return nullptr;
}
