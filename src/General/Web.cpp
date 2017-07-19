
#include "Main.h"
#include "Web.h"
#include <SFML/Network.hpp>
#include <thread>

wxDEFINE_EVENT(wxEVT_THREAD_WEBGET_COMPLETED, wxThreadEvent);

string Web::getHttp(const char* host, const char* uri)
{
	// Setup connection & request
	sf::Http http(host);
	sf::Http::Request request;
	request.setMethod(sf::Http::Request::Get);
	request.setUri(uri);

	// Send HTTP request
	auto response = http.sendRequest(request);

	switch (response.getStatus())
	{
	case sf::Http::Response::Ok:
		return response.getBody();
	default:
		return "connect_failed";
	}
}

void Web::getHttpAsync(const char* host, const char* uri, wxEvtHandler* event_handler)
{
	std::thread thread([=]()
	{
		// Queue wx event with http request response
		auto event = new wxThreadEvent(wxEVT_THREAD_WEBGET_COMPLETED);
		event->SetString(getHttp(host, uri));
		wxQueueEvent(event_handler, event);
	});

	thread.detach();
}
