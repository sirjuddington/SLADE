
#include "Main.h"
#include "Dialogs/ExtMessageDialog.h"
#include "Dialogs/Preferences/ACSPrefsPanel.h"
#include "Export.h"
#include "Scripting/Lua.h"
#include "Utility/SFileDialog.h"
#include "thirdparty/sol/sol.hpp"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
enum class MessageBoxIcon
{
	Info,
	Question,
	Warning,
	Error
};
}


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace Lua
{
// -----------------------------------------------------------------------------
// Shows a message box with a [title] and [message]
// -----------------------------------------------------------------------------
void messageBox(const std::string& title, const std::string& message, MessageBoxIcon icon = MessageBoxIcon::Info)
{
	long style = 4 | wxCENTRE;
	switch (icon)
	{
	case MessageBoxIcon::Info: style |= wxICON_INFORMATION; break;
	case MessageBoxIcon::Question: style |= wxICON_QUESTION; break;
	case MessageBoxIcon::Warning: style |= wxICON_WARNING; break;
	case MessageBoxIcon::Error: style |= wxICON_ERROR; break;
	}

	wxMessageBox(message, title, style, currentWindow());
}

// -----------------------------------------------------------------------------
// Shows an extended message box with a [title], [message] and [extra] in a
// scrollable text view
// -----------------------------------------------------------------------------
void messageBoxExtended(const std::string& title, const std::string& message, const std::string& extra)
{
	ExtMessageDialog dlg(currentWindow(), title);
	dlg.setMessage(message);
	dlg.setExt(extra);
	dlg.CenterOnParent();
	dlg.ShowModal();
}

// -----------------------------------------------------------------------------
// Prompts for a string and returns what was entered
// -----------------------------------------------------------------------------
std::string promptString(const std::string& title, const std::string& message, const std::string& default_value)
{
	return wxGetTextFromUser(message, title, default_value, currentWindow()).ToStdString();
}

// -----------------------------------------------------------------------------
// Prompt for a number (int) and returns what was entered
// -----------------------------------------------------------------------------
int promptNumber(const std::string& title, const std::string& message, int default_value, int min, int max)
{
	return (int)wxGetNumberFromUser(message, "", title, default_value, min, max);
}

// -----------------------------------------------------------------------------
// Prompts for a yes/no answer and returns true if yes
// -----------------------------------------------------------------------------
bool promptYesNo(const std::string& title, const std::string& message)
{
	return (wxMessageBox(message, title, wxYES_NO | wxICON_QUESTION) == wxYES);
}

// -----------------------------------------------------------------------------
// Opens the file browser to select a single file
// -----------------------------------------------------------------------------
std::string browseFile(std::string_view title, std::string_view extensions, std::string_view filename)
{
	SFileDialog::FDInfo inf;
	SFileDialog::openFile(inf, title, extensions, currentWindow(), filename);
	return inf.filenames.empty() ? "" : inf.filenames[0];
}

// -----------------------------------------------------------------------------
// Opens the file browser to select multiple files
// -----------------------------------------------------------------------------
vector<std::string> browseFiles(std::string_view title, std::string_view extensions)
{
	SFileDialog::FDInfo inf;
	vector<std::string> filenames;
	if (SFileDialog::openFiles(inf, title, extensions, currentWindow()))
		for (const auto& file : inf.filenames)
			filenames.push_back(file);
	return filenames;
}

// -----------------------------------------------------------------------------
// Opens the file browser to save a single file
// -----------------------------------------------------------------------------
std::string saveFile(std::string_view title, std::string_view extensions, std::string_view fn_default = {})
{
	SFileDialog::FDInfo inf;
	if (SFileDialog::saveFile(inf, title, extensions, currentWindow(), fn_default))
		return inf.filenames[0];

	return {};
}

// -----------------------------------------------------------------------------
// Opens the file browser to save multiple files
// -----------------------------------------------------------------------------
std::tuple<std::string, std::string> saveFiles(std::string_view title, std::string_view extensions)
{
	SFileDialog::FDInfo inf;
	if (SFileDialog::saveFiles(inf, title, extensions, currentWindow()))
		return std::make_tuple(inf.path, inf.extension);

	return { {}, {} };
}

// -----------------------------------------------------------------------------
// Registers the UI namespace with lua
// -----------------------------------------------------------------------------
void registerUINamespace(sol::state& lua)
{
	auto ui = lua.create_named_table("UI");

	// Functions
	// -------------------------------------------------------------------------
	ui["MessageBox"] = sol::overload(
		&messageBox, [](const std::string& title, const std::string& message) { messageBox(title, message); });
	ui["MessageBoxExt"]   = &messageBoxExtended;
	ui["PromptString"]    = &promptString;
	ui["PromptNumber"]    = &promptNumber;
	ui["PromptYesNo"]     = &promptYesNo;
	ui["PromptOpenFile"]  = &browseFile;
	ui["PromptOpenFiles"] = &browseFiles;
	ui["PromptSaveFile"]  = sol::overload(
        &saveFile, [](std::string_view title, std::string_view extensions) { return saveFile(title, extensions); });
	ui["PromptSaveFiles"] = &saveFiles;
	ui["ShowSplash"]      = sol::overload(
        [](const std::string& message) { UI::showSplash(message, false, currentWindow()); },
        [](const std::string& message, bool progress) { UI::showSplash(message, progress, currentWindow()); });
	ui["HideSplash"]               = &UI::hideSplash;
	ui["UpdateSplash"]             = &UI::updateSplash;
	ui["SplashProgress"]           = &UI::getSplashProgress;
	ui["SetSplashMessage"]         = &UI::setSplashMessage;
	ui["SetSplashProgressMessage"] = &UI::setSplashProgressMessage;
	ui["SetSplashProgress"]        = &UI::setSplashProgress;

	// Constants
	// -------------------------------------------------------------------------
	ui["MB_ICON_INFO"]     = sol::property([]() { return MessageBoxIcon::Info; });
	ui["MB_ICON_QUESTION"] = sol::property([]() { return MessageBoxIcon::Question; });
	ui["MB_ICON_WARNING"]  = sol::property([]() { return MessageBoxIcon::Warning; });
	ui["MB_ICON_ERROR"]    = sol::property([]() { return MessageBoxIcon::Error; });
}
} // namespace Lua
