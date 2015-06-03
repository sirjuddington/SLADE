
#ifndef __MAINAPP_H__
#define __MAINAPP_H__

#include "SAction.h"
#include <wx/app.h>
#include <wx/log.h>
#include <wx/stopwatch.h>

class SLADELog : public wxLog
{
protected:
// wx2.9.x is no longer supported.
#if (wxMAJOR_VERSION < 3)
#error This will not compile with wxWidgets older than 3.0.0 !
#endif
	void DoLogText(const wxString& msg);

public:
	SLADELog() {}
	~SLADELog() {}
};

class MainWindow;
class SActionHandler;

class MainApp : public wxApp
{
friend class SAction;
friend class SActionHandler;
private:
	MainWindow*				main_window;
	int						cur_id;
	SAction*				action_invalid;
	vector<SAction*>		actions;
	vector<SActionHandler*>	action_handlers;
	bool					init_ok;
	wxStopWatch				timer;

public:
	MainApp();
	~MainApp();

	virtual bool OnInit();
	virtual int OnExit();
	virtual void OnFatalException();

#ifdef __APPLE__
	virtual void MacOpenFile(const wxString &fileName);
#endif // __APPLE__

	MainWindow*	getMainWindow() { return main_window; }

	bool	initDirectories();
	void	initLogFile();
	void	initActions();
	void	readConfigFile();
	void	saveConfigFile();
	bool	isInitialised() { return init_ok; }
	long	runTimer() { return timer.Time(); }
	void	checkForUpdates(bool message_box);

	int			newMenuId() { return cur_id++; }
	SAction*	getAction(string id);
	bool		doAction(string id);
	void		toggleAction(string id);

	void	onMenu(wxCommandEvent& e);
	void	onVersionCheckCompleted(wxThreadEvent& e);
};

#define theApp ((MainApp*)wxTheApp)

// Basic 'interface' class for classes that handle SActions (yay multiple inheritance)
class SActionHandler
{
friend class MainApp;
protected:
	virtual bool	handleAction(string id) { return false; }

public:
	SActionHandler()
	{
		theApp->action_handlers.push_back(this);
	}

	~SActionHandler()
	{
		for (unsigned a = 0; a < theApp->action_handlers.size(); a++)
		{
			if (theApp->action_handlers[a] == this)
			{
				theApp->action_handlers.erase(theApp->action_handlers.begin() + a);
				a--;
			}
		}
	}
};

#endif //__MAINAPP_H__
