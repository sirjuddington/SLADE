#pragma once

wxDECLARE_EVENT(wxEVT_THREAD_WEBGET_COMPLETED, wxThreadEvent);

namespace Web
{
std::string getHttp(const std::string& host, const std::string& uri);
void        getHttpAsync(const std::string& host, const std::string& uri, wxEvtHandler* event_handler);
} // namespace Web
