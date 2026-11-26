#pragma once

class wxWebRequestEvent;
class wxSingleInstanceChecker;
class MainAppFileListener;
class wxWindow;

#ifdef SLADE_MAP_SCRIPT_CLI
using SLADEWxAppBase = wxAppConsole;
#else
using SLADEWxAppBase = wxApp;
#endif

class SLADEWxApp : public SLADEWxAppBase
{
public:
	SLADEWxApp();
	~SLADEWxApp() override = default;

	bool isSessionEnding() const { return session_ending_; }

#ifdef SLADE_MAP_SCRIPT_CLI
	// Some shared source files refer to these wxApp GUI helpers. The CLI never
	// creates windows, but retaining no-op forms lets it share the core without
	// initializing a GUI toolkit.
	bool      IsActive() const { return false; }
	void      SetTopWindow(wxWindow*) {}
	wxWindow* GetTopWindow() const { return nullptr; }
#endif

	bool OnInit() override;
	int  OnExit() override;
#ifdef SLADE_MAP_SCRIPT_CLI
	int OnRun() override;
#endif
	void OnFatalException() override;
	bool OnExceptionInMainLoop() override;

#if defined(__APPLE__) && !defined(SLADE_MAP_SCRIPT_CLI)
	virtual void MacOpenFile(const wxString& fileName) override;
#endif // __APPLE__

	bool singleInstanceCheck();
	void checkForUpdates(bool message_box);

	void onMenu(wxCommandEvent& e);
	void onWebRequestUpdate(wxWebRequestEvent& e);
	void onActivate(wxActivateEvent& e);
	void onEndSession(wxCloseEvent& e);

private:
	wxSingleInstanceChecker* single_instance_checker_  = nullptr;
	MainAppFileListener*     file_listener_            = nullptr;
	wxDialog*                crash_dialog_             = nullptr;
	bool                     session_ending_           = false;
	int                      version_check_request_id_ = 0;
};

DECLARE_APP(SLADEWxApp)
