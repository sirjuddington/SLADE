#pragma once

class wxSingleInstanceChecker;
class MainAppFileListener;
class SLADECrashDialog;

class SLADEWxApp : public wxApp
{
public:
	SLADEWxApp()           = default;
	~SLADEWxApp() override = default;

	bool isSessionEnding() const { return session_ending_; }

	bool OnInit() override;
	int  OnExit() override;
	void OnFatalException() override;

#ifdef __APPLE__
	virtual void MacOpenFile(const wxString& fileName) override;
#endif // __APPLE__

	bool singleInstanceCheck();
	void checkForUpdates(bool message_box);

	void onMenu(wxCommandEvent& e);
	void onVersionCheckCompleted(wxThreadEvent& e);
	void onActivate(wxActivateEvent& e);
	void onEndSession(wxCloseEvent& e);

private:
	wxSingleInstanceChecker* single_instance_checker_ = nullptr;
	MainAppFileListener*     file_listener_           = nullptr;
	SLADECrashDialog*        crash_dialog_            = nullptr;
	bool                     session_ending_          = false;
};

DECLARE_APP(SLADEWxApp)
