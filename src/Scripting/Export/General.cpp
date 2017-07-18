
#include "Main.h"
#include "Export.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditor.h"
#include "Scripting/Lua.h"
#include "Utility/SFileDialog.h"
#include "Archive/Archive.h"
#include "MapEditor/MapEditContext.h"

namespace Lua
{

// logMessage: Prints a log message
void logMessage(const char* message)
{
	Log::message(Log::MessageType::Script, message);
}

// Get the global error message
string globalError()
{
	return Global::error;
}

// Show a message box
void messageBox(const string& title, const string& message)
{
	wxMessageBox(message, title, 5L, Lua::currentWindow());
}

// Prompt for a string
string promptString(const string& title, const string& message, const string& default_value)
{
	return wxGetTextFromUser(message, title, default_value, Lua::currentWindow());
}

// Prompt for a number
int promptNumber(
	const string& title,
	const string& message,
	int default_value,
	int min,
	int max)
{
	return (int)wxGetNumberFromUser(message, "", title, default_value, min, max);
}

// Prompt for a yes/no answer
bool promptYesNo(const string& title, const string& message)
{
	return (wxMessageBox(message, title, wxYES_NO | wxICON_QUESTION) == wxYES);
}

// Browse for a single file
string browseFile(const string& title, const string& extensions, const string& filename)
{
	SFileDialog::fd_info_t inf;
	SFileDialog::openFile(inf, title, extensions, Lua::currentWindow(), filename);
	return inf.filenames.empty() ? "" : inf.filenames[0];
}

// Browse for multiple files
vector<string> browseFiles(const string& title, const string& extensions)
{
	SFileDialog::fd_info_t inf;
	vector<string> filenames;
	if (SFileDialog::openFiles(inf, title, extensions, Lua::currentWindow()))
		filenames.assign(inf.filenames.begin(), inf.filenames.end());
	return filenames;
}

// Switch to the tab for [archive], opening it if necessary
bool showArchive(Archive* archive)
{
	if (!archive)
		return false;

	MainEditor::openArchiveTab(archive);
	return true;
}

void registerSLADENamespace(sol::state& lua)
{
	sol::table slade = lua["slade"];
	slade.set_function("logMessage",			&logMessage);
	slade.set_function("globalError",			&globalError);
	slade.set_function("messageBox",			&messageBox);
	slade.set_function("promptString",			&promptString);
	slade.set_function("promptNumber",			&promptNumber);
	slade.set_function("promptYesNo",			&promptYesNo);
	slade.set_function("browseFile",			&browseFile);
	slade.set_function("browseFiles",			&browseFiles);
	slade.set_function("currentArchive",		&MainEditor::currentArchive);
	slade.set_function("currentEntry",			&MainEditor::currentEntry);
	slade.set_function("currentEntrySelection",	&MainEditor::currentEntrySelection);
	slade.set_function("showArchive",			&showArchive);
	slade.set_function("showEntry",				&MainEditor::openEntry);
	slade.set_function("mapEditor",				&MapEditor::editContext);
}

} // namespace Lua
