
#ifndef __CONSOLEPANEL_H__
#define __CONSOLEPANEL_H__

#include "common.h"

class ConsolePanel : public wxPanel
{
public:
	ConsolePanel(wxWindow* parent, int id);
	~ConsolePanel();

	void	initLayout();
	void	setupTextArea() const;
	void	update();

private:
	wxStyledTextCtrl*	text_log_;
	wxTextCtrl*			text_command_;
	int					cmd_log_index_;
	wxTimer				timer_update_;
	unsigned			next_message_index_;

	// Events
	void onCommandEnter(wxCommandEvent& e);
	void onCommandKeyDown(wxKeyEvent& e);
};

#endif//__CONSOLEPANEL_H__
