
#ifndef __CONSOLEPANEL_H__
#define __CONSOLEPANEL_H__

#include "common.h"

class ConsolePanel : public wxPanel
{
public:
	ConsolePanel(wxWindow* parent, int id);
	~ConsolePanel();

	void	initLayout();
	void	update();

	// Events
	void onCommandEnter(wxCommandEvent& e);
	void onCommandKeyDown(wxKeyEvent& e);

private:
	wxTextCtrl*	text_log;
	wxTextCtrl*	text_command;
	int			cmd_log_index;
	wxTimer		timer_update;
	unsigned	next_message_index;
};

#endif//__CONSOLEPANEL_H__
