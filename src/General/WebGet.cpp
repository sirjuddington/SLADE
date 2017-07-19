
#include "Main.h"
#include "WebGet.h"
#include <SFML/Network.hpp>
#include <thread>

wxDEFINE_EVENT(wxEVT_THREAD_WEBGET_COMPLETED, wxThreadEvent);

WebGet::WebGet(wxEvtHandler* handler, string host, string path) :
	wxThread(),
	handler_{ handler },
	host_{ host },
	path_{ path }
{
	// Ensure path begins with /
	if (!path_.StartsWith("/"))
		path_.Prepend("/");
}

wxThread::ExitCode WebGet::Entry()
{
	sf::Http http(CHR(host_));

	sf::Http::Request request;
	request.setMethod(sf::Http::Request::Get);
	request.setUri(CHR(path_));

	auto response = http.sendRequest(request);

	if (response.getStatus() == sf::Http::Response::Ok)
	{
		// Send event
		auto event = new wxThreadEvent(wxEVT_THREAD_WEBGET_COMPLETED);
		event->SetString(response.getBody());
		wxQueueEvent(handler_, event);
	}
	else
	{
		// Send (failed) event
		auto event = new wxThreadEvent(wxEVT_THREAD_WEBGET_COMPLETED);
		event->SetString("connect_failed");
		wxQueueEvent(handler_, event);
	}

	return nullptr;
}

void Web::getHttp(const char* host, const char* uri, wxEvtHandler* event_handler)
{
	std::thread thread([=]()
	{
		sf::Http http(host);

		sf::Http::Request request;
		request.setMethod(sf::Http::Request::Get);
		request.setUri(uri);

		auto response = http.sendRequest(request);

		if (response.getStatus() == sf::Http::Response::Ok)
		{
			// Send event
			auto event = new wxThreadEvent(wxEVT_THREAD_WEBGET_COMPLETED);
			event->SetString(response.getBody());
			wxQueueEvent(event_handler, event);
		}
		else
		{
			// Send (failed) event
			auto event = new wxThreadEvent(wxEVT_THREAD_WEBGET_COMPLETED);
			event->SetString("connect_failed");
			wxQueueEvent(event_handler, event);
		}
	});

	thread.detach();
}
