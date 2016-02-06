
#ifndef __CONSOLEPANEL_H__
#define __CONSOLEPANEL_H__

#include "General/ListenerAnnouncer.h"
#include <wx/panel.h>

class ConsolePanel : public wxPanel, Listener
{
private:
	wxTextCtrl*	text_log;
	wxTextCtrl*	text_command;
	int			cmd_log_index;

public:
	ConsolePanel(wxWindow* parent, int id);
	~ConsolePanel();

	void initLayout();

	// Listener stuff
	void onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	// Events
	void onCommandEnter(wxCommandEvent& e);
	void onCommandKeyDown(wxKeyEvent& e);
};

#endif//__CONSOLEPANEL_H__
