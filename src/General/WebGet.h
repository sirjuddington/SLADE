#pragma once

#include "common.h"

wxDECLARE_EVENT(wxEVT_THREAD_WEBGET_COMPLETED, wxThreadEvent);

class WebGet : public wxThread
{
public:
	WebGet(wxEvtHandler* handler, string host, string path);

	ExitCode Entry() override;

private:
	wxEvtHandler*	handler_;
	string			host_;
	string			path_;
};

namespace Web
{
	void getHttp(const char* host, const char* uri, wxEvtHandler* event_handler);
}
