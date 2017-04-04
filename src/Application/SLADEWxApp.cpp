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
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryDataFormat.h"
#include "Dialogs/SetupWizard/SetupWizardDialog.h"
#include "External/dumb/dumb.h"
#include "External/email/wxMailer.h"
#include "General/ColourConfiguration.h"
#include "General/Console/Console.h"
#include "General/Executables.h"
#include "General/KeyBind.h"
#include "General/Lua.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "General/UI.h"
#include "General/VersionCheck.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SIFormat.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MainEditor/UI/ArchiveManagerPanel.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/MapEditorWindow.h"
#include "MapEditor/NodeBuilders.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"
#include "UI/SplashWindow.h"
#include "UI/TextEditor/TextLanguage.h"
#include "UI/TextEditor/TextStyle.h"
#include "Utility/Tokenizer.h"
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

	int beta_num = 0;
	int version_num = 3120;
	string version = "3.1.2 Alpha";
#ifdef GIT_DESCRIPTION
	string sc_rev = GIT_DESCRIPTION;
#else
	string sc_rev = "";
#endif

	int log_verbosity = 1;

#ifdef DEBUG
	bool debug = true;
#else
	bool debug = false;
#endif

	double ppi_scale = 1.0;
	int win_version_major = 0;
	int win_version_minor = 0;
}


bool	exiting = false;
string	current_action = "";
bool	update_check_message_box = false;
CVAR(String, dir_last, "", CVAR_SAVE)
CVAR(Int, log_verbosity, 1, CVAR_SAVE)
CVAR(Bool, setup_wizard_run, false, CVAR_SAVE)
CVAR(Bool, update_check, true, CVAR_SAVE)
CVAR(Bool, update_check_beta, false, CVAR_SAVE)


/*******************************************************************
 * CLASSES
 *******************************************************************/

/* SLADEStackTrace class
 * Extension of the wxStackWalker class that formats stack trace
 * information to a multi-line string, that can be retrieved via
 * getTraceString(). wxStackWalker is currently unimplemented on some
 * platforms, so unfortunately it has to be disabled there
 *******************************************************************/
#if wxUSE_STACKWALKER
class SLADEStackTrace : public wxStackWalker
{
private:
	string	stack_trace;
	string	top_level;

public:
	SLADEStackTrace()
	{
		stack_trace = "Stack Trace:\n";
	}

	~SLADEStackTrace() {}

	string getTraceString()
	{
		return stack_trace;
	}

	string getTopLevel()
	{
		return top_level;
	}

	void OnStackFrame(const wxStackFrame& frame)
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
};


/* SLADECrashDialog class
 * A simple dialog that displays a crash message and a scrollable,
 * multi-line textbox with a stack trace
 *******************************************************************/
class SLADECrashDialog : public wxDialog, public wxThreadHelper
{
private:
	wxTextCtrl*	text_stack;
	wxTextCtrl*	text_description;
	wxButton*	btn_copy_trace;
	wxButton*	btn_exit;
	wxButton*	btn_send;
	string		trace;
	string		top_level;

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

		// Last 5 log lines
		trace += "\nLast Log Messages:\n";
		vector<string> lines = theConsole->lastLogLines(10);
		for (unsigned a = 0; a < lines.size(); a++)
			trace += lines[a];

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

	bool OnAdvise(const wxString& topic, const wxString& item, const void* data, size_t size, wxIPCFormat format)
	{
		return true;
	}

	virtual bool OnPoke(const wxString& topic, const wxString& item, const void *data, size_t size, wxIPCFormat format)
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

	wxConnectionBase* OnAcceptConnection(const wxString& topic)
	{
		return new MainAppFLConnection();
	}
};

class MainAppFLClient : public wxClient
{
public:
	MainAppFLClient() {}
	~MainAppFLClient() {}

	wxConnectionBase* OnMakeConnection()
	{
		return new MainAppFLConnection();
	}
};


/*******************************************************************
 * SLADELOG CLASS FUNCTIONS
 *******************************************************************/

void SLADELog::DoLogText(const wxString& msg)
{
	if (!exiting)
		theConsole->logMessage(msg);
}


/*******************************************************************
 * FREEIMAGE ERROR HANDLER
 *******************************************************************/

/* FreeImageErrorHandler
 * Allows to catch FreeImage errors and log them to the console.
 *******************************************************************/
void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message)
{
	string error = "FreeImage: ";
	if (fif != FIF_UNKNOWN)
	{
		error += S_FMT("[%s] ", FreeImage_GetFormatFromFIF(fif));
	}
	error += message;

	LOG_MESSAGE(2, error);
}


/*******************************************************************
 * SLADEWXAPP CLASS FUNCTIONS
 *******************************************************************/
IMPLEMENT_APP(SLADEWxApp)

/* SLADEWxApp::SLADEWxApp
 * SLADEWxApp class constructor
 *******************************************************************/
SLADEWxApp::SLADEWxApp()
{
	main_window = NULL;
	init_ok = false;
	save_config = true;
}

/* SLADEWxApp::~SLADEWxApp
 * SLADEWxApp class destructor
 *******************************************************************/
SLADEWxApp::~SLADEWxApp()
{
}

/* SLADEWxApp::initLogFile
 * Sets up the SLADE log file
 *******************************************************************/
void SLADEWxApp::initLogFile()
{
	// Set wxLog target(s)
	wxLog::SetActiveTarget(new SLADELog());
	FILE* log_file = fopen(CHR(App::path("slade3.log", App::Dir::User)), "wt");
	new wxLogChain(new wxLogStderr(log_file));

	// Write logfile header
	string year = wxNow().Right(4);
	wxLogMessage("SLADE - It's a Doom Editor");
	wxLogMessage("Version %s", Global::version);
	if (Global::sc_rev != "") wxLogMessage("Git Revision %s", Global::sc_rev);
	wxLogMessage("Written by Simon Judd, 2008-%s", year);
#ifdef SFML_VERSION_MAJOR
	wxLogMessage("Compiled with wxWidgets %i.%i.%i and SFML %i.%i", wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER, SFML_VERSION_MAJOR, SFML_VERSION_MINOR);
#else
	wxLogMessage("Compiled with wxWidgets %i.%i.%i", wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER);
#endif
	wxLogMessage("--------------------------------");

	// Set up FreeImage to use our log:
	FreeImage_SetOutputMessage(FreeImageErrorHandler);
}

/* SLADEWxApp::initActions
 * Sets up all menu/toolbar actions
 *******************************************************************/
void SLADEWxApp::initActions()
{
	SAction::initWxId(26000);

	// MainWindow
	new SAction("main_exit", "E&xit", "exit", "Quit SLADE", "", SAction::Type::Normal, wxID_EXIT);
	new SAction("main_undo", "Undo", "undo", "Undo", "Ctrl+Z");
	new SAction("main_redo", "Redo", "redo", "Redo", "Ctrl+Y");
	new SAction("main_setbra", "Set &Base Resource Archive", "archive", "Set the Base Resource Archive, to act as the program 'IWAD'");
	new SAction("main_preferences", "&Preferences...", "settings", "Setup SLADE options and preferences", "", SAction::Type::Normal, wxID_PREFERENCES);
	new SAction("main_showam", "&Archive Manager", "archive", "Toggle the Archive Manager window", "Ctrl+1");
	new SAction("main_showconsole", "&Console", "console", "Toggle the Console window", "Ctrl+2");
	new SAction("main_showundohistory", "&Undo History", "undo", "Toggle the Undo History window", "Ctrl+3");
	new SAction("main_onlinedocs", "Online &Documentation", "wiki", "View SLADE documentation online");
	new SAction("main_about", "&About", "logo", "Informaton about SLADE", "", SAction::Type::Normal, wxID_ABOUT);
	new SAction("main_updatecheck", "Check for Updates...", "", "Check online for updates");

	// ArchiveManagerPanel
	new SAction("aman_newwad", "New Wad Archive", "newarchive", "Create a new Doom Wad Archive", "Ctrl+Shift+W");
	new SAction("aman_newzip", "New Zip Archive", "newzip", "Create a new Zip Archive", "Ctrl+Shift+Z");
	new SAction("aman_newmap", "New Map", "mapeditor", "Create a new standalone map", "Ctrl+Shift+M");
	new SAction("aman_open", "&Open", "open", "Open an existing Archive", "Ctrl+O");
	new SAction("aman_opendir", "Open &Directory", "opendir", "Open a directory as an Archive");
	new SAction("aman_save", "&Save", "save", "Save the currently open Archive", "Ctrl+S");
	new SAction("aman_saveas", "Save &As", "saveas", "Save the currently open Archive to a new file", "Ctrl+Shift+S");
	new SAction("aman_saveall", "Save All", "saveall", "Save all open Archives");
	new SAction("aman_close", "&Close", "close", "Close the currently open Archive", "Ctrl+W");
	new SAction("aman_closeall", "Close All", "closeall", "Close all open Archives");
	new SAction("aman_recent_open", "Open", "open", "Open the selected Archive(s)");
	new SAction("aman_recent_remove", "Remove", "close", "Remove the selected Archive(s) from the recent list");
	new SAction("aman_bookmark_go", "Go To", "open", "Go to the selected bookmark");
	new SAction("aman_bookmark_remove", "Remove", "close", "Remove the selected bookmark(s) from the list");
	new SAction("aman_save_a", "&Save", "save", "Save the selected Archive", "Ctrl+S");
	new SAction("aman_saveas_a", "Save &As", "saveas", "Save the selected Archive to a new file", "Ctrl+Shift+S");
	new SAction("aman_close_a", "&Close", "close", "Close the selected Archive", "Ctrl+W");
	new SAction("aman_recent", "<insert recent file name>", "", "", "", SAction::Type::Normal, -1, -1, 20);

	// ArchivePanel
	new SAction("arch_newentry", "New Entry", "newentry", "Create a new empty entry");
	new SAction("arch_newpalette", "New PLAYPAL", "palette", "Create a new palette entry");
	new SAction("arch_newanimated", "New ANIMATED", "animation", "Create a new Boom ANIMATED entry");
	new SAction("arch_newswitches", "New SWITCHES", "switch", "Create a new Boom SWITCHES entry");
	new SAction("arch_newdir", "New Directory", "newfolder", "Create a new empty directory");
	new SAction("arch_importfiles", "&Import Files", "importfiles", "Import multiple files into the archive", "kb:el_import_files");
	new SAction("arch_buildarchive", "&Build Archive", "buildarchive", "Build archive from the current directory", "kb:el_build_archive");
	new SAction("arch_texeditor", "&Texture Editor", "texeditor", "Open the texture editor for the current archive");
	new SAction("arch_mapeditor", "&Map Editor", "mapeditor", "Open the map editor");
	new SAction("arch_clean_patches", "Remove Unused &Patches", "", "Remove any unused patches, and their associated entries");
	new SAction("arch_clean_textures", "Remove Unused &Textures", "", "Remove any unused textures");
	new SAction("arch_clean_flats", "Remove Unused &Flats", "", "Remove any unused flats");
	new SAction("arch_check_duplicates", "Check Duplicate Entry Names", "", "Checks the archive for any entries sharing the same name");
	new SAction("arch_check_duplicates2", "Check Duplicate Entry Content", "", "Checks the archive for any entries sharing the same data");
	new SAction("arch_clean_iwaddupes", "Remove Entries Duplicated from IWAD", "", "Remove entries that are exact duplicates of entries from the base resource archive");
	new SAction("arch_replace_maps", "Replace in Maps", "", "Tool to find and replace thing types, specials and textures in all maps");
	new SAction("arch_entry_rename", "Rename", "rename", "Rename the selected entries", "kb:el_rename");
	new SAction("arch_entry_rename_each", "Rename Each", "renameeach", "Rename separately all the selected entries");
	new SAction("arch_entry_delete", "Delete", "delete", "Delete the selected entries");
	new SAction("arch_entry_revert", "Revert", "revert", "Revert any modifications made to the selected entries since the last save");
	new SAction("arch_entry_cut", "Cut", "cut", "Cut the selected entries");
	new SAction("arch_entry_copy", "Copy", "copy", "Copy the selected entries");
	new SAction("arch_entry_paste", "Paste", "paste", "Paste the selected entries");
	new SAction("arch_entry_moveup", "Move Up", "up", "Move the selected entries up", "kb:el_move_up");
	new SAction("arch_entry_movedown", "Move Down", "down", "Move the selected entries down", "kb:el_move_down");
	new SAction("arch_entry_sort", "Sort", "down", "Sort the entries in the list");
	new SAction("arch_entry_import", "Import", "import", "Import a file to the selected entry", "kb:el_import");
	new SAction("arch_entry_export", "Export", "export", "Export the selected entries to files", "kb:el_export");
	new SAction("arch_entry_bookmark", "Bookmark", "bookmark", "Bookmark the current entry");
	new SAction("arch_entry_opentab", "In New Tab", "", "Open selected entries in separate tabs");
	new SAction("arch_entry_crc32", "Compute CRC-32 Checksum", "text", "Compute the CRC-32 checksums of the selected entries");
	new SAction("arch_entry_openext", "", "", "", "", SAction::Type::Normal, -1, -1, 20);
	new SAction("arch_entry_setup_external", "Setup External Editors", "settings", "Open the preferences dialog to set up external editors");
	new SAction("arch_bas_convertb", "Convert to SWANTBLS", "", "Convert any selected SWITCHES and ANIMATED entries to a single SWANTBLS entry");
	new SAction("arch_bas_convertz", "Convert to ANIMDEFS", "", "Convert any selected SWITCHES and ANIMATED entries to a single ANIMDEFS entry");
	new SAction("arch_swan_convert", "Compile to SWITCHES and ANIMATED", "", "Convert SWANTBLS entries into SWITCHES and ANIMATED entries");
	new SAction("arch_texturex_convertzd", "Convert to TEXTURES", "", "Convert any selected TEXTUREx entries to ZDoom TEXTURES format");
	new SAction("arch_texturex_finderrors", "Find Texture Errors", "", "Log to the console any error detected in the TEXTUREx entries");
	new SAction("arch_view_text", "View as Text", "text", "Open the selected entry in the text editor, regardless of type");
	new SAction("arch_view_hex", "View as Hex", "data", "Open the selected entry in the hex editor, regardless of type");
	new SAction("arch_gfx_convert", "Convert to...", "convert", "Open the Gfx Conversion Dialog for any selected gfx entries");
	new SAction("arch_gfx_translate", "Colour Remap...", "remap", "Remap a range of colours in the selected gfx entries to another range (paletted gfx only)");
	new SAction("arch_gfx_colourise", "Colourise", "colourise", "Colourise the selected gfx entries");
	new SAction("arch_gfx_tint", "Tint", "tint", "Tint the selected gfx entries by a colour/amount");
	new SAction("arch_gfx_offsets", "Modify Gfx Offsets", "offset", "Mass-modify the offsets for any selected gfx entries");
	new SAction("arch_gfx_addptable", "Add to Patch Table", "pnames", "Add selected gfx entries to PNAMES");
	new SAction("arch_gfx_addtexturex", "Add to TEXTUREx", "texturex", "Create textures from selected gfx entries and add them to TEXTUREx");
	new SAction("arch_gfx_exportpng", "Export as PNG", "export", "Export selected gfx entries to PNG format files");
	new SAction("arch_gfx_pngopt", "Optimize PNG", "pngopt", "Optimize PNG entries");
	new SAction("arch_audio_convertwd", "Convert WAV to Doom Sound", "convert", "Convert any selected WAV format entries to Doom Sound format");
	new SAction("arch_audio_convertdw", "Convert Doom Sound to WAV", "convert", "Convert any selected Doom Sound format entries to WAV format");
	new SAction("arch_audio_convertmus", "Convert MUS to MIDI", "convert", "Convert any selected MUS format entries to MIDI format");
	new SAction("arch_scripts_compileacs", "Compile ACS", "compile", "Compile any selected text entries to ACS bytecode");
	new SAction("arch_scripts_compilehacs", "Compile ACS (Hexen bytecode)", "compile2", "Compile any selected text entries to Hexen-compatible ACS bytecode");
	new SAction("arch_map_opendb2", "Open Map in Doom Builder 2", "", "Open the selected map in Doom Builder 2");
	new SAction("arch_run", "Run Archive", "run", "Run the current archive", "Ctrl+Shift+R");

	// GfxEntryPanel
	new SAction("pgfx_mirror", "Mirror", "mirror", "Mirror the graphic horizontally");
	new SAction("pgfx_flip", "Flip", "flip", "Flip the graphic vertically");
	new SAction("pgfx_rotate", "Rotate", "rotate", "Rotate the graphic");
	new SAction("pgfx_translate", "Colour Remap", "remap", "Remap a range of colours in the graphic to another range (paletted gfx only)");
	new SAction("pgfx_colourise", "Colourise", "colourise", "Colourise the graphic");
	new SAction("pgfx_tint", "Tint", "tint", "Tint the graphic by a colour/amount");
	new SAction("pgfx_alph", "alPh Chunk", "", "Add/Remove alPh chunk to/from the PNG", "", SAction::Type::Check);
	new SAction("pgfx_trns", "tRNS Chunk", "", "Add/Remove tRNS chunk to/from the PNG", "", SAction::Type::Check);
	new SAction("pgfx_extract", "Extract All", "", "Extract all images in this entry to separate PNGs");
	new SAction("pgfx_crop", "Crop", "settings", "Crop the graphic");
	new SAction("pgfx_convert", "Convert to...", "convert", "Open the Gfx Conversion Dialog for the entry");
	new SAction("pgfx_pngopt", "Optimize PNG", "pngopt", "Optimize PNG entry");

	// ArchiveEntryList
	new SAction("aelt_sizecol", "Size", "", "Show the size column", "", SAction::Type::Check);
	new SAction("aelt_typecol", "Type", "", "Show the type column", "", SAction::Type::Check);
	new SAction("aelt_indexcol", "Index", "", "Show the index column", "", SAction::Type::Check);
	new SAction("aelt_hrules", "Horizontal Rules", "", "Show horizontal rules between entries", "", SAction::Type::Check);
	new SAction("aelt_vrules", "Vertical Rules", "", "Show vertical rules between columns", "", SAction::Type::Check);
	new SAction("aelt_bgcolour", "Colour by Type", "", "Colour item background by entry type", "", SAction::Type::Check);
	new SAction("aelt_bgalt", "Alternating Row Colour", "", "Show alternating row colours", "", SAction::Type::Check);

	// TextureEditorPanel
	new SAction("txed_new", "New Texture", "tex_new", "Create a new, empty texture", "kb:txed_tex_new");
	new SAction("txed_delete", "Delete Texture", "tex_delete", "Deletes the selected texture(s) from the list", "kb:txed_tex_delete");
	new SAction("txed_new_patch", "New Texture from Patch", "tex_newpatch", "Create a new texture from an existing patch", "kb:txed_tex_new_patch");
	new SAction("txed_new_file", "New Texture from File", "tex_newfile", "Create a new texture from an image file", "kb:txed_tex_new_file");
	new SAction("txed_rename", "Rename Texture", "tex_rename", "Rename the selected texture(s)");
	new SAction("txed_rename_each", "Rename Each", "tex_renameeach", "Rename separately all the selected textures");
	new SAction("txed_export", "Export Texture", "tex_export", "Create standalone images from the selected texture(s)");
	new SAction("txed_extract", "Extract Texture", "tex_extract", "Export the selected texture(s) as PNG files");
	new SAction("txed_offsets", "Modify Offsets", "tex_offset", "Mass modify offsets in the selected texture(s)");
	new SAction("txed_up", "Move Up", "up", "Move the selected texture(s) up in the list", "kb:txed_tex_up");
	new SAction("txed_down", "Move Down", "down", "Move the selected texture(s) down in the list", "kb:txed_tex_down");
	new SAction("txed_sort", "Sort", "down", "Sort the textures in the list");
	new SAction("txed_copy", "Copy", "copy", "Copy the selected texture(s)", "Ctrl+C");
	new SAction("txed_cut", "Cut", "cut", "Cut the selected texture(s)", "Ctrl+X");
	new SAction("txed_paste", "Paste", "paste", "Paste the previously copied texture(s)", "Ctrl+V");
	new SAction("txed_patch_add", "Add Patch", "patch_add", "Add a patch to the texture", "kb:txed_patch_add");
	new SAction("txed_patch_remove", "Remove Selected Patch(es)", "patch_remove", "Remove selected patch(es) from the texture", "kb:txed_patch_delete");
	new SAction("txed_patch_replace", "Replace Selected Patch(es)", "patch_replace", "Replace selected patch(es) with a different patch", "kb:txed_patch_replace");
	new SAction("txed_patch_back", "Send Selected Patch(es) Back", "patch_back", "Send selected patch(es) toward the back", "kb:txed_patch_back");
	new SAction("txed_patch_forward", "Bring Selected Patch(es) Forward", "patch_forward", "Bring selected patch(es) toward the front", "kb:txed_patch_forward");
	new SAction("txed_patch_duplicate", "Duplicate Selected Patch(es)", "patch_duplicate", "Duplicate the selected patch(es)", "kb:txed_patch_duplicate");

	// AnimatedEntryPanel
	new SAction("anim_new", "New Animation", "animation_new", "Create a new, dummy animation");
	new SAction("anim_delete", "Delete Animation", "animation_delete", "Deletes the selected animation(s) from the list");
	new SAction("anim_up", "Move Up", "up", "Move the selected animation(s) up in the list");
	new SAction("anim_down", "Move Down", "down", "Move the selected animation(s) down in the list");

	// SwitchesEntryPanel
	new SAction("swch_new", "New Switch", "switch_new", "Create a new, dummy switch");
	new SAction("swch_delete", "Delete Switch", "switch_delete", "Deletes the selected switch(es) from the list");
	new SAction("swch_up", "Move Up", "up", "Move the selected switch(es) up in the list");
	new SAction("swch_down", "Move Down", "down", "Move the selected switch(es) down in the list");

	// PaletteEntryPanel
	new SAction("ppal_addcustom", "Add to Custom Palettes", "plus", "Add the current palette to the custom palettes list");
	new SAction("ppal_test", "Test Palette", "palette_test", "Temporarily add the current palette to the palette chooser");
	new SAction("ppal_exportas", "Export As...", "export", "Export the current palette to a file");
	new SAction("ppal_importfrom", "Import From...", "import", "Import data from a file in the current palette");
	new SAction("ppal_colourise", "Colourise", "palette_colourise", "Colourise the palette");
	new SAction("ppal_tint", "Tint", "palette_tint", "Tint the palette");
	new SAction("ppal_tweak", "Tweak", "palette_tweak", "Tweak the palette");
	new SAction("ppal_invert", "Invert", "palette_invert", "Invert the palette");
	new SAction("ppal_gradient", "Gradient", "palette_gradient", "Add a gradient to the palette");
	new SAction("ppal_moveup", "Pull Ahead", "palette_pull", "Move this palette one rank towards the first");
	new SAction("ppal_movedown", "Push Back", "palette_push", "Move this palette one rank towards the last");
	new SAction("ppal_duplicate", "Duplicate", "palette_duplicate", "Create a copy of this palette at the end");
	new SAction("ppal_remove", "Remove", "palette_delete", "Erase this palette");
	new SAction("ppal_removeothers", "Remove Others", "palette_deleteothers", "Keep only this palette and erase all others");
	new SAction("ppal_report", "Write Report", "text", "Write an info report on this palette");
	new SAction("ppal_generate", "Generate Palettes", "palette", "Generate full range of palettes from the first");
	new SAction("ppal_colormap", "Generate Colormaps", "colormap", "Generate colormap lump from the first palette");

	// MapEntryPanel
	new SAction("pmap_open_text", "Edit Level Script", "text", "Open the map header as text (to edit fragglescript, etc.)");

	// DataEntryPanel
	new SAction("data_add_row", "Add Row", "plus", "Add a new row (after the currently selected row");
	new SAction("data_delete_row", "Delete Row(s)", "close", "Delete the currently selected row(s)");
	new SAction("data_cut_row", "Cut Row(s)", "cut", "Cut the currently selected row(s)", "Ctrl+X");
	new SAction("data_copy_row", "Copy Row(s)", "copy", "Copy the currently selected row(s)", "Ctrl+C");
	new SAction("data_paste_row", "Paste Row(s)", "paste", "Paste at the currently selected row", "Ctrl+V");
	new SAction("data_change_value", "Change Value...", "rename", "Change the value of the selected cell(s)");

	// TextEntryPanel
	new SAction("ptxt_wrap", "Word Wrapping", "", "Toggle word wrapping", "", SAction::Type::Check, -1, -1, 1, "txed_word_wrap");
	new SAction("ptxt_find_replace", "Find+Replace...", "", "Find and (optionally) replace text", "kb:ted_findreplace");
	new SAction("ptxt_fold_foldall", "Fold All", "minus", "Fold all possible code", "kb:ted_fold_foldall");
	new SAction("ptxt_fold_unfoldall", "Unfold All", "plus", "Unfold all folded code", "kb:ted_fold_unfoldall");
	new SAction("ptxt_jump_to_line", "Jump To Line...", "up", "Jump to a specific line number", "kb:ted_jumptoline");

	// Map Editor Window
	new SAction("mapw_save", "&Save Map Changes", "save", "Save any changes to the current map", "Ctrl+S");
	new SAction("mapw_saveas", "Save Map &As...", "saveas", "Save the map to a new wad archive", "Ctrl+Shift+S");
	new SAction("mapw_rename", "&Rename Map", "rename", "Rename the current map");
	new SAction("mapw_convert", "Con&vert Map...", "convert", "Convert the current map to a different format");
	new SAction("mapw_backup", "Restore Backup...", "undo", "Restore a previous backup of the current map");
	new SAction("mapw_undo", "Undo", "undo", "Undo", "Ctrl+Z");
	new SAction("mapw_redo", "Redo", "redo", "Redo", "Ctrl+Y");
	new SAction("mapw_setbra", "Set &Base Resource Archive", "archive", "Set the Base Resource Archive, to act as the program 'IWAD'");
	new SAction("mapw_preferences", "&Preferences...", "settings", "Setup SLADE options and preferences");
	int group_mode = SAction::newGroup();
	new SAction("mapw_mode_vertices", "Vertices Mode", "verts", "Change to vertices editing mode", "kb:me2d_mode_vertices", SAction::Type::Radio, -1, group_mode);
	new SAction("mapw_mode_lines", "Lines Mode", "lines", "Change to lines editing mode", "kb:me2d_mode_lines", SAction::Type::Radio, -1, group_mode);
	new SAction("mapw_mode_sectors", "Sectors Mode", "sectors", "Change to sectors editing mode", "kb:me2d_mode_sectors", SAction::Type::Radio, -1, group_mode);
	new SAction("mapw_mode_things", "Things Mode", "things", "Change to things editing mode", "kb:me2d_mode_things", SAction::Type::Radio, -1, group_mode);
	new SAction("mapw_mode_3d", "3d Mode", "3d", "Change to 3d editing mode", "kb:map_toggle_3d", SAction::Type::Radio, -1, group_mode);
	int group_flat_type = SAction::newGroup();
	new SAction("mapw_flat_none", "Wireframe", "flat_w", "Don't show flats (wireframe)", "", SAction::Type::Radio, -1, group_flat_type);
	new SAction("mapw_flat_untextured", "Untextured", "flat_u", "Show untextured flats", "", SAction::Type::Radio, -1, group_flat_type);
	new SAction("mapw_flat_textured", "Textured", "flat_t", "Show textured flats", "", SAction::Type::Radio, -1, group_flat_type);
	int group_sector_mode = SAction::newGroup();
	new SAction("mapw_sectormode_normal", "Normal (Both)", "sector_both", "Edit sector floors and ceilings", "", SAction::Type::Radio, -1, group_sector_mode);
	new SAction("mapw_sectormode_floor", "Floors", "sector_floor", "Edit sector floors", "", SAction::Type::Radio, -1, group_sector_mode);
	new SAction("mapw_sectormode_ceiling", "Ceilings", "sector_ceiling", "Edit sector ceilings", "", SAction::Type::Radio, -1, group_sector_mode);
	new SAction("mapw_showproperties", "&Item Properties", "properties", "Toggle the Item Properties window", "Ctrl+1");
	new SAction("mapw_showconsole", "&Console", "console", "Toggle the Console window", "Ctrl+2");
	new SAction("mapw_showundohistory", "&Undo History", "undo", "Toggle the Undo History window", "Ctrl+3");
	new SAction("mapw_showchecks", "Map Checks", "tick", "Toggle the Map Checks window", "Ctrl+4");
	new SAction("mapw_showscripteditor", "Script &Editor", "text", "Toggle the Script Editor window", "Ctrl+5");
	new SAction("mapw_run_map", "Run Map", "run", "Run the current map", "Ctrl+Shift+R");
	new SAction("mapw_draw_lines", "Draw Lines", "linedraw", "Begin line drawing", "kb:me2d_begin_linedraw");
	new SAction("mapw_draw_shape", "Draw Shape", "shapedraw", "Begin shape drawing", "kb:me2d_begin_shapedraw");
	new SAction("mapw_edit_objects", "Edit Object(s)", "objectedit", "Edit currently selected object(s)", "kb:me2d_begin_object_edit");
	new SAction("mapw_vertex_create", "Create Vertex Here", "", "Create a new vertex at the cursor position");
	new SAction("mapw_line_changetexture", "Change Texture", "", "Change the currently selected or hilighted line texture(s)", "kb:me2d_line_change_texture");
	new SAction("mapw_line_changespecial", "Change Special", "", "Change the currently selected or hilighted line special");
	new SAction("mapw_line_tagedit", "Edit Tagged", "", "Select sectors/things to tag to this line's special", "kb:me2d_line_tag_edit");
	new SAction("mapw_line_correctsectors", "Correct Sectors", "tick", "Correct line sector references");
	new SAction("mapw_line_flip", "Flip Line", "", "Flip the currently selected of hilighted line(s)", "kb:me2d_line_flip");
	new SAction("mapw_thing_changetype", "Change Type", "", "Change the currently selected or hilighted thing type(s)", "kb:me2d_thing_change_type");
	new SAction("mapw_thing_create", "Create Thing Here", "", "Create a new thing at the cursor position");
	new SAction("mapw_sector_create", "Create Sector Here", "", "Create a sector at the cursor position");
	new SAction("mapw_sector_changetexture", "Change Texture", "", "Change the currently selected or hilighted sector texture(s)", "kb:me2d_sector_change_texture");
	new SAction("mapw_sector_changespecial", "Change Special", "", "Change the currently selected or hilighted sector special(s)");
	new SAction("mapw_sector_join", "Merge Sectors", "", "Join the currently selected sectors together, removing unneeded lines", "kb:me2d_sector_join");
	new SAction("mapw_sector_join_keep", "Join Sectors", "", "Join the currently selected sectors together, keeping all lines", "kb:me2d_sector_join_keep");
	new SAction("mapw_item_properties", "Properties", "properties", "Edit the currently selected item's properties");
	new SAction("mapw_camera_set", "Move 3d Camera Here", "", "Set the current position of the 3d mode camera to the cursor position");
	new SAction("mapw_clear_selection", "Clear Selection", "", "Clear the current selection, if any", "kb:me2d_clear_selection");
	new SAction("mapw_show_fullmap", "Show Full Map", "", "Zooms out so that the full map is visible", "kb:me2d_show_all");
	new SAction("mapw_show_item", "Show Item...", "", "Zoom and scroll to show a map item");
	new SAction("mapw_mirror_y", "Mirror Vertically", "flip", "Mirror the selected objects vertically", "kb:me2d_mirror_y");
	new SAction("mapw_mirror_x", "Mirror Horizontally", "mirror", "Mirror the selected objects horizontally", "kb:me2d_mirror_x");
	new SAction("mapw_run_map_here", "Run Map from Here", "run", "Run the current map, starting at the current cursor position");

	// Script editor
	new SAction("mapw_script_save", "Save", "save", "Save changes to scripts");
	new SAction("mapw_script_compile", "Compile", "compile", "Compile scripts");
	new SAction("mapw_script_jumpto", "Jump To...", "up", "Jump to a specific script/function");
	new SAction("mapw_script_togglelanguage", "Show Language List", "properties", "Show/Hide the language list", "", SAction::Type::Check, -1, -1, 1, "script_show_language_list");
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
		MainAppFLClient* client = new MainAppFLClient();
		MainAppFLConnection* connection = (MainAppFLConnection*)client->MakeConnection(wxGetHostName(), "SLADE_MAFL", "files");

		if (connection)
		{
			// Send args as archives to open
			for (int a = 1; a < argc; a++)
			{
				string arg = argv[a];
				connection->Poke(arg, arg);
			}

			connection->Disconnect();
		}

		delete client;

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

	// Set locale to C so that the tokenizer will work properly
	// even in locales where the decimal separator is a comma.
	setlocale(LC_ALL, "C");

	// Init global variables
	Global::error = "";
	ArchiveManager::getInstance();
	init_ok = false;

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

	// Init application
	if (!App::init())
		return false;

	// Load image handlers
	wxInitAllImageHandlers();

	// Init logfile
	initLogFile();

	// Get Windows version
#ifdef __WXMSW__
	wxGetOsVersion(&Global::win_version_major, &Global::win_version_minor);
	LOG_MESSAGE(0, "Windows Version: %d.%d", Global::win_version_major, Global::win_version_minor);
#endif

	// Init keybinds
	KeyBind::initBinds();

	// Load configuration file
	wxLogMessage("Loading configuration");
	readConfigFile();
	Global::log_verbosity = log_verbosity;

	// Check that SLADE.pk3 can be found
	wxLogMessage("Loading resources");
	theArchiveManager->init();
	if (!theArchiveManager->resArchiveOK())
	{
		wxMessageBox("Unable to find slade.pk3, make sure it exists in the same directory as the SLADE executable", "Error", wxICON_ERROR);
		return false;
	}

	// Init lua
	Lua::init();

	// Calculate scaling factor (from system ppi)
	wxMemoryDC dc;
	Global::ppi_scale = (double)(dc.GetPPI().x) / 96.0;

	// Show splash screen
	SplashWindow::getInstance()->init();
	UI::showSplash("Starting up...");

	// Init SImage formats
	SIFormat::initFormats();

	// Load program icons
	wxLogMessage("Loading icons");
	Icons::loadIcons();

	// Load program fonts
	Drawing::initFonts();

	// Load entry types
	wxLogMessage("Loading entry types");
	EntryDataFormat::initBuiltinFormats();
	EntryType::loadEntryTypes();

	// Load text languages
	wxLogMessage("Loading text languages");
	TextLanguage::loadLanguages();

	// Init text stylesets
	wxLogMessage("Loading text style sets");
	StyleSet::loadResourceStyles();
	StyleSet::loadCustomStyles();

	// Init colour configuration
	wxLogMessage("Loading colour configuration");
	ColourConfiguration::init();

	// Init nodebuilders
	NodeBuilders::init();

	// Init game executables
	Executables::init();

	// Init actions
	initActions();

	// Init main editor
	MainEditor::init();

	// Init base resource
	wxLogMessage("Loading base resource");
	theArchiveManager->initBaseResource();
	wxLogMessage("Base resource loaded");

	// Show the main window
	theMainWindow->Show(true);
	SetTopWindow(theMainWindow);
	SplashWindow::getInstance()->SetParent(theMainWindow);
	SplashWindow::getInstance()->CentreOnParent();

	// Open any archives on the command line
	// argv[0] is normally the executable itself (i.e. Slade.exe)
	// and opening it as an archive should not be attempted...
	for (int a = 1; a < argc; a++)
	{
		string arg = argv[a];
		theArchiveManager->openArchive(arg);
	}

	// Hide splash screen
	UI::hideSplash();

	init_ok = true;
	wxLogMessage("SLADE Initialisation OK");

	// Init game configuration
	theGameConfiguration->init();

	// Show Setup Wizard if needed
	if (!setup_wizard_run)
	{
		SetupWizardDialog dlg(theMainWindow);
		dlg.ShowModal();
		setup_wizard_run = true;
		theMainWindow->Update();
		theMainWindow->Refresh();
	}

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
	exiting = true;

	if (save_config)
	{
		// Save configuration
		saveConfigFile();

		// Save text style configuration
		StyleSet::saveCurrent();

		// Save colour configuration
		MemChunk ccfg;
		ColourConfiguration::writeConfiguration(ccfg);
		ccfg.exportFile(App::path("colours.cfg", App::Dir::User));

		// Save game exes
		wxFile f;
		f.Open(App::path("executables.cfg", App::Dir::User), wxFile::write);
		f.Write(Executables::writeExecutables());
		f.Close();
	}

	// Close the map editor if it's open
	MapEditorWindow::deleteInstance();

	// Close all open archives
	theArchiveManager->closeAll();

	// Clean up
	EntryType::cleanupEntryTypes();
	ArchiveManager::deleteInstance();
	Console::deleteInstance();
	SplashWindow::deleteInstance();
	delete single_instance_checker;
	delete file_listener;

	// Clear temp folder
	wxDir temp;
	temp.Open(App::path("", App::Dir::Temp));
	string filename = wxEmptyString;
	bool files = temp.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
	while (files)
	{
		if (!wxRemoveFile(App::path(filename, App::Dir::Temp)))
			wxLogMessage("Warning: Could not clean up temporary file \"%s\"", filename);
		files = temp.GetNext(&filename);
	}

	// Close lua
	Lua::close();

	// Close DUMB
	dumb_exit();

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

/* SLADEWxApp::readConfigFile
 * Reads and parses the SLADE configuration file
 *******************************************************************/
void SLADEWxApp::readConfigFile()
{
	// Open SLADE.cfg
	Tokenizer tz;
	if (!tz.openFile(App::path("slade3.cfg", App::Dir::User)))
		return;

	// Go through the file with the tokenizer
	string token = tz.getToken();
	while (!tz.atEnd())
	{
		// If we come across a 'cvars' token, read in the cvars section
		if (!token.Cmp("cvars"))
		{
			token = tz.getToken();	// Skip '{'

			// Keep reading name/value pairs until we hit the ending '}'
			string cvar_name = tz.getToken();
			while (cvar_name.Cmp("}") && !tz.atEnd())
			{
				string cvar_val = tz.getToken();
				read_cvar(cvar_name, cvar_val);
				cvar_name = tz.getToken();
			}
		}

		// Read base resource archive paths
		if (!token.Cmp("base_resource_paths"))
		{
			// Skip {
			token = wxString::FromUTF8(UTF8(tz.getToken()));

			// Read paths until closing brace found
			token = tz.getToken();
			while (token.Cmp("}") && !tz.atEnd())
			{
				theArchiveManager->addBaseResourcePath(token);
				token = wxString::FromUTF8(UTF8(tz.getToken()));
			}
		}

		// Read recent files list
		if (token == "recent_files")
		{
			// Skip {
			token = tz.getToken();

			// Read files until closing brace found
			token = wxString::FromUTF8(UTF8(tz.getToken()));
			while (token != "}" && !tz.atEnd())
			{
				theArchiveManager->addRecentFile(token);
				token = wxString::FromUTF8(UTF8(tz.getToken()));
			}
		}

		// Read keybinds
		if (token == "keys")
		{
			token = tz.getToken();	// Skip {
			KeyBind::readBinds(tz);
		}

		// Read nodebuilder paths
		if (token == "nodebuilder_paths")
		{
			token = tz.getToken();	// Skip {

			// Read paths until closing brace found
			token = tz.getToken();
			while (token != "}" && !tz.atEnd())
			{
				string path = tz.getToken();
				NodeBuilders::addBuilderPath(token, path);
				token = tz.getToken();
			}
		}

		// Read game exe paths
		if (token == "executable_paths")
		{
			token = tz.getToken();	// Skip {

			// Read paths until closing brace found
			token = tz.getToken();
			while (token != "}" && !tz.atEnd())
			{
				if (token.length())
				{
					string path = tz.getToken();
					Executables::setGameExePath(token, path);
				}
				token = tz.getToken();
			}
		}

		// Read window size/position info
		if (token == "window_info")
		{
			token = tz.getToken();	// Skip {
			Misc::readWindowInfo(&tz);
		}

		// Get next token
		token = tz.getToken();
	}
}

/* SLADEWxApp::saveConfigFile
 * Saves the SLADE configuration file
 *******************************************************************/
void SLADEWxApp::saveConfigFile()
{
	// Open SLADE.cfg for writing text
	wxFile file(App::path("slade3.cfg", App::Dir::User), wxFile::write);

	// Do nothing if it didn't open correctly
	if (!file.IsOpened())
		return;

	// Write cfg header
	file.Write("/*****************************************************\n");
	file.Write(" * SLADE Configuration File\n");
	file.Write(" * Don't edit this unless you know what you're doing\n");
	file.Write(" *****************************************************/\n\n");

	// Write cvars
	save_cvars(file);

	// Write base resource archive paths
	file.Write("\nbase_resource_paths\n{\n");
	for (size_t a = 0; a < theArchiveManager->numBaseResourcePaths(); a++)
	{
		string path = theArchiveManager->getBaseResourcePath(a);
		path.Replace("\\", "/");
		file.Write(S_FMT("\t\"%s\"\n", path), wxConvUTF8);
	}
	file.Write("}\n");

	// Write recent files list (in reverse to keep proper order when reading back)
	file.Write("\nrecent_files\n{\n");
	for (int a = theArchiveManager->numRecentFiles() - 1; a >= 0; a--)
	{
		string path = theArchiveManager->recentFile(a);
		path.Replace("\\", "/");
		file.Write(S_FMT("\t\"%s\"\n", path), wxConvUTF8);
	}
	file.Write("}\n");

	// Write keybinds
	file.Write("\nkeys\n{\n");
	file.Write(KeyBind::writeBinds());
	file.Write("}\n");

	// Write nodebuilder paths
	file.Write("\n");
	NodeBuilders::saveBuilderPaths(file);

	// Write game exe paths
	file.Write("\nexecutable_paths\n{\n");
	file.Write(Executables::writePaths());
	file.Write("}\n");

	// Write window info
	file.Write("\nwindow_info\n{\n");
	Misc::writeWindowInfo(file);
	file.Write("}\n");

	// Close configuration file
	file.Write("\n// End Configuration File\n\n");
}

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

/* SLADEWxApp::exitApp
 * Exits the application. If [save_config] is fale, no configuration
 * files will be saved (slade.cfg, executables.cfg, etc.)
 *******************************************************************/
void SLADEWxApp::exitApp(bool save_config)
{
	this->save_config = save_config;
	theMainWindow->Close();
}

/* SLADEWxApp::doAction
 * Performs the SAction matching [id]
 *******************************************************************/
bool SLADEWxApp::doAction(string id, int wx_id_offset)
{
	// Toggle action if necessary
	SAction::fromId(id)->toggle();

	// Send action to all handlers
	bool handled = false;
	for (unsigned a = 0; a < action_handlers.size(); a++)
	{
		action_handlers[a]->wx_id_offset = wx_id_offset;
		if (action_handlers[a]->handleAction(id))
		{
			handled = true;
			break;
		}
	}

	// Warn if nothing handled it
	if (!handled)
		wxLogMessage("Warning: Action \"%s\" not handled", id);

	// Log action (to log file only)
	exiting = true;
	wxLogMessage("**** Action \"%s\"", id);
	exiting = false;

	// Return true if handled
	return handled;
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
		handled = doAction(action, e.GetId() - s_action->getWxId());

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
	if (!e.GetActive())
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
		uint8_t* test = NULL;
		test[123] = 5;
	}
}

CONSOLE_COMMAND(setup_wizard, 0, false)
{
	SetupWizardDialog dlg(theMainWindow);
	dlg.ShowModal();
}

CONSOLE_COMMAND(quit, 0, true)
{
	bool save_config = true;
	for (unsigned a = 0; a < args.size(); a++)
	{
		if (args[a].Lower() == "nosave")
			save_config = false;
	}

	theApp->exitApp(save_config);
}
