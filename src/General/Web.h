#pragma once

wxDECLARE_EVENT(wxEVT_THREAD_WEBGET_COMPLETED, wxThreadEvent);

namespace slade::web
{
string getHttp(const string& host, const string& uri);
void   getHttpAsync(const string& host, const string& uri, wxEvtHandler* event_handler);
} // namespace slade::web
