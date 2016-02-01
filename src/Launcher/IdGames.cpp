
#include "Main.h"
#include "WxStuff.h"
#include "IdGames.h"
#include "Console.h"
#include "Utility/XmlHelpers.h"
#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include <wx/xml/xml.h>

using namespace idGames;

wxDEFINE_EVENT(wxEVT_COMMAND_IDGAMES_APICALL_COMPLETED, wxThreadEvent);

ApiCall::ApiCall(wxEvtHandler* handler, string command, vector<key_value_t> params)
	: wxThread(),
	handler(handler),
	command(command),
	params(params)
{
}

ApiCall::~ApiCall()
{
}

wxThread::ExitCode ApiCall::Entry()
{
	// Init HTTP
	wxHTTP http;
	http.SetHeader("Content-type", "text/html; charset=utf-8");
	http.SetTimeout(10);

	// Wait for connection
	LOG_MESSAGE(3, "ApiCall: Testing connection...");
	int attempt_count = 0;
	while (!http.Connect("www.doomworld.com"))
	{
		LOG_MESSAGE(3, "ApiCall: No connection, waiting 1 sec");
		wxSleep(1);

		// Max connection attempts
		if (attempt_count++ > 5)
		{
			// Send (failed) event
			if (handler)
			{
				wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_IDGAMES_APICALL_COMPLETED);
				event->SetString("connect_failed");
				wxQueueEvent(handler, event);
			}

			return NULL;
		}
	}

	// Build API call string
	string call = "/idgames/api/api.php?action=" + command;
	for (unsigned a = 0; a < params.size(); a++)
		call += "&" + params[a].key + "=" + params[a].value;

	// Get version info
	LOG_MESSAGE(3, "ApiCall: " + call);
	wxInputStream* stream = http.GetInputStream(call);
	string result;
	if (http.GetError() == wxPROTO_NOERR)
	{
		wxStringOutputStream out(&result);
		stream->Read(out);

		LOG_MESSAGE(3, "ApiCall: Got result:\n%s", result);
	}
	else
	{
		LOG_MESSAGE(3, "ApiCall: Error connecting to www.doomworld.com");
	}

	// Clean up
	delete stream;
	http.Close();

	// Send event
	if (handler)
	{
		wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_IDGAMES_APICALL_COMPLETED);
		event->SetString(command + ":" + result);
		wxQueueEvent(handler, event);
	}

	return NULL;
}



void idGames::readFileXml(file_t& file, wxXmlNode* node)
{
	wxXmlNode* child = node->GetChildren();
	long l_val;
	double d_val;
	while (child)
	{
		// Id
		if (child->GetName() == "id")
		{
			string text = XmlHelpers::getContent(child);
			if (text.ToLong(&l_val))
				file.id = l_val;
		}

		// Title
		else if (child->GetName() == "title")
			file.title = XmlHelpers::getContent(child);

		// Dir
		else if (child->GetName() == "dir")
			file.dir = XmlHelpers::getContent(child);

		// Filename
		else if (child->GetName() == "filename")
			file.filename = XmlHelpers::getContent(child);

		// Size
		if (child->GetName() == "size")
		{
			string text = XmlHelpers::getContent(child);
			if (text.ToLong(&l_val))
				file.size_bytes = l_val;
		}

		// Age
		if (child->GetName() == "age")
		{
			string text = XmlHelpers::getContent(child);
			if (text.ToLong(&l_val))
				file.age = l_val;
		}

		// Date
		else if (child->GetName() == "date")
			file.date = XmlHelpers::getContent(child);

		// Author
		else if (child->GetName() == "author")
			file.author = XmlHelpers::getContent(child);

		// Email
		else if (child->GetName() == "email")
			file.email = XmlHelpers::getContent(child);

		// Description
		else if (child->GetName() == "description")
			file.description = XmlHelpers::getContent(child);

		// Credits
		else if (child->GetName() == "credits")
			file.credits = XmlHelpers::getContent(child);

		// Base
		else if (child->GetName() == "base")
			file.base = XmlHelpers::getContent(child);

		// Build Time
		else if (child->GetName() == "buildtime")
			file.build_time = XmlHelpers::getContent(child);

		// Editors Used
		else if (child->GetName() == "editors")
			file.editors = XmlHelpers::getContent(child);

		// Bugs
		else if (child->GetName() == "bugs")
			file.bugs = XmlHelpers::getContent(child);

		// Text File
		else if (child->GetName() == "textfile")
			file.text_file = XmlHelpers::getContent(child);

		// Rating
		if (child->GetName() == "rating")
		{
			string text = XmlHelpers::getContent(child);
			if (text.ToDouble(&d_val))
				file.rating = d_val;
		}

		// No. Votes
		if (child->GetName() == "votes")
		{
			string text = XmlHelpers::getContent(child);
			if (text.ToLong(&l_val))
				file.votes = l_val;
		}

		// URL
		else if (child->GetName() == "url")
			file.url = XmlHelpers::getContent(child);

		// Reviews
		else if (child->GetName() == "reviews")
			readFileReviews(file, child);

		child = child->GetNext();
	}
}

void idGames::readFileReviews(file_t& file, wxXmlNode* reviews_node)
{
	string r_text;
	string r_rating;
	long l_val;

	wxXmlNode* review_node = reviews_node->GetChildren();
	while (review_node)
	{
		wxXmlNode* child = review_node->GetChildren();

		// Read review properties
		while (child)
		{
			if (child->GetName() == "text")
				r_text = XmlHelpers::getContent(child);

			else if (child->GetName() == "vote")
			{
				r_rating = XmlHelpers::getContent(child);
				if (!r_rating.ToLong(&l_val))
					l_val = -1;
			}

			child = child->GetNext();
		}

		// Add review
		file.reviews.push_back(review_t(r_text, l_val));

		review_node = review_node->GetNext();
	}
}





CONSOLE_COMMAND(idgames_test, 1, false)
{
	string command = args[0];
	unsigned a = 1;
	vector<key_value_t> params;
	while (a < args.size())
	{
		string param = args[a++];
		if (a == args.size())
			break;
		string value = args[a++];
		params.push_back(key_value_t(param, value));
	}

	ApiCall* call = new ApiCall(NULL, command, params);
	call->Create();
	call->Run();
}
