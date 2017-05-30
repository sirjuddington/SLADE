/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SLADEWxApp.cpp
 * Description: SLADEWxApp class functions.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "SLADEWxApp.h"
#include "Archive/ArchiveManager.h"
#include "External/email/wxMailer.h"
#include "General/Console/Console.h"
#include "General/VersionCheck.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MainEditor/UI/ArchiveManagerPanel.h"
#include "OpenGL/OpenGL.h"
#include <wx/statbmp.h>

#undef BOOL

#ifdef UPDATEREVISION
#include "gitinfo.h"
#endif


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace Global
{
	string error = "";

	int beta_num = 1;
	int version_num = 3120;
	string version = "3.1.2 Beta 1";
#ifdef GIT_DESCRIPTION
	string sc_rev = GIT_DESCRIPTION;
#else
	string sc_rev = "";
#endif

#ifdef DEBUG
	bool debug = true;
#else
	bool debug = false;
#endif

	double ppi_scale = 1.0;
	int win_version_major = 0;
	int win_version_minor = 0;
}


string	current_action = "";
bool	update_check_message_box = false;
CVAR(String, dir_last, "", CVAR_SAVE)
CVAR(Bool, update_check, true, CVAR_SAVE)
CVAR(Bool, update_check_beta, false, CVAR_SAVE)


/*******************************************************************
 * CLASSES
 *******************************************************************/

/* SLADELog class
 * Extension of the wxLog class to send all wxWidgets log messages
 * to the SLADE log implementation
 *******************************************************************/
class SLADELog : public wxLog
{
protected:
	// wx2.9.x is no longer supported.
#if (wxMAJOR_VERSION < 3)
#error This will not compile with wxWidgets older than 3.0.0 !
#endif
	void DoLogText(const wxString& msg) override
	{
		Log::message(Log::MessageType::Info, msg);
	}

public:
	SLADELog() {}
	~SLADELog() {}
};


/* SLADEStackTrace class
 * Extension of the wxStackWalker class that formats stack trace
 * information to a multi-line string, that can be retrieved via
 * getTraceString(). wxStackWalker is currently unimplemented on some
 * platforms, so unfortunately it has to be disabled there
 *******************************************************************/
#if wxUSE_STACKWALKER
class SLADEStackTrace : public wxStackWalker
{
public:
	SLADEStackTrace()
	{
		stack_trace = "Stack Trace:\n";
	}

	~SLADEStackTrace() {}

	string getTraceString() const
	{
		return stack_trace;
	}

	string getTopLevel() const
	{
		return top_level;
	}

	void OnStackFrame(const wxStackFrame& frame) override
	{
		string location = "[unknown location] ";
		if (frame.HasSourceLocation())
			location = S_FMT("(%s:%d) ", frame.GetFileName(), frame.GetLine());

		wxUIntPtr address = wxPtrToUInt(frame.GetAddress());
		string func_name = frame.GetName();
		if (func_name.IsEmpty())
			func_name = S_FMT("[unknown:%d]", address);

		string line = S_FMT("%s%s", location, func_name);
		stack_trace.Append(S_FMT("%d: %s\n", frame.GetLevel(), line));

		if (frame.GetLevel() == 0)
			top_level = line;
	}

private:
	string	stack_trace;
	string	top_level;
};


/* SLADECrashDialog class
 * A simple dialog that displays a crash message and a scrollable,
 * multi-line textbox with a stack trace
 *******************************************************************/
class SLADECrashDialog : public wxDialog, public wxThreadHelper
{
public:
	SLADECrashDialog(SLADEStackTrace& st) : wxDialog(wxTheApp->GetTopWindow(), -1, "SLADE Application Crash")
	{
		top_level = st.getTopLevel();

		// Setup sizer
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND);

		// Add dead doomguy picture
		theArchiveManager->programResourceArchive()
			->entryAtPath("images/STFDEAD0.png")
			->exportFile(App::path("STFDEAD0.png", App::Dir::Temp));
		wxImage img;
		img.LoadFile(App::path("STFDEAD0.png", App::Dir::Temp));
		img.Rescale(img.GetWidth(), img.GetHeight(), wxIMAGE_QUALITY_NEAREST);
		wxStaticBitmap* picture = new wxStaticBitmap(this, -1, wxBitmap(img));
		hbox->Add(picture, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxTOP|wxBOTTOM, 10);

		// Add general crash message
#ifndef NOCURL
		string message = "SLADE has crashed unexpectedly. To help fix the problem that caused this crash, "
			"please (optionally) enter a short description of what you were doing at the time "
			"of the crash, and click the 'Send Crash Report' button.";
#else
		string message = "SLADE has crashed unexpectedly. To help fix the problem that caused this crash, "
			"please email a copy of the stack trace below to sirjuddington@gmail.com, along with a "
			"description of what you were doing at the time of the crash.";
#endif
		wxStaticText* label = new wxStaticText(this, -1, message);
		hbox->Add(label, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 10);
		label->Wrap(480 - 20 - picture->GetSize().x);

#ifndef NOCURL
		// Add description text area
		text_description = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxSize(-1, 100), wxTE_MULTILINE);
		sizer->Add(new wxStaticText(this, -1, "Description:"), 0, wxLEFT|wxRIGHT, 10);
		sizer->AddSpacer(2);
		sizer->Add(text_description, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);
#endif

		// SLADE info
		trace = S_FMT("Version: %s\n", Global::version);
		if (current_action.IsEmpty())
			trace += "No current action\n";
		else
			trace += S_FMT("Current action: %s", current_action);
		trace += "\n";

		// System info
		OpenGL::gl_info_t gl_info = OpenGL::getInfo();
		trace += "Operating System: " + wxGetOsDescription() + "\n";
		trace += "Graphics Vendor: " + gl_info.vendor + "\n";
		trace += "Graphics Hardware: " + gl_info.renderer + "\n";
		trace += "OpenGL Version: " + gl_info.version + "\n";

		// Stack trace
		trace += "\n";
		trace += st.getTraceString();

		// Last 10 log lines
		trace += "\nLast Log Messages:\n";
		auto& log = Log::history();
		for (auto a = log.size() - 10; a < log.size(); a++)
			trace += log[a].message + "\n";

		// Add stack trace text area
		text_stack = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL);
		text_stack->SetValue(trace);
		text_stack->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
		sizer->Add(new wxStaticText(this, -1, "Crash Information:"), 0, wxLEFT|wxRIGHT, 10);
		sizer->AddSpacer(2);
		sizer->Add(text_stack, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

		// Dump stack trace to a file (just in case)
		wxFile file(App::path("slade3_crash.log", App::Dir::User), wxFile::write);
		file.Write(trace);
		file.Close();

		// Also dump stack trace to console
		std::cerr << trace;

#ifndef NOCURL
		// Add small privacy disclaimer
		string privacy = "Sending a crash report will only send the information displayed above, "
						"along with a copy of the logs for this session.";
		label = new wxStaticText(this, -1, privacy);
		label->Wrap(480);
		sizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 10);
#endif

		// Add 'Copy Stack Trace' button
		hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 6);
		btn_copy_trace = new wxButton(this, -1, "Copy Stack Trace");
		hbox->AddStretchSpacer();
		hbox->Add(btn_copy_trace, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4);
		btn_copy_trace->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SLADECrashDialog::onBtnCopyTrace, this);

		// Add 'Exit SLADE' button
		btn_exit = new wxButton(this, -1, "Exit SLADE");
		hbox->Add(btn_exit, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4);
		btn_exit->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SLADECrashDialog::onBtnExit, this);

#ifndef NOCURL
		// Add 'Send Crash Report' button
		btn_send = new wxButton(this, -1, "Send Crash Report");
		hbox->Add(btn_send, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4);
		btn_send->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &SLADECrashDialog::onBtnSend, this);
#endif

		Bind(wxEVT_THREAD, &SLADECrashDialog::onThreadUpdate, this);
		Bind(wxEVT_CLOSE_WINDOW, &SLADECrashDialog::onClose, this);

		// Setup layout
		Layout();
		SetInitialSize(wxSize(500, 600));
		CenterOnParent();
	}

	~SLADECrashDialog() {}

	wxThread::ExitCode Entry()
	{
		wxMailer mailer("slade.errors@gmail.com", "hxixjnwdovyoktwq", "smtp://smtp.gmail.com:587");

		// Create message
		wxEmailMessage msg;
		msg.SetFrom("SLADE");
		msg.SetTo("slade.errors@gmail.com");
		msg.SetSubject("[" + Global::version + "] @ " + top_level);
		msg.SetMessage(S_FMT("Description:\n%s\n\n%s", text_description->GetValue(), trace));
		msg.AddAttachment(App::path("slade3.log", App::Dir::User));
		msg.Finalize();

		// Send email
		bool sent = mailer.Send(msg);

		// Send event
		wxThreadEvent* evt = new wxThreadEvent();
		evt->SetInt(sent ? 1 : 0);
		wxQueueEvent(GetEventHandler(), evt);

		return (wxThread::ExitCode)0;
	}

	void onBtnCopyTrace(wxCommandEvent& e)
	{
		if (wxTheClipboard->Open())
		{
			wxTheClipboard->SetData(new wxTextDataObject(trace));
			wxTheClipboard->Flush();
			wxTheClipboard->Close();
			wxMessageBox("Stack trace successfully copied to clipboard");
		}
		else
			wxMessageBox("Unable to access the system clipboard, please select+copy the text above manually", wxMessageBoxCaptionStr, wxICON_EXCLAMATION);
	}

	void onBtnSend(wxCommandEvent& e)
	{
		btn_send->SetLabel("Sending...");
		btn_send->Enable(false);
		btn_exit->Enable(false);

		if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR)
			EndModal(wxID_OK);

		if (GetThread()->Run() != wxTHREAD_NO_ERROR)
			EndModal(wxID_OK);
	}

	void onBtnExit(wxCommandEvent& e)
	{
		EndModal(wxID_OK);
	}

	void onThreadUpdate(wxThreadEvent& e)
	{
		if (e.GetInt() == 1)
		{
			// Report sent successfully, exit
			wxMessageBox(
				"The crash report was sent successfully, and SLADE will now close.",
				"Crash Report Sent"
			);
			EndModal(wxID_OK);
		}
		else
		{
			// Sending failed
			btn_send->SetLabel("Send Crash Report");
			btn_send->Enable();
			btn_exit->Enable();
			wxMessageBox(
				"The crash report failed to send. Please either try again or click 'Exit SLADE' "
				"to exit without sending.",
				"Failed to Send"
			);
		}
	}

	void onClose(wxCloseEvent& e)
	{
		if (GetThread() && GetThread()->IsRunning())
			GetThread()->Wait();

		Destroy();
	}

private:
	wxTextCtrl*	text_stack;
	wxTextCtrl*	text_description;
	wxButton*	btn_copy_trace;
	wxButton*	btn_exit;
	wxButton*	btn_send;
	string		trace;
	string		top_level;
};
#endif//wxUSE_STACKWALKER


/* MainAppFileListener and related Classes
 * wxWidgets IPC classes used to send filenames of archives to open
 * from one SLADE instance to another in the case where a second
 * instance is opened
 *******************************************************************/
class MainAppFLConnection : public wxConnection
{
public:
	MainAppFLConnection() {}
	~MainAppFLConnection() {}

	bool OnAdvise(
		const wxString& topic,
		const wxString& item,
		const void* data,
		size_t size,
		wxIPCFormat format) override
	{
		return true;
	}

	bool OnPoke(
		const wxString& topic,
		const wxString& item,
		const void *data,
		size_t size,
		wxIPCFormat format) override
	{
		theArchiveManager->openArchive(item);
		return true;
	}
};

class MainAppFileListener : public wxServer
{
public:
	MainAppFileListener() {}
	~MainAppFileListener() {}

	wxConnectionBase* OnAcceptConnection(const wxString& topic) override
	{
		return new MainAppFLConnection();
	}
};

class MainAppFLClient : public wxClient
{
public:
	MainAppFLClient() {}
	~MainAppFLClient() {}

	wxConnectionBase* OnMakeConnection() override
	{
		return new MainAppFLConnection();
	}
};


/*******************************************************************
 * SLADEWXAPP CLASS FUNCTIONS
 *******************************************************************/
IMPLEMENT_APP(SLADEWxApp)

/* SLADEWxApp::SLADEWxApp
 * SLADEWxApp class constructor
 *******************************************************************/
SLADEWxApp::SLADEWxApp() :
	single_instance_checker{ nullptr },
	file_listener{ nullptr }
{
}

/* SLADEWxApp::~SLADEWxApp
 * SLADEWxApp class destructor
 *******************************************************************/
SLADEWxApp::~SLADEWxApp()
{
}

/* SLADEWxApp::singleInstanceCheck
 * Checks if another instance of SLADE is already running, and if so,
 * sends the args to the file listener of the existing SLADE
 * process. Returns false if another instance was found and the new
 * SLADE was started with arguments
 *******************************************************************/
bool SLADEWxApp::singleInstanceCheck()
{
	single_instance_checker = new wxSingleInstanceChecker;

	if (argc == 1)
		return true;

	if (single_instance_checker->IsAnotherRunning())
	{
		delete single_instance_checker;

		// Connect to the file listener of the existing SLADE process
		auto client = std::make_unique<MainAppFLClient>();
		auto connection = client->MakeConnection(wxGetHostName(), "SLADE_MAFL", "files");

		if (connection)
		{
			// Send args as archives to open
			for (int a = 1; a < argc; a++)
				connection->Poke(argv[a], argv[a]);

			connection->Disconnect();
		}

		return false;
	}

	return true;
}

/* SLADEWxApp::OnInit
 * Application initialization, run when program is started
 *******************************************************************/
bool SLADEWxApp::OnInit()
{
	// Check if an instance of SLADE is already running
	if (!singleInstanceCheck())
	{
		printf("Found active instance. Quitting.\n");
		return false;
	}

	// Init global variables
	Global::error = "";

	// Start up file listener
	file_listener = new MainAppFileListener();
	file_listener->Create("SLADE_MAFL");

	// Setup system options
	wxSystemOptions::SetOption("mac.listctrl.always_use_generic", 1);

	// Set application name (for wx directory stuff)
#ifdef __WINDOWS__
	wxApp::SetAppName("SLADE3");
#else
	wxApp::SetAppName("slade3");
#endif

	// Handle exceptions using wxDebug stuff, but only in release mode
#ifdef NDEBUG
	wxHandleFatalExceptions(true);
#endif

	// Load image handlers
	wxInitAllImageHandlers();

	// Calculate scaling factor (from system ppi)
	wxMemoryDC dc;
	Global::ppi_scale = (double)(dc.GetPPI().x) / 96.0;

	// Get Windows version
#ifdef __WXMSW__
	wxGetOsVersion(&Global::win_version_major, &Global::win_version_minor);
	LOG_MESSAGE(1, "Windows Version: %d.%d", Global::win_version_major, Global::win_version_minor);
#endif

	// Init application
	if (!App::init())
		return false;

	// Reroute wx log messages
	wxLog::SetActiveTarget(new SLADELog());

	// Check for updates
#ifdef __WXMSW__
	wxHTTP::Initialize();
	if (update_check)
		checkForUpdates(false);
#endif

	// Bind events
	Bind(wxEVT_MENU, &SLADEWxApp::onMenu, this);
	Bind(wxEVT_COMMAND_VERSIONCHECK_COMPLETED, &SLADEWxApp::onVersionCheckCompleted, this);
	Bind(wxEVT_ACTIVATE_APP, &SLADEWxApp::onActivate, this);

	return true;
}

/* SLADEWxApp::OnExit
 * Application shutdown, run when program is closed
 *******************************************************************/
int SLADEWxApp::OnExit()
{
	delete single_instance_checker;
	delete file_listener;

	return 0;
}

void SLADEWxApp::OnFatalException()
{
#if wxUSE_STACKWALKER
#ifndef _DEBUG
	SLADEStackTrace st;
	st.WalkFromException();
	SLADECrashDialog sd(st);
	sd.ShowModal();
#endif//_DEBUG
#endif//wxUSE_STACKWALKER
}

#ifdef __APPLE__
void SLADEWxApp::MacOpenFile(const wxString &fileName)
{
	theMainWindow->getArchiveManagerPanel()->openFile(fileName);
}
#endif // __APPLE__

/* SLADEWxApp::checkForUpdates
 * Runs the version checker, if [message_box] is true, a message box
 * will be shown if already up-to-date
 *******************************************************************/
void SLADEWxApp::checkForUpdates(bool message_box)
{
#ifdef __WXMSW__
	update_check_message_box = message_box;
	LOG_MESSAGE(1, "Checking for updates...");
	VersionCheck* checker = new VersionCheck(this);
	checker->Create();
	checker->Run();
#endif
}


/*******************************************************************
 * MAINAPP CLASS EVENTS
 *******************************************************************/

/* SLADEWxApp::onMenu
 * Called when a menu item is selected in the application
 *******************************************************************/
void SLADEWxApp::onMenu(wxCommandEvent& e)
{
	bool handled = false;

	// Find applicable action
	auto s_action = SAction::fromWxId(e.GetId());
	auto action = s_action->getId();

	// Handle action if valid
	if (action != "invalid")
	{
		current_action = action;
		SActionHandler::setWxIdOffset(e.GetId() - s_action->getWxId());
		handled = SActionHandler::doAction(action);

		// Check if triggering object is a menu item
		if (s_action && s_action->getType() == SAction::Type::Check)
		{
			if (e.GetEventObject() && e.GetEventObject()->IsKindOf(wxCLASSINFO(wxMenuItem)))
			{
				auto item = (wxMenuItem*)e.GetEventObject();
				item->Check(s_action->isChecked());
			}
		}

		current_action = "";
	}

	// If not handled, let something else handle it
	if (!handled)
		e.Skip();
}

/* SLADEWxApp::onVersionCheckCompleted
 * Called when the VersionCheck thread completes
 *******************************************************************/
void SLADEWxApp::onVersionCheckCompleted(wxThreadEvent& e)
{
	// Check failed
	if (e.GetString() == "connect_failed")
	{
		LOG_MESSAGE(1, "Version check failed, unable to connect");
		if (update_check_message_box)
			wxMessageBox("Update check failed: unable to connect to internet. Check your connection and try again.", "Check for Updates");
		return;
	}

	wxArrayString info = wxSplit(e.GetString(), '\n');
	
	// Check for correct info
	if (info.size() != 5)
	{
		LOG_MESSAGE(1, "Version check failed, received invalid version info");
		if (update_check_message_box)
			wxMessageBox("Update check failed: received invalid version info.", "Check for Updates");
		return;
	}

	// Get version numbers
	long version_stable, version_beta, beta_num;
	info[0].ToLong(&version_stable);
	info[2].ToLong(&version_beta);
	info[3].ToLong(&beta_num);

	LOG_MESSAGE(1, "Latest stable release: v%ld \"%s\"", version_stable, info[1].Trim());
	LOG_MESSAGE(1, "Latest beta release: v%ld_b%ld \"%s\"", version_beta, beta_num, info[4].Trim());

	// Check if new stable version
	bool new_stable = false;
	if (Global::version_num < version_stable ||								// New stable version
		(Global::version_num == version_stable && Global::beta_num > 0))	// Stable version of current beta
		new_stable = true;

	// Check if new beta version
	bool new_beta = false;
	if (version_stable < version_beta)
	{
		// Stable -> Beta
		if (Global::version_num < version_beta && Global::beta_num == 0)
			new_beta = true;

		// Beta -> Beta
		else if (Global::version_num < version_beta ||															// New version beta
				(Global::beta_num < beta_num && Global::version_num == version_beta && Global::beta_num > 0))	// Same version, newer beta
			new_beta = true;
	}

	// Ask for new beta
	if (update_check_beta && new_beta)
	{
		if (wxMessageBox(S_FMT("A new beta version of SLADE is available (%s), click OK to visit the SLADE homepage and download the update.", info[4].Trim()), "New Beta Version Available", wxOK|wxCANCEL) == wxOK)
			wxLaunchDefaultBrowser("http://slade.mancubus.net/index.php?page=downloads");

		return;
	}

	// Ask for new stable
	if (new_stable)
	{
		if (wxMessageBox(S_FMT("A new version of SLADE is available (%s), click OK to visit the SLADE homepage and download the update.", info[1].Trim()), "New Version Available", wxOK|wxCANCEL) == wxOK)
			wxLaunchDefaultBrowser("http://slade.mancubus.net/index.php?page=downloads");

		return;
	}

	LOG_MESSAGE(1, "Already up-to-date");
	if (update_check_message_box)
		wxMessageBox("SLADE is already up to date", "Check for Updates");
}

/* SLADEWxApp::onActivate
 * Called when the app gains focus
 *******************************************************************/
void SLADEWxApp::onActivate(wxActivateEvent& e)
{
	if (!e.GetActive() || App::isExiting())
	{
		e.Skip();
		return;
	}

	// Check open directory archives for changes on the file system
	if (theMainWindow && theMainWindow->getArchiveManagerPanel())
		theMainWindow->getArchiveManagerPanel()->checkDirArchives();

	e.Skip();
}

/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

CONSOLE_COMMAND (crash, 0, false)
{
	if (wxMessageBox("Yes, this command does actually exist and *will* crash the program. Do you really want it to crash?", "...Really?", wxYES_NO|wxCENTRE) == wxYES)
	{
		uint8_t* test = nullptr;
		test[123] = 5;
	}
}

CONSOLE_COMMAND(quit, 0, true)
{
	bool save_config = true;
	for (unsigned a = 0; a < args.size(); a++)
	{
		if (args[a].Lower() == "nosave")
			save_config = false;
	}

	App::exit(save_config);
}
