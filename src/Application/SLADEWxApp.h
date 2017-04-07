#pragma once

#include "common.h"

class wxSingleInstanceChecker;
class MainAppFileListener;

class SLADEWxApp : public wxApp
{
public:
	SLADEWxApp();
	~SLADEWxApp();

	bool OnInit() override;
	int OnExit() override;
	void OnFatalException() override;

#ifdef __APPLE__
	virtual void MacOpenFile(const wxString &fileName);
#endif // __APPLE__

	bool	singleInstanceCheck();
	void	checkForUpdates(bool message_box);

	void	onMenu(wxCommandEvent& e);
	void	onVersionCheckCompleted(wxThreadEvent& e);
	void	onActivate(wxActivateEvent& event);

private:
	wxSingleInstanceChecker*	single_instance_checker;
	MainAppFileListener*		file_listener;
};
