#pragma once

#include <wx/thread.h>

wxDECLARE_EVENT(wxEVT_COMMAND_IDGAMES_APICALL_COMPLETED, wxThreadEvent);

class wxXmlNode;
namespace idGames
{
	class ApiCall : public wxThread
	{
	public:
		ApiCall(wxEvtHandler* handler, string command, vector<key_value_t> params);
		virtual ~ApiCall();

		ExitCode Entry();

	private:
		wxEvtHandler*		handler_	= nullptr;
		string				command_;
		vector<key_value_t>	params_;
	};

	struct Review
	{
		string	text;
		int		rating;

		Review() {}
		Review(string text, int rating)
			: text(text), rating(rating) {}
	};

	struct File
	{
		int				id;
		string			title;
		string			dir;
		string			filename;
		int				size_bytes;
		int				age;
		string			date;
		string			author;
		string			email;
		string			description;
		string			credits;
		string			base;
		string			build_time;
		string			editors;
		string			bugs;
		string			text_file;
		double			rating;
		int				votes;
		string			url;
		vector<Review>	reviews;
	};

	void	readFileXml(File& file, wxXmlNode* file_node);
	void	readFileReviews(File& file, wxXmlNode* reviews_node);
}
