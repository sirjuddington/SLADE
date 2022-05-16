
#include "Main.h"
#include "Export.h"
#include "Scripting/Lua.h"
#include "UI/Dialogs/ExtMessageDialog.h"
#include "UI/Dialogs/Preferences/ACSPrefsPanel.h"
#include "Utility/SFileDialog.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;


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
namespace slade::lua
{
// -----------------------------------------------------------------------------
// Shows a message box with a [title] and [message]
// -----------------------------------------------------------------------------
void messageBox(const string& title, const string& message, MessageBoxIcon icon = MessageBoxIcon::Info)
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
void messageBoxExtended(const string& title, const string& message, const string& extra)
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
string promptString(const string& title, const string& message, const string& default_value)
{
	return wxGetTextFromUser(message, title, default_value, currentWindow()).ToStdString();
}

// -----------------------------------------------------------------------------
// Prompt for a number (int) and returns what was entered
// -----------------------------------------------------------------------------
int promptNumber(const string& title, const string& message, int default_value, int min, int max)
{
	return (int)wxGetNumberFromUser(message, "", title, default_value, min, max);
}

// -----------------------------------------------------------------------------
// Prompts for a yes/no answer and returns true if yes
// -----------------------------------------------------------------------------
bool promptYesNo(const string& title, const string& message)
{
	return (wxMessageBox(message, title, wxYES_NO | wxICON_QUESTION) == wxYES);
}

// -----------------------------------------------------------------------------
// Opens the file browser to select a single file
// -----------------------------------------------------------------------------
string browseFile(string_view title, string_view extensions, string_view filename)
{
	filedialog::FDInfo inf;
	filedialog::openFile(inf, title, extensions, currentWindow(), filename);
	return inf.filenames.empty() ? "" : inf.filenames[0];
}

// -----------------------------------------------------------------------------
// Opens the file browser to select multiple files
// -----------------------------------------------------------------------------
vector<string> browseFiles(string_view title, string_view extensions)
{
	filedialog::FDInfo inf;
	vector<string>     filenames;
	if (filedialog::openFiles(inf, title, extensions, currentWindow()))
		for (const auto& file : inf.filenames)
			filenames.push_back(file);
	return filenames;
}

// -----------------------------------------------------------------------------
// Opens the file browser to save a single file
// -----------------------------------------------------------------------------
string saveFile(string_view title, string_view extensions, string_view fn_default = {})
{
	filedialog::FDInfo inf;
	if (filedialog::saveFile(inf, title, extensions, currentWindow(), fn_default))
		return inf.filenames[0];

	return {};
}

// -----------------------------------------------------------------------------
// Opens the file browser to save multiple files
// -----------------------------------------------------------------------------
std::tuple<string, string> saveFiles(string_view title, string_view extensions)
{
	filedialog::FDInfo inf;
	if (filedialog::saveFiles(inf, title, extensions, currentWindow()))
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
		&messageBox, [](const string& title, const string& message) { messageBox(title, message); });
	ui["MessageBoxExt"]   = &messageBoxExtended;
	ui["PromptString"]    = &promptString;
	ui["PromptNumber"]    = &promptNumber;
	ui["PromptYesNo"]     = &promptYesNo;
	ui["PromptOpenFile"]  = &browseFile;
	ui["PromptOpenFiles"] = &browseFiles;
	ui["PromptSaveFile"]  = sol::overload(
        &saveFile, [](string_view title, string_view extensions) { return saveFile(title, extensions); });
	ui["PromptSaveFiles"] = &saveFiles;
	ui["ShowSplash"]      = sol::overload(
        [](const string& message) { ui::showSplash(message, false, currentWindow()); },
        [](const string& message, bool progress) { ui::showSplash(message, progress, currentWindow()); });
	ui["HideSplash"]               = &ui::hideSplash;
	ui["UpdateSplash"]             = &ui::updateSplash;
	ui["SplashProgress"]           = &ui::getSplashProgress;
	ui["SetSplashMessage"]         = &ui::setSplashMessage;
	ui["SetSplashProgressMessage"] = &ui::setSplashProgressMessage;
	ui["SetSplashProgress"]        = sol::resolve<void(float)>(ui::setSplashProgress);

	// Constants
	// -------------------------------------------------------------------------
	ui["MB_ICON_INFO"]     = sol::property([]() { return MessageBoxIcon::Info; });
	ui["MB_ICON_QUESTION"] = sol::property([]() { return MessageBoxIcon::Question; });
	ui["MB_ICON_WARNING"]  = sol::property([]() { return MessageBoxIcon::Warning; });
	ui["MB_ICON_ERROR"]    = sol::property([]() { return MessageBoxIcon::Error; });
}
} // namespace slade::lua
