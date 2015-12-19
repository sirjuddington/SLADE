/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MainApp.cpp
 * Description: MainApp class functions.
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
#include "WxStuff.h"
#include "MainApp.h"
#include "MainWindow.h"
#include "ArchiveManager.h"
#include "Tokenizer.h"
#include "Console.h"
#include "Icons.h"
#include "SplashWindow.h"
#include "EntryDataFormat.h"
#include "TextLanguage.h"
#include "TextStyle.h"
#include "ResourceManager.h"
#include "SIFormat.h"
#include "KeyBind.h"
#include "ColourConfiguration.h"
#include "Drawing.h"
#include "MapEditorWindow.h"
#include "GameConfiguration.h"
#include "NodeBuilders.h"
#include "Lua.h"
#include "Dialogs/SetupWizard/SetupWizardDialog.h"
#include "Executables.h"
#include "Misc.h"
#include "VersionCheck.h"
#include "dumb/dumb.h"
#include <wx/image.h>
#include <wx/stdpaths.h>
#include <wx/ffile.h>
#include <wx/stackwalk.h>
#include <wx/dir.h>
#include <wx/sysopt.h>
#include <wx/filename.h>
#include <wx/protocol/http.h>

#undef BOOL
#include <FreeImage.h>

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
	int version_num = 3105;
	string version = "3.1.0.5";
	
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
}

string	dir_data = "";
string	dir_user = "";
string	dir_app = "";
bool	exiting = false;
string	current_action = "";
bool	update_check_message_box = false;
CVAR(String, dir_last, "", CVAR_SAVE)
CVAR(Int, log_verbosity, 1, CVAR_SAVE)
CVAR(Int, temp_location, 0, CVAR_SAVE)
CVAR(String, temp_location_custom, "", CVAR_SAVE)
CVAR(Bool, setup_wizard_run, false, CVAR_SAVE)
CVAR(Bool, update_check, true, CVAR_SAVE)
CVAR(Bool, update_check_beta, false, CVAR_SAVE)


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, map_show_selection_numbers)


/*******************************************************************
 * CLASSES
 *******************************************************************/

/* SLADEStackTrace class
 * Extension of the wxStackWalker class that formats stack trace
 * information to a multi-line string, that can be retrieved via
 * getTraceString(). wxStackWalker is currently unimplemented on mac,
 * so unfortunately it has to be disabled there
 *******************************************************************/
#if !defined(__APPLE__) && !defined(__FreeBSD__)
class SLADEStackTrace : public wxStackWalker
{
private:
	string	stack_trace;

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

	void OnStackFrame(const wxStackFrame& frame)
	{
		string location = "[unknown location] ";
		if (frame.HasSourceLocation())
			location = S_FMT("(%s:%d) ", frame.GetFileName(), frame.GetLine());

		wxUIntPtr address = wxPtrToUInt(frame.GetAddress());
		string func_name = frame.GetName();
		if (func_name.IsEmpty())
			func_name = S_FMT("[unknown:%d]", address);

		stack_trace.Append(S_FMT("%d: %s%s\n", frame.GetLevel(), location, func_name));
	}
};


/* SLADECrashDialog class
 * A simple dialog that displays a crash message and a scrollable,
 * multi-line textbox with a stack trace
 *******************************************************************/
class SLADECrashDialog : public wxDialog
{
private:
	wxTextCtrl*	text_stack;

public:
	SLADECrashDialog(SLADEStackTrace& st) : wxDialog(wxTheApp->GetTopWindow(), -1, "SLADE3 Application Crash")
	{
		// Setup sizer
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		// Add general crash method
		string message = "SLADE3 has crashed unexpectedly. To help fix the problem that caused this crash,\nplease copy+paste the information from the window below to a text file, and email\nit to <sirjuddington@gmail.com> along with a description of what you were\ndoing at the time of the crash. Sorry for the inconvenience.";
		sizer->Add(new wxStaticText(this, -1, message), 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 4);

		// Setup stack trace string
		string trace = S_FMT("Version: %s\n", Global::version);
		if (current_action.IsEmpty())
			trace += "No current action\n";
		else
			trace += S_FMT("Current action: %s", current_action);
		trace += "\n";
		trace += st.getTraceString();

		// Add stack trace text area
		text_stack = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL);
		text_stack->SetValue(trace);
		text_stack->SetFont(wxFont(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
		sizer->Add(text_stack, 1, wxEXPAND|wxALL, 4);

		// Dump stack trace to a file (just in case)
		wxFile file(appPath("slade3_crash.log", DIR_USER), wxFile::write);
		file.Write(trace);
		file.Close();

		// Add standard 'OK' button
		sizer->Add(CreateStdDialogButtonSizer(wxOK), 0, wxEXPAND|wxALL, 4);

		// Setup layout
		Layout();
		SetInitialSize(wxSize(500, 500));
	}

	~SLADECrashDialog() {}
};
#endif//!__APPLE__ && !__FreeBSD__

/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

/* appPath
 * Prepends an application-related path to a filename,
 * DIR_DATA: SLADE application data directory (for SLADE.pk3)
 * DIR_USER: User configuration and resources directory
 * DIR_APP: Directory of the SLADE executable
 * DIR_TEMP: Temporary files directory
 *******************************************************************/
int temp_fail_count = 0;
string appPath(string filename, int dir)
{
	// Setup separator character
#ifdef WIN32
	string sep = "\\";
#else
	string sep = "/";
#endif

	if (dir == DIR_DATA)
		return dir_data + sep + filename;
	else if (dir == DIR_USER)
		return dir_user + sep + filename;
	else if (dir == DIR_APP)
		return dir_app + sep + filename;
	else if (dir == DIR_TEMP)
	{
		// Get temp path
		string dir_temp;
		if (temp_location == 0)
			dir_temp = wxStandardPaths::Get().GetTempDir().Append(sep).Append("SLADE3");
		else if (temp_location == 1)
			dir_temp = dir_temp = dir_app + sep + "temp";
		else
			dir_temp = temp_location_custom;

		// Create folder if necessary
		if (!wxDirExists(dir_temp) && temp_fail_count < 2)
		{
			if (!wxMkdir(dir_temp))
			{
				wxLogMessage("Unable to create temp directory \"%s\"", dir_temp);
				temp_fail_count++;
				return appPath(filename, dir);
			}
		}

		return dir_temp + sep + filename;
	}
	else
		return filename;
}


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
 * MAINAPP CLASS FUNCTIONS
 *******************************************************************/
IMPLEMENT_APP(MainApp)

/* MainApp::MainApp
 * MainApp class constructor
 *******************************************************************/
MainApp::MainApp()
{
	main_window = NULL;
	cur_id = 26000;
	action_invalid = NULL;
	init_ok = false;
}

/* MainApp::~MainApp
 * MainApp class destructor
 *******************************************************************/
MainApp::~MainApp()
{
}

/* MainApp::initDirectories
 * Checks for and creates necessary application directories. Returns
 * true if all directories existed and were created successfully if
 * needed, false otherwise
 *******************************************************************/
bool MainApp::initDirectories()
{
	// Setup separator character
#ifdef WIN32
	string sep = "\\";
#else
	string sep = "/";
#endif

	// Setup app dir
	dir_app = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();

	// Check for portable install
	if (wxFileExists(appPath("portable", DIR_APP)))
	{
		// Setup portable user/data dirs
		dir_data = dir_app;
		dir_user = dir_app + sep + "config";
	}
	else
	{
		// Setup standard user/data dirs
		dir_user = wxStandardPaths::Get().GetUserDataDir();
		dir_data = wxStandardPaths::Get().GetDataDir();
	}

	// Create user dir if necessary
	if (!wxDirExists(dir_user))
	{
		if (!wxMkdir(dir_user))
		{
			wxMessageBox(S_FMT("Unable to create user directory \"%s\"", dir_user), "Error", wxICON_ERROR);
			return false;
		}
	}

	// Check data dir
	if (!wxDirExists(dir_data))
		dir_data = dir_app;	// Use app dir if data dir doesn't exist

	return true;
}

/* MainApp::initLogFile
 * Sets up the SLADE log file
 *******************************************************************/
void MainApp::initLogFile()
{
	// Set wxLog target(s)
	wxLog::SetActiveTarget(new SLADELog());
	FILE* log_file = fopen(CHR(appPath("slade3.log", DIR_USER)), "wt");
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

/* MainApp::initActions
 * Sets up all menu/toolbar actions
 *******************************************************************/
void MainApp::initActions()
{
	// MainWindow
	new SAction("main_exit", "E&xit", "t_exit", "Quit SLADE", "", 0, wxID_EXIT);
	new SAction("main_undo", "Undo", "t_undo", "Undo", "Ctrl+Z");
	new SAction("main_redo", "Redo", "t_redo", "Redo", "Ctrl+Y");
	new SAction("main_setbra", "Set &Base Resource Archive", "e_archive", "Set the Base Resource Archive, to act as the program 'IWAD'");
	new SAction("main_preferences", "&Preferences...", "t_settings", "Setup SLADE options and preferences", "", NORMAL, wxID_PREFERENCES);
	new SAction("main_showam", "&Archive Manager", "e_archive", "Toggle the Archive Manager window", "Ctrl+1");
	new SAction("main_showconsole", "&Console", "t_console", "Toggle the Console window", "Ctrl+2");
	new SAction("main_showundohistory", "&Undo History", "t_undo", "Toggle the Undo History window", "Ctrl+3");
	new SAction("main_onlinedocs", "Online &Documentation", "t_wiki", "View SLADE documentation online");
	new SAction("main_about", "&About", "i_logo", "Informaton about SLADE", "", 0, wxID_ABOUT);
	new SAction("main_updatecheck", "Check for Updates...", "", "Check online for updates");

	// ArchiveManagerPanel
	new SAction("aman_newwad", "New Wad Archive", "t_newarchive", "Create a new Doom Wad Archive", "Ctrl+Shift+W");
	new SAction("aman_newzip", "New Zip Archive", "t_newzip", "Create a new Zip Archive", "Ctrl+Shift+Z");
	new SAction("aman_newmap", "New Map", "t_mapeditor", "Create a new standalone map", "Ctrl+Shift+M");
	new SAction("aman_open", "&Open", "t_open", "Open an existing Archive", "Ctrl+O");
	new SAction("aman_opendir", "Open &Directory", "t_opendir", "Open a directory as an Archive");
	new SAction("aman_save", "&Save", "t_save", "Save the currently open Archive", "Ctrl+S");
	new SAction("aman_saveas", "Save &As", "t_saveas", "Save the currently open Archive to a new file", "Ctrl+Shift+S");
	new SAction("aman_saveall", "Save All", "t_saveall", "Save all open Archives");
	new SAction("aman_close", "&Close", "t_close", "Close the currently open Archive", "Ctrl+W");
	new SAction("aman_closeall", "Close All", "t_closeall", "Close all open Archives");
	new SAction("aman_recent_open", "Open", "t_open", "Open the selected Archive(s)");
	new SAction("aman_recent_remove", "Remove", "t_close", "Remove the selected Archive(s) from the recent list");
	new SAction("aman_bookmark_go", "Go To", "t_open", "Go to the selected bookmark");
	new SAction("aman_bookmark_remove", "Remove", "t_close", "Remove the selected bookmark(s) from the list");
	new SAction("aman_save_a", "&Save", "t_save", "Save the selected Archive", "Ctrl+S");
	new SAction("aman_saveas_a", "Save &As", "t_saveas", "Save the selected Archive to a new file", "Ctrl+Shift+S");
	new SAction("aman_close_a", "&Close", "t_close", "Close the selected Archive", "Ctrl+W");

	// Recent files
	new SAction("aman_recent1", "<insert recent file name>", "");
	new SAction("aman_recent2", "<insert recent file name>", "");
	new SAction("aman_recent3", "<insert recent file name>", "");
	new SAction("aman_recent4", "<insert recent file name>", "");
	new SAction("aman_recent5", "<insert recent file name>", "");
	new SAction("aman_recent6", "<insert recent file name>", "");
	new SAction("aman_recent7", "<insert recent file name>", "");
	new SAction("aman_recent8", "<insert recent file name>", "");
	new SAction("aman_recent9", "<insert recent file name>", "");
	new SAction("aman_recent10", "<insert recent file name>", "");
	new SAction("aman_recent11", "<insert recent file name>", "");
	new SAction("aman_recent12", "<insert recent file name>", "");
	new SAction("aman_recent13", "<insert recent file name>", "");
	new SAction("aman_recent14", "<insert recent file name>", "");
	new SAction("aman_recent15", "<insert recent file name>", "");
	new SAction("aman_recent16", "<insert recent file name>", "");
	new SAction("aman_recent17", "<insert recent file name>", "");
	new SAction("aman_recent18", "<insert recent file name>", "");
	new SAction("aman_recent19", "<insert recent file name>", "");
	new SAction("aman_recent20", "<insert recent file name>", "");

	// ArchivePanel
	new SAction("arch_newentry", "New Entry", "t_newentry", "Create a new empty entry");
	new SAction("arch_newpalette", "New PLAYPAL", "e_palette", "Create a new palette entry");
	new SAction("arch_newanimated", "New ANIMATED", "t_animation", "Create a new Boom ANIMATED entry");
	new SAction("arch_newswitches", "New SWITCHES", "t_switch", "Create a new Boom SWITCHES entry");
	new SAction("arch_newdir", "New Directory", "t_newfolder", "Create a new empty directory");
	new SAction("arch_importfiles", "&Import Files", "t_importfiles", "Import multiple files into the archive", "kb:el_import_files");
	new SAction("arch_buildarchive", "&Build Archive", "t_buildarchive", "Build archive from the current directory", "kb:el_build_archive");
	new SAction("arch_texeditor", "&Texture Editor", "t_texeditor", "Open the texture editor for the current archive");
	new SAction("arch_mapeditor", "&Map Editor", "t_mapeditor", "Open the map editor");
	new SAction("arch_clean_patches", "Remove Unused &Patches", "", "Remove any unused patches, and their associated entries");
	new SAction("arch_clean_textures", "Remove Unused &Textures", "", "Remove any unused textures");
	new SAction("arch_clean_flats", "Remove Unused &Flats", "", "Remove any unused flats");
	new SAction("arch_check_duplicates", "Check Duplicate Entry Names", "", "Checks the archive for any entries sharing the same name");
	new SAction("arch_check_duplicates2", "Check Duplicate Entry Content", "", "Checks the archive for any entries sharing the same data");
	new SAction("arch_clean_iwaddupes", "Remove Entries Duplicated from IWAD", "", "Remove entries that are exact duplicates of entries from the base resource archive");
	new SAction("arch_replace_maps", "Replace in Maps", "", "Tool to find and replace thing types, specials and textures in all maps");
	new SAction("arch_entry_rename", "Rename", "t_rename", "Rename the selected entries", "kb:el_rename");
	new SAction("arch_entry_rename_each", "Rename Each", "t_renameeach", "Rename separately all the selected entries");
	new SAction("arch_entry_delete", "Delete", "t_delete", "Delete the selected entries");
	new SAction("arch_entry_revert", "Revert", "t_revert", "Revert any modifications made to the selected entries since the last save");
	new SAction("arch_entry_cut", "Cut", "t_cut", "Cut the selected entries");
	new SAction("arch_entry_copy", "Copy", "t_copy", "Copy the selected entries");
	new SAction("arch_entry_paste", "Paste", "t_paste", "Paste the selected entries");
	new SAction("arch_entry_moveup", "Move Up", "t_up", "Move the selected entries up", "kb:el_move_up");
	new SAction("arch_entry_movedown", "Move Down", "t_down", "Move the selected entries down", "kb:el_move_down");
	new SAction("arch_entry_sort", "Sort", "t_down", "Sort the entries in the list");
	new SAction("arch_entry_import", "Import", "t_import", "Import a file to the selected entry", "kb:el_import");
	new SAction("arch_entry_export", "Export", "t_export", "Export the selected entries to files", "kb:el_export");
	new SAction("arch_entry_bookmark", "Bookmark", "t_bookmark", "Bookmark the current entry");
	new SAction("arch_entry_opentab", "Open in Tab", "t_open", "Open selected entries in separate tabs");
	new SAction("arch_entry_crc32", "Compute CRC-32 Checksum", "e_text", "Compute the CRC-32 checksums of the selected entries");
	new SAction("arch_bas_convertb", "Convert to SWANTBLS", "", "Convert any selected SWITCHES and ANIMATED entries to a single SWANTBLS entry");
	new SAction("arch_bas_convertz", "Convert to ANIMDEFS", "", "Convert any selected SWITCHES and ANIMATED entries to a single ANIMDEFS entry");
	new SAction("arch_swan_convert", "Compile to SWITCHES and ANIMATED", "", "Convert SWANTBLS entries into SWITCHES and ANIMATED entries");
	new SAction("arch_texturex_convertzd", "Convert to TEXTURES", "", "Convert any selected TEXTUREx entries to ZDoom TEXTURES format");
	new SAction("arch_view_text", "View as Text", "e_text", "Open the selected entry in the text editor, regardless of type");
	new SAction("arch_view_hex", "View as Hex", "e_data", "Open the selected entry in the hex editor, regardless of type");
	new SAction("arch_gfx_convert", "Convert to...", "t_convert", "Open the Gfx Conversion Dialog for any selected gfx entries");
	new SAction("arch_gfx_translate", "Colour Remap...", "t_remap", "Remap a range of colours in the selected gfx entries to another range (paletted gfx only)");
	new SAction("arch_gfx_colourise", "Colourise", "t_colourise", "Colourise the selected gfx entries");
	new SAction("arch_gfx_tint", "Tint", "t_tint", "Tint the selected gfx entries by a colour/amount");
	new SAction("arch_gfx_offsets", "Modify Gfx Offsets", "t_offset", "Mass-modify the offsets for any selected gfx entries");
	new SAction("arch_gfx_addptable", "Add to Patch Table", "e_pnames", "Add selected gfx entries to PNAMES");
	new SAction("arch_gfx_addtexturex", "Add to TEXTUREx", "e_texturex", "Create textures from selected gfx entries and add them to TEXTUREx");
	new SAction("arch_gfx_exportpng", "Export as PNG", "t_export", "Export selected gfx entries to PNG format files");
	new SAction("arch_gfx_pngopt", "Optimize PNG", "t_pngopt", "Optimize PNG entries");
	new SAction("arch_audio_convertwd", "Convert WAV to Doom Sound", "t_convert", "Convert any selected WAV format entries to Doom Sound format");
	new SAction("arch_audio_convertdw", "Convert Doom Sound to WAV", "t_convert", "Convert any selected Doom Sound format entries to WAV format");
	new SAction("arch_audio_convertmus", "Convert MUS to MIDI", "t_convert", "Convert any selected MUS format entries to MIDI format");
	new SAction("arch_scripts_compileacs", "Compile ACS", "t_compile", "Compile any selected text entries to ACS bytecode");
	new SAction("arch_scripts_compilehacs", "Compile ACS (Hexen bytecode)", "t_compile2", "Compile any selected text entries to Hexen-compatible ACS bytecode");
	new SAction("arch_map_opendb2", "Open Map in Doom Builder 2", "", "Open the selected map in Doom Builder 2");
	new SAction("arch_run", "Run Archive", "t_run", "Run the current archive", "Ctrl+Shift+R");

	// GfxEntryPanel
	new SAction("pgfx_mirror", "Mirror", "t_mirror", "Mirror the graphic horizontally");
	new SAction("pgfx_flip", "Flip", "t_flip", "Flip the graphic vertically");
	new SAction("pgfx_rotate", "Rotate", "t_rotate", "Rotate the graphic");
	new SAction("pgfx_translate", "Colour Remap", "t_remap", "Remap a range of colours in the graphic to another range (paletted gfx only)");
	new SAction("pgfx_colourise", "Colourise", "t_colourise", "Colourise the graphic");
	new SAction("pgfx_tint", "Tint", "t_tint", "Tint the graphic by a colour/amount");
	new SAction("pgfx_alph", "alPh Chunk", "", "Add/Remove alPh chunk to/from the PNG", "", SAction::CHECK);
	new SAction("pgfx_trns", "tRNS Chunk", "", "Add/Remove tRNS chunk to/from the PNG", "", SAction::CHECK);
	new SAction("pgfx_extract", "Extract All", "", "Extract all images in this entry to separate PNGs");
	new SAction("pgfx_crop", "Crop", "t_settings", "Crop the graphic");
	new SAction("pgfx_convert", "Convert to...", "t_convert", "Open the Gfx Conversion Dialog for the entry");
	new SAction("pgfx_pngopt", "Optimize PNG", "t_pngopt", "Optimize PNG entry");

	// ArchiveEntryList
	new SAction("aelt_sizecol", "Size", "", "Show the size column", "", SAction::CHECK);
	new SAction("aelt_typecol", "Type", "", "Show the type column", "", SAction::CHECK);
	new SAction("aelt_indexcol", "Index", "", "Show the index column", "", SAction::CHECK);
	new SAction("aelt_hrules", "Horizontal Rules", "", "Show horizontal rules between entries", "", SAction::CHECK);
	new SAction("aelt_vrules", "Vertical Rules", "", "Show vertical rules between columns", "", SAction::CHECK);
	new SAction("aelt_bgcolour", "Colour by Type", "", "Colour item background by entry type", "", SAction::CHECK);
	new SAction("aelt_bgalt", "Alternating Row Colour", "", "Show alternating row colours", "", SAction::CHECK);

	// TextureEditorPanel
	new SAction("txed_new", "New Texture", "t_tex_new", "Create a new, empty texture", "kb:txed_tex_new");
	new SAction("txed_delete", "Delete Texture", "t_tex_delete", "Deletes the selected texture(s) from the list", "kb:txed_tex_delete");
	new SAction("txed_new_patch", "New Texture from Patch", "t_tex_newpatch", "Create a new texture from an existing patch", "kb:txed_tex_new_patch");
	new SAction("txed_new_file", "New Texture from File", "t_tex_newfile", "Create a new texture from an image file", "kb:txed_tex_new_file");
	new SAction("txed_rename", "Rename Texture", "t_tex_rename", "Rename the selected texture(s)");
	new SAction("txed_rename_each", "Rename Each", "t_tex_renameeach", "Rename separately all the selected textures");
	new SAction("txed_export", "Export Texture", "t_tex_export", "Create standalone images from the selected texture(s)");
	new SAction("txed_extract", "Extract Texture", "t_tex_extract", "Export the selected texture(s) as PNG files");
	new SAction("txed_offsets", "Modify Offsets", "t_tex_offset", "Mass modify offsets in the selected texture(s)");
	new SAction("txed_up", "Move Up", "t_up", "Move the selected texture(s) up in the list", "kb:txed_tex_up");
	new SAction("txed_down", "Move Down", "t_down", "Move the selected texture(s) down in the list", "kb:txed_tex_down");
	new SAction("txed_sort", "Sort", "t_down", "Sort the textures in the list");
	new SAction("txed_copy", "Copy", "t_copy", "Copy the selected texture(s)", "Ctrl+C");
	new SAction("txed_cut", "Cut", "t_cut", "Cut the selected texture(s)", "Ctrl+X");
	new SAction("txed_paste", "Paste", "t_paste", "Paste the previously copied texture(s)", "Ctrl+V");
	new SAction("txed_patch_add", "Add Patch", "t_patch_add", "Add a patch to the texture", "kb:txed_patch_add");
	new SAction("txed_patch_remove", "Remove Selected Patch(es)", "t_patch_remove", "Remove selected patch(es) from the texture", "kb:txed_patch_delete");
	new SAction("txed_patch_replace", "Replace Selected Patch(es)", "t_patch_replace", "Replace selected patch(es) with a different patch", "kb:txed_patch_replace");
	new SAction("txed_patch_back", "Send Selected Patch(es) Back", "t_patch_back", "Send selected patch(es) toward the back", "kb:txed_patch_back");
	new SAction("txed_patch_forward", "Bring Selected Patch(es) Forward", "t_patch_forward", "Bring selected patch(es) toward the front", "kb:txed_patch_forward");
	new SAction("txed_patch_duplicate", "Duplicate Selected Patch(es)", "t_patch_duplicate", "Duplicate the selected patch(es)", "kb:txed_patch_duplicate");

	// AnimatedEntryPanel
	new SAction("anim_new", "New Animation", "t_animation_new", "Create a new, dummy animation");
	new SAction("anim_delete", "Delete Animation", "t_animation_delete", "Deletes the selected animation(s) from the list");
	new SAction("anim_up", "Move Up", "t_up", "Move the selected animation(s) up in the list");
	new SAction("anim_down", "Move Down", "t_down", "Move the selected animation(s) down in the list");

	// SwitchesEntryPanel
	new SAction("swch_new", "New Switch", "t_switch_new", "Create a new, dummy switch");
	new SAction("swch_delete", "Delete Switch", "t_switch_delete", "Deletes the selected switch(es) from the list");
	new SAction("swch_up", "Move Up", "t_up", "Move the selected switch(es) up in the list");
	new SAction("swch_down", "Move Down", "t_down", "Move the selected switch(es) down in the list");

	// PaletteEntryPanel
	new SAction("ppal_addcustom", "Add to Custom Palettes", "t_plus", "Add the current palette to the custom palettes list");
	new SAction("ppal_test", "Test Palette", "t_palette_test", "Temporarily add the current palette to the palette chooser");
	new SAction("ppal_exportas", "Export As...", "t_export", "Export the current palette to a file");
	new SAction("ppal_importfrom", "Import From...", "t_import", "Import data from a file in the current palette");
	new SAction("ppal_colourise", "Colourise", "t_palette_colourise", "Colourise the palette");
	new SAction("ppal_tint", "Tint", "t_palette_tint", "Tint the palette");
	new SAction("ppal_tweak", "Tweak", "t_palette_tweak", "Tweak the palette");
	new SAction("ppal_invert", "Invert", "t_palette_invert", "Invert the palette");
	new SAction("ppal_moveup", "Pull Ahead", "t_palette_pull", "Move this palette one rank towards the first");
	new SAction("ppal_movedown", "Push Back", "t_palette_push", "Move this palette one rank towards the last");
	new SAction("ppal_duplicate", "Duplicate", "t_palette_duplicate", "Create a copy of this palette at the end");
	new SAction("ppal_remove", "Remove", "t_palette_delete", "Erase this palette");
	new SAction("ppal_removeothers", "Remove Others", "t_palette_deleteothers", "Keep only this palette and erase all others");
	new SAction("ppal_report", "Write Report", "e_text", "Write an info report on this palette");
	new SAction("ppal_generate", "Generate Palettes", "e_palette", "Generate full range of palettes from the first");
	new SAction("ppal_colormap", "Generate Colormaps", "e_colormap", "Generate colormap lump from the first palette");

	// MapEntryPanel
	new SAction("pmap_open_text", "Edit Level Script", "e_text", "Open the map header as text (to edit fragglescript, etc.)");

	// Map Editor Window
	new SAction("mapw_save", "&Save Map Changes", "t_save", "Save any changes to the current map", "Ctrl+S");
	new SAction("mapw_saveas", "Save Map &As...", "t_saveas", "Save the map to a new wad archive", "Ctrl+Shift+S");
	new SAction("mapw_rename", "&Rename Map", "t_rename", "Rename the current map");
	new SAction("mapw_convert", "Con&vert Map...", "t_convert", "Convert the current map to a different format");
	new SAction("mapw_backup", "Restore Backup...", "t_undo", "Restore a previous backup of the current map");
	new SAction("mapw_undo", "Undo", "t_undo", "Undo", "Ctrl+Z");
	new SAction("mapw_redo", "Redo", "t_redo", "Redo", "Ctrl+Y");
	new SAction("mapw_setbra", "Set &Base Resource Archive", "e_archive", "Set the Base Resource Archive, to act as the program 'IWAD'");
	new SAction("mapw_preferences", "&Preferences...", "t_settings", "Setup SLADE options and preferences");
	int group_mode = SAction::newGroup();
	new SAction("mapw_mode_vertices", "Vertices Mode", "t_verts", "Change to vertices editing mode", "kb:me2d_mode_vertices", SAction::RADIO, -1, group_mode);
	new SAction("mapw_mode_lines", "Lines Mode", "t_lines", "Change to lines editing mode", "kb:me2d_mode_lines", SAction::RADIO, -1, group_mode);
	new SAction("mapw_mode_sectors", "Sectors Mode", "t_sectors", "Change to sectors editing mode", "kb:me2d_mode_sectors", SAction::RADIO, -1, group_mode);
	new SAction("mapw_mode_things", "Things Mode", "t_things", "Change to things editing mode", "kb:me2d_mode_things", SAction::RADIO, -1, group_mode);
	new SAction("mapw_mode_3d", "3d Mode", "t_3d", "Change to 3d editing mode", "kb:map_toggle_3d", SAction::RADIO, -1, group_mode);
	int group_flat_type = SAction::newGroup();
	new SAction("mapw_flat_none", "Wireframe", "t_flat_w", "Don't show flats (wireframe)", "", SAction::RADIO, -1, group_flat_type);
	new SAction("mapw_flat_untextured", "Untextured", "t_flat_u", "Show untextured flats", "", SAction::RADIO, -1, group_flat_type);
	new SAction("mapw_flat_textured", "Textured", "t_flat_t", "Show textured flats", "", SAction::RADIO, -1, group_flat_type);
	int group_sector_mode = SAction::newGroup();
	new SAction("mapw_sectormode_normal", "Normal (Both)", "t_sector_both", "Edit sector floors and ceilings", "", SAction::RADIO, -1, group_sector_mode);
	new SAction("mapw_sectormode_floor", "Floors", "t_sector_floor", "Edit sector floors", "", SAction::RADIO, -1, group_sector_mode);
	new SAction("mapw_sectormode_ceiling", "Ceilings", "t_sector_ceiling", "Edit sector ceilings", "", SAction::RADIO, -1, group_sector_mode);
	new SAction("mapw_showproperties", "&Item Properties", "t_properties", "Toggle the Item Properties window", "Ctrl+1");
	new SAction("mapw_showconsole", "&Console", "t_console", "Toggle the Console window", "Ctrl+2");
	new SAction("mapw_showundohistory", "&Undo History", "t_undo", "Toggle the Undo History window", "Ctrl+3");
	new SAction("mapw_showchecks", "Map Checks", "i_tick", "Toggle the Map Checks window", "Ctrl+4");
	new SAction("mapw_showscripteditor", "Script &Editor", "e_text", "Toggle the Script Editor window", "Ctrl+5");
	new SAction("mapw_run_map", "Run Map", "t_run", "Run the current map", "Ctrl+Shift+R");
	new SAction("mapw_draw_lines", "Draw Lines", "t_linedraw", "Begin line drawing", "kb:me2d_begin_linedraw");
	new SAction("mapw_draw_shape", "Draw Shape", "t_shapedraw", "Begin shape drawing", "kb:me2d_begin_shapedraw");
	new SAction("mapw_edit_objects", "Edit Object(s)", "t_objectedit", "Edit currently selected object(s)", "kb:me2d_begin_object_edit");
	new SAction("mapw_vertex_create", "Create Vertex Here", "", "Create a new vertex at the cursor position");
	new SAction("mapw_line_changetexture", "Change Texture", "", "Change the currently selected or hilighted line texture(s)", "kb:me2d_line_change_texture");
	new SAction("mapw_line_changespecial", "Change Special", "", "Change the currently selected or hilighted line special");
	new SAction("mapw_line_tagedit", "Edit Tagged", "", "Select sectors/things to tag to this line's special", "kb:me2d_line_tag_edit");
	new SAction("mapw_line_correctsectors", "Correct Sectors", "i_tick", "Correct line sector references");
	new SAction("mapw_line_flip", "Flip Line", "", "Flip the currently selected of hilighted line(s)", "kb:me2d_line_flip");
	new SAction("mapw_thing_changetype", "Change Type", "", "Change the currently selected or hilighted thing type(s)", "kb:me2d_thing_change_type");
	new SAction("mapw_thing_create", "Create Thing Here", "", "Create a new thing at the cursor position");
	new SAction("mapw_sector_create", "Create Sector Here", "", "Create a sector at the cursor position");
	new SAction("mapw_sector_changetexture", "Change Texture", "", "Change the currently selected or hilighted sector texture(s)", "kb:me2d_sector_change_texture");
	new SAction("mapw_sector_changespecial", "Change Special", "", "Change the currently selected or hilighted sector special(s)");
	new SAction("mapw_sector_join", "Merge Sectors", "", "Join the currently selected sectors together, removing unneeded lines", "kb:me2d_sector_join");
	new SAction("mapw_sector_join_keep", "Join Sectors", "", "Join the currently selected sectors together, keeping all lines", "kb:me2d_sector_join_keep");
	new SAction("mapw_item_properties", "Properties", "t_properties", "Edit the currently selected item's properties");
	new SAction("mapw_camera_set", "Move 3d Camera Here", "", "Set the current position of the 3d mode camera to the cursor position");
	new SAction("mapw_clear_selection", "Clear Selection", "", "Clear the current selection, if any", "kb:me2d_clear_selection");
	new SAction("mapw_show_fullmap", "Show Full Map", "", "Zooms out so that the full map is visible", "kb:me2d_show_all");
	new SAction("mapw_show_item", "Show Item...", "", "Zoom and scroll to show a map item");
	new SAction("mapw_toggle_selection_numbers", "Show Selection Numbers", "", "Show/hide selection numbers", "kb:me2d_toggle_selection_numbers", SAction::CHECK);
	new SAction("mapw_mirror_y", "Mirror Vertically", "t_flip", "Mirror the selected objects vertically", "kb:me2d_mirror_y");
	new SAction("mapw_mirror_x", "Mirror Horizontally", "t_mirror", "Mirror the selected objects horizontally", "kb:me2d_mirror_x");

	// Script editor
	new SAction("mapw_script_save", "Save", "t_save", "Save changes to scripts");
	new SAction("mapw_script_compile", "Compile", "t_compile", "Compile scripts");
	new SAction("mapw_script_jumpto", "Jump To...", "t_up", "Jump to a specific script/function");


	// Init checked actions
	getAction("mapw_toggle_selection_numbers")->toggled = map_show_selection_numbers;
}

/* MainApp::OnInit
 * Application initialization, run when program is started
 *******************************************************************/
bool MainApp::OnInit()
{
	// Set locale to C so that the tokenizer will work properly
	// even in locales where the decimal separator is a comma.
	setlocale(LC_ALL, "C");

	// Init global variables
	Global::error = "";
	ArchiveManager::getInstance();
	init_ok = false;

	// Init variables
	action_invalid = new SAction("invalid", "Invalid Action", "", "Something's gone wrong here");

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

	// Init application directories
	if (!initDirectories())
		return false;

	// Load image handlers
	wxInitAllImageHandlers();

	// Init logfile
	initLogFile();

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
	theSplashWindow->init();
	theSplashWindow->show("Starting up...");

	// Init SImage formats
	SIFormat::initFormats();

	// Load program icons
	wxLogMessage("Loading icons");
	loadIcons();

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
	theMainWindow;

	// Init base resource
	wxLogMessage("Loading base resource");
	theArchiveManager->initBaseResource();
	wxLogMessage("Base resource loaded");

	// Show the main window
	theMainWindow->Show(true);
	SetTopWindow(theMainWindow);
	theSplashWindow->SetParent(theMainWindow);
	theSplashWindow->CentreOnParent();

	// Open any archives on the command line
	// argv[0] is normally the executable itself (i.e. Slade.exe)
	// and opening it as an archive should not be attempted...
	for (int a = 1; a < argc; a++)
	{
		string arg = argv[a];
		theArchiveManager->openArchive(arg);
	}

	// Hide splash screen
	theSplashWindow->hide();

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
	}

	// Check for updates
#ifdef __WXMSW__
	wxHTTP::Initialize();
	if (update_check)
		checkForUpdates(false);
#endif

	// Bind events
	Bind(wxEVT_MENU, &MainApp::onMenu, this);
	Bind(wxEVT_COMMAND_VERSIONCHECK_COMPLETED, &MainApp::onVersionCheckCompleted, this);

	return true;
}

/* MainApp::OnExit
 * Application shutdown, run when program is closed
 *******************************************************************/
int MainApp::OnExit()
{
	exiting = true;

	// Save configuration
	saveConfigFile();

	// Save text style configuration
	StyleSet::saveCurrent();

	// Save colour configuration
	MemChunk ccfg;
	ColourConfiguration::writeConfiguration(ccfg);
	ccfg.exportFile(appPath("colours.cfg", DIR_USER));

	// Save game exes
	wxFile f;
	f.Open(appPath("executables.cfg", DIR_USER), wxFile::write);
	f.Write(Executables::writeExecutables());
	f.Close();

	// Close the map editor if it's open
	MapEditorWindow::deleteInstance();

	// Close all open archives
	theArchiveManager->closeAll();

	// Clean up
	EntryType::cleanupEntryTypes();
	ArchiveManager::deleteInstance();
	Console::deleteInstance();
	SplashWindow::deleteInstance();

	// Clear temp folder
	wxDir temp;
	temp.Open(appPath("", DIR_TEMP));
	string filename = wxEmptyString;
	bool files = temp.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
	while (files)
	{
		if (!wxRemoveFile(appPath(filename, DIR_TEMP)))
			wxLogMessage("Warning: Could not clean up temporary file \"%s\"", filename);
		files = temp.GetNext(&filename);
	}

	// Close lua
	Lua::close();

	// Close DUMB
	dumb_exit();

	return 0;
}

void MainApp::OnFatalException()
{
#if !defined(__APPLE__) && !defined(__FreeBSD__)
#ifndef _DEBUG
	SLADEStackTrace st;
	st.WalkFromException();
	SLADECrashDialog sd(st);
	sd.ShowModal();
#endif
#endif
}

#ifdef __APPLE__
void MainApp::MacOpenFile(const wxString &fileName)
{
	theMainWindow->getArchiveManagerPanel()->openFile(fileName);
}
#endif // __APPLE__

/* MainApp::readConfigFile
 * Reads and parses the SLADE configuration file
 *******************************************************************/
void MainApp::readConfigFile()
{
	// Open SLADE.cfg
	Tokenizer tz;
	if (!tz.openFile(appPath("slade3.cfg", DIR_USER)))
		return;

	// Go through the file with the tokenizer
	string token = tz.getToken();
	while (token.Cmp(""))
	{
		// If we come across a 'cvars' token, read in the cvars section
		if (!token.Cmp("cvars"))
		{
			token = tz.getToken();	// Skip '{'

			// Keep reading name/value pairs until we hit the ending '}'
			string cvar_name = tz.getToken();
			while (cvar_name.Cmp("}"))
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
			while (token.Cmp("}"))
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
			while (token != "}")
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
			while (token != "}")
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
			while (token != "}")
			{
				if (token.length())
				{
					string path = tz.getToken();
					Executables::setExePath(token, path);
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

/* MainApp::saveConfigFile
 * Saves the SLADE configuration file
 *******************************************************************/
void MainApp::saveConfigFile()
{
	// Open SLADE.cfg for writing text
	wxFile file(appPath("slade3.cfg", DIR_USER), wxFile::write);

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

/* MainApp::checkForUpdates
 * Runs the version checker, if [message_box] is true, a message box
 * will be shown if already up-to-date
 *******************************************************************/
void MainApp::checkForUpdates(bool message_box)
{
#ifdef __WXMSW__
	update_check_message_box = message_box;
	LOG_MESSAGE(1, "Checking for updates...");
	VersionCheck* checker = new VersionCheck(this);
	checker->Create();
	checker->Run();
#endif
}

/* MainApp::getAction
 * Returns the SAction matching [id]
 *******************************************************************/
SAction* MainApp::getAction(string id)
{
	// Find matching action
	for (unsigned a = 0; a < actions.size(); a++)
	{
		if (S_CMP(actions[a]->getId(), id))
			return actions[a];
	}

	// Not found
	return action_invalid;
}

/* MainApp::doAction
 * Performs the SAction matching [id]
 *******************************************************************/
bool MainApp::doAction(string id)
{
	// Toggle action if necessary
	toggleAction(id);

	// Send action to all handlers
	bool handled = false;
	for (unsigned a = 0; a < action_handlers.size(); a++)
	{
		if (action_handlers[a]->handleAction(id))
		{
			handled = true;
			break;
		}
	}

	// Warn if nothing handled it
	if (!handled)
		wxLogMessage("Warning: Action \"%s\" not handled", id);

	// Return true if handled
	return handled;
}

/* MainApp::toggleAction
 * Toggles the SAction matching [id]
 *******************************************************************/
void MainApp::toggleAction(string id)
{
	// Check action type for check/radio toggle
	SAction* action = getAction(id);

	// Type is 'check', just toggle it
	if (action && action->type == SAction::CHECK)
		action->toggled = !action->toggled;

	// Type is 'radio', toggle this and un-toggle others in the group
	else if (action && action->type == SAction::RADIO && action->group >= 0)
	{
		// Go through and toggle off all other actions in the same group
		for (unsigned a = 0; a < actions.size(); a++)
		{
			if (actions[a]->group == action->group)
				actions[a]->toggled = false;
		}

		action->toggled = true;
	}
}


/*******************************************************************
 * MAINAPP CLASS EVENTS
 *******************************************************************/

/* MainApp::onMenu
 * Called when a menu item is selected in the application
 *******************************************************************/
void MainApp::onMenu(wxCommandEvent& e)
{
	// Find applicable action
	string action = "";
	SAction* s_action = NULL;
	for (unsigned a = 0; a < actions.size(); a++)
	{
		if (actions[a]->getWxId() == e.GetId())
		{
			action = actions[a]->getId();
			s_action = actions[a];
			break;
		}
	}

	// If action is valid, send to all action handlers
	bool handled = false;
	if (!action.IsEmpty())
	{
		current_action = action;
		handled = doAction(action);

		// Check if triggering object is a menu item
		if (s_action && s_action->type == SAction::CHECK)
		{
			if (e.GetEventObject() && e.GetEventObject()->IsKindOf(wxCLASSINFO(wxMenuItem)))
			{
				wxMenuItem* item = (wxMenuItem*)e.GetEventObject();
				item->Check(s_action->toggled);
			}
		}

		current_action = "";
	}

	// If not handled, let something else handle it
	if (!handled)
		e.Skip();
}

/* MainApp::onVersionCheckCompleted
 * Called when the VersionCheck thread completes
 *******************************************************************/
void MainApp::onVersionCheckCompleted(wxThreadEvent& e)
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
