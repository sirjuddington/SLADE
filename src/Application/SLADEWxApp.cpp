
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SLADEWxApp.cpp
// Description: SLADEWxApp class functions.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "SLADEWxApp.h"
#include "App.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Database/Database.h"
#include "General/Console.h"
#include "General/SAction.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/ArchiveManagerPanel.h"
#include "MainEditor/UI/MainWindow.h"
#include "UI/Dialogs/CrashReportDialog.h"
#include "UI/WxWebpHandler.h"
#include "Utility/JsonUtils.h"
#include "Utility/Parser.h"
#include <cpptrace/from_current.hpp>
#include <wx/filefn.h>
#include <wx/statbmp.h>
#include <wx/url.h>
#include <wx/webrequest.h>
#ifdef UPDATEREVISION
#include "gitinfo.h"
#endif

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::global
{
string error;

#ifdef GIT_DESCRIPTION
string sc_rev = GIT_DESCRIPTION;
#else
string sc_rev;
#endif

#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif

int win_version_major = 0;
int win_version_minor = 0;
} // namespace slade::global

namespace
{
bool         update_check_message_box = false;
const string update_check_url{
	"https://raw.githubusercontent.com/sirjuddington/SLADE-aux/refs/heads/main/version_win.txt"
};
} // namespace

CVAR(String, dir_last, "", CVar::Flag::Save)
CVAR(Bool, update_check, true, CVar::Flag::Save)
CVAR(Bool, update_check_beta, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Classes
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SLADELog class
//
// Extension of the wxLog class to send all wxWidgets log messages
// to the SLADE log implementation
// -----------------------------------------------------------------------------
class SLADELog : public wxLog
{
protected:
	// wx < 3.1.0 is no longer supported.
#if !(wxCHECK_VERSION(3, 1, 0))
#error This will not compile with wxWidgets older than 3.1.0 !
#endif
	void DoLogText(const wxString& msg) override
	{
		static const string wx_prefix = "[wxWidgets] ";

		const auto msg_str = msg.utf8_string();
		if (msg.Lower().Contains(wxS("error")))
			log::error(wx_prefix + msg_str.substr(10));
		else if (msg.Lower().Contains(wxS("warning")))
			log::warning(wx_prefix + msg_str.substr(10));
		else
			log::info(wx_prefix + msg_str.substr(10));
	}

public:
	SLADELog()           = default;
	~SLADELog() override = default;
};


// -----------------------------------------------------------------------------
// MainAppFileListener and related Classes
//
// wxWidgets IPC classes used to send filenames of archives to open
// from one SLADE instance to another in the case where a second
// instance is opened
// -----------------------------------------------------------------------------
class MainAppFLConnection : public wxConnection
{
public:
	MainAppFLConnection()           = default;
	~MainAppFLConnection() override = default;

	bool OnAdvise(const wxString& topic, const wxString& item, const void* data, size_t size, wxIPCFormat format)
		override
	{
		return true;
	}

	bool OnPoke(const wxString& topic, const wxString& item, const void* data, size_t size, wxIPCFormat format) override
	{
		app::archiveManager().openArchive(item.utf8_string());
		return true;
	}
};

class MainAppFileListener : public wxServer
{
public:
	MainAppFileListener()           = default;
	~MainAppFileListener() override = default;

	wxConnectionBase* OnAcceptConnection(const wxString& topic) override { return new MainAppFLConnection(); }

	static wxString serverName()
	{
#ifdef __WXGTK__
		// Use $XDG_RUNTIME_DIR or /tmp for the server name on Linux/Unix
		wxString server;
		wxGetEnv(wxS("XDG_RUNTIME_DIR"), &server);
		if (server.IsEmpty())
			wxGetEnv(wxS("TMPDIR"), &server);
		if (server.IsEmpty())
			server = wxS("/tmp");
		server += wxS("/SLADE_MAFL");
		return server;
#else
		return wxS("SLADE_MAFL");
#endif
	}
};

class MainAppFLClient : public wxClient
{
public:
	MainAppFLClient()           = default;
	~MainAppFLClient() override = default;

	wxConnectionBase* OnMakeConnection() override { return new MainAppFLConnection(); }
};


// -----------------------------------------------------------------------------
//
// SLADEWxApp Class Functions
//
// -----------------------------------------------------------------------------
IMPLEMENT_APP(SLADEWxApp)


// -----------------------------------------------------------------------------
// Checks if another instance of SLADE is already running, and if so, sends the
// args to the file listener of the existing SLADE process.
// Returns false if another instance was found and the new SLADE was started
// with arguments
// -----------------------------------------------------------------------------
bool SLADEWxApp::singleInstanceCheck()
{
	auto data_dir = wxStandardPaths::Get().GetUserDataDir();
	if (!wxDirExists(data_dir))
		wxMkdir(data_dir);

	single_instance_checker_ = new wxSingleInstanceChecker;
	single_instance_checker_->Create(WX_FMT("SLADE-{}", app::version().toString()), data_dir);

	if (argc == 1)
		return true;

	if (single_instance_checker_->IsAnotherRunning())
	{
		delete single_instance_checker_;

		// Connect to the file listener of the existing SLADE process
		auto client = std::make_unique<MainAppFLClient>();
		if (auto connection = client->MakeConnection(wxGetHostName(), MainAppFileListener::serverName(), wxS("files")))
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

// -----------------------------------------------------------------------------
// Application initialization, run when program is started
// -----------------------------------------------------------------------------
bool SLADEWxApp::OnInit()
{
	// Check if an instance of SLADE is already running
	if (!singleInstanceCheck())
	{
		printf("Found active instance. Quitting.\n");
		return false;
	}

	// Init wxSocket stuff (for WebGet)
	wxSocketBase::Initialize();

	// Start up file listener
	file_listener_ = new MainAppFileListener();
	file_listener_->Create(MainAppFileListener::serverName());

	// Setup system options
	wxSystemOptions::SetOption(wxS("mac.listctrl.always_use_generic"), 1);

	// Set application name (for wx directory stuff)
#ifdef __WINDOWS__
	wxApp::SetAppName(wxS("SLADE3"));
#else
	wxApp::SetAppName(wxS("slade3"));
#endif

	// Handle exceptions using wxDebug stuff, but only in release mode
#ifdef NDEBUG
	wxHandleFatalExceptions(true);
#endif

	// Load image handlers
	wxInitAllImageHandlers();
#if !wxCHECK_VERSION(3, 3, 0)
	wxImage::AddHandler(new WxWebpHandler);
#endif

	// Get Windows version
#ifdef __WXMSW__
	wxGetOsVersion(&global::win_version_major, &global::win_version_minor);
	log::info("Windows Version: {}.{}", global::win_version_major, global::win_version_minor);
#endif

	// Reroute wx log messages
	wxLog::SetActiveTarget(new SLADELog());

	// Get command line arguments
	vector<string> args;
	for (int a = 1; a < argc; a++)
		args.push_back(argv[a].utf8_string());

	// Init application
	bool init_ok = false;
	CPPTRACE_TRY
	{
		init_ok = app::init(args);
	}
	CPPTRACE_CATCH(const std::exception& ex)
	{
		log::error("Exception during SLADE initialization: {}", ex.what());
		app::handleException();
	}
	if (!init_ok)
		return false;

	// Init crash dialog
	// Do it now rather than after a crash happens, since it may fail depending on the type of crash
#ifndef _DEBUG
	crash_dialog_ = new ui::CrashReportDialog(GetMainTopWindow());
#endif //_DEBUG

	// Check for updates
#ifdef __WXMSW__
	wxHTTP::Initialize();
	if (update_check)
		checkForUpdates(false);
#endif

	// Bind events
	Bind(wxEVT_MENU, &SLADEWxApp::onMenu, this);
	Bind(wxEVT_WEBREQUEST_STATE, &SLADEWxApp::onWebRequestUpdate, this);
	Bind(wxEVT_ACTIVATE_APP, &SLADEWxApp::onActivate, this);
	Bind(wxEVT_QUERY_END_SESSION, &SLADEWxApp::onEndSession, this);

	return true;
}

// -----------------------------------------------------------------------------
// Application shutdown, run when program is closed
// -----------------------------------------------------------------------------
int SLADEWxApp::OnExit()
{
	wxSocketBase::Shutdown();
	delete single_instance_checker_;
	delete file_listener_;

	// Close program database after wx cleanup/exit as we want to keep the database
	// connection open until all windows are closed etc.
	auto retcode = wxApp::OnExit();
	database::close();

	return retcode;
}

// -----------------------------------------------------------------------------
// Handler for when a fatal exception occurs - show the stack trace/crash dialog
// if it's configured to be used
// -----------------------------------------------------------------------------
void SLADEWxApp::OnFatalException()
{
#ifndef _DEBUG
	dynamic_cast<ui::CrashReportDialog*>(crash_dialog_)->loadFromCpptrace(cpptrace::generate_trace());
	crash_dialog_->CenterOnParent();
	crash_dialog_->ShowModal();
#endif //_DEBUG
}

// -----------------------------------------------------------------------------
// Handler for when an unhandled exception occurs - log it to the console
// -----------------------------------------------------------------------------
bool SLADEWxApp::OnExceptionInMainLoop()
{
	CPPTRACE_TRY
	{
		cpptrace::rethrow();
	}
	CPPTRACE_CATCH(const std::exception& ex)
	{
		string error = ex.what();

#ifdef _DEBUG
		wxTrap();
#endif

		app::handleException();
	}

	return true;
}

#ifdef __APPLE__
void SLADEWxApp::MacOpenFile(const wxString& fileName)
{
	theMainWindow->archiveManagerPanel()->openFile(fileName.utf8_string());
}
#endif // __APPLE__

// -----------------------------------------------------------------------------
// Runs the version checker, if [message_box] is true, a message box will be
// shown if already up-to-date
// -----------------------------------------------------------------------------
void SLADEWxApp::checkForUpdates(bool message_box)
{
#ifdef __WXMSW__
	update_check_message_box = message_box;
	log::info(1, "Checking for updates...");
	auto request              = wxWebSession::GetDefault().CreateRequest(this, wxString::FromUTF8(update_check_url));
	version_check_request_id_ = request.GetId();
	request.Start();
#endif
}


// -----------------------------------------------------------------------------
//
// SLADEWxApp Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a menu item is selected in the application
// -----------------------------------------------------------------------------
void SLADEWxApp::onMenu(wxCommandEvent& e)
{
	bool handled = false;

	// Find applicable action
	auto s_action = SAction::fromWxId(e.GetId());
	auto action   = s_action->id();

	// Handle action if valid
	if (action != "invalid")
	{
		SActionHandler::setWxIdOffset(e.GetId() - s_action->wxId());
		handled = SActionHandler::doAction(action);

		if (s_action->type() == SAction::Type::Check)
		{
			// Check if triggering object is a menu item
			if (e.GetEventObject() && e.GetEventObject()->IsKindOf(wxCLASSINFO(wxMenuItem)))
			{
				auto item = dynamic_cast<wxMenuItem*>(e.GetEventObject());
				item->Check(s_action->isChecked());
			}
		}
	}

	// If not handled, let something else handle it
	if (!handled)
		e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a web request status is updated
// -----------------------------------------------------------------------------
void SLADEWxApp::onWebRequestUpdate(wxWebRequestEvent& e)
{
	// Version Check request update
	if (e.GetRequest().GetId() == version_check_request_id_)
	{
		// Check failed
		if (e.GetState() == wxWebRequest::State_Failed || e.GetState() == wxWebRequest::State_Unauthorized)
		{
			log::error("Update check failed, unable to connect");
			if (update_check_message_box)
				wxMessageBox(
					wxS("Update check failed: unable to connect to internet. "
						"Check your connection and try again."),
					wxS("Check for Updates"));

			return;
		}

		// If not completed, ignore
		if (e.GetState() != wxWebRequest::State_Completed)
			return;

		// Parse version info
		app::Version stable, beta;
		string       bin_stable, installer_stable, bin_beta; // Currently unused but may be useful in the future
		Parser       parser;
		auto         response_string = e.GetResponse().AsString();
		if (parser.parseText(response_string.utf8_string()))
		{
			// Stable
			if (auto node_stable = parser.parseTreeRoot()->childPTN("stable"))
			{
				// Version
				if (auto node_version = node_stable->childPTN("version"))
				{
					stable.major    = node_version->intValue(0);
					stable.minor    = node_version->intValue(1);
					stable.revision = node_version->intValue(2);
				}

				// Binaries link
				if (auto node_bin = node_stable->childPTN("bin"))
					bin_stable = node_bin->stringValue();

				// Installer link
				if (auto node_install = node_stable->childPTN("install"))
					installer_stable = node_install->stringValue();
			}

			// Beta
			if (auto node_beta = parser.parseTreeRoot()->childPTN("beta"))
			{
				// Version
				if (auto node_version = node_beta->childPTN("version"))
				{
					beta.major    = node_version->intValue(0);
					beta.minor    = node_version->intValue(1);
					beta.revision = node_version->intValue(2);
				}

				// Beta number
				if (auto node_beta_num = node_beta->childPTN("beta"))
					beta.beta = node_beta_num->intValue();

				// Binaries link
				if (auto node_bin = node_beta->childPTN("bin"))
					bin_beta = node_bin->stringValue();
			}
		}

		// Check for correct info
		if (stable.major == 0 || beta.major == 0)
		{
			log::warning("Update check failed, received invalid version info");
			log::debug("Received version text:\n\n{}", response_string.utf8_string());
			if (update_check_message_box)
				wxMessageBox(wxS("Update check failed: received invalid version info."), wxS("Check for Updates"));
			return;
		}

		log::info("Latest stable release: v{}", stable.toString());
		log::info("Latest beta release: v{}", beta.toString());

		// Check if new stable version
		bool new_stable = app::version().cmp(stable) < 0;
		bool new_beta   = app::version().cmp(beta) < 0;

		// Set up for new beta/stable version prompt (if any)
		string message, caption, version;
		if (update_check_beta && new_beta)
		{
			// New Beta
			caption = "New Beta Version Available";
			version = beta.toString();
			message = fmt::format(
				"A new beta version of SLADE is available ({}), click OK to visit the SLADE homepage "
				"and download the update.",
				version);
		}
		else if (new_stable)
		{
			// New Stable
			caption = "New Version Available";
			version = stable.toString();
			message = fmt::format(
				"A new version of SLADE is available ({}), click OK to visit the SLADE homepage and "
				"download the update.",
				version);
		}
		else
		{
			// No update
			log::info(1, "Already up-to-date");
			if (update_check_message_box)
				wxMessageBox(wxS("SLADE is already up to date"), wxS("Check for Updates"));

			return;
		}

		// Prompt to update
		if (wxMessageBox(wxString::FromUTF8(message), wxString::FromUTF8(caption), wxOK | wxCANCEL) == wxOK)
			wxLaunchDefaultBrowser(wxS("https://slade.mancubus.net/index.php?page=downloads"));
	}
}

// -----------------------------------------------------------------------------
// Called when the app gains focus
// -----------------------------------------------------------------------------
void SLADEWxApp::onActivate(wxActivateEvent& e)
{
	if (!e.GetActive() || app::isExiting())
	{
		e.Skip();
		return;
	}

	// Check open directory archives for changes on the file system
	if (theMainWindow && theMainWindow->archiveManagerPanel())
		theMainWindow->archiveManagerPanel()->checkDirArchives();

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the system is ending the session (shutdown/restart)
// -----------------------------------------------------------------------------
void SLADEWxApp::onEndSession(wxCloseEvent& e)
{
	session_ending_ = true;
	maineditor::windowWx()->Close();
	e.Skip();
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

CONSOLE_COMMAND(crash, 0, false)
{
	if (wxMessageBox(
			wxS("Yes, this command does actually exist and *will* crash the program. Do you really want it to crash?"),
			wxS("...Really?"),
			wxYES_NO | wxCENTRE)
		== wxYES)
	{
		uint8_t* test = nullptr;
		*test         = 5;
	}
}

CONSOLE_COMMAND(exception, 0, false)
{
	string test;
	auto   c = test.at(100);
}

CONSOLE_COMMAND(quit, 0, true)
{
	bool save_config = true;
	for (auto& arg : args)
	{
		if (strutil::equalCI(arg, "nosave"))
			save_config = false;
	}

	app::exit(save_config);
}
