#pragma once

wxDECLARE_EVENT(wxEVT_THREAD_WEBGET_COMPLETED, wxThreadEvent);

namespace Web
{
wxString getHttp(const char* host, const char* uri);
void     getHttpAsync(const char* host, const char* uri, wxEvtHandler* event_handler);
} // namespace Web
