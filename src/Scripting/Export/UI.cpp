
#include "Main.h"
#include "UI/UI.h"
#include "Export.h"
#include "Scripting/Lua.h"
#include "Scripting/LuaBridge.h"
#include "UI/Dialogs/ExtMessageDialog.h"
#include "Utility/SFileDialog.h"

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
static void messageBox(const string& title, const string& message, MessageBoxIcon icon = MessageBoxIcon::Info)
{
	long style = 4 | wxCENTRE;
	switch (icon)
	{
	case MessageBoxIcon::Info:     style |= wxICON_INFORMATION; break;
	case MessageBoxIcon::Question: style |= wxICON_QUESTION; break;
	case MessageBoxIcon::Warning:  style |= wxICON_WARNING; break;
	case MessageBoxIcon::Error:    style |= wxICON_ERROR; break;
	}

	wxMessageBox(message, title, style, currentWindow());
}

// -----------------------------------------------------------------------------
// Shows an extended message box with a [title], [message] and [extra] in a
// scrollable text view
// -----------------------------------------------------------------------------
static void messageBoxExtended(const string& title, const string& message, const string& extra)
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
static string promptString(const string& title, const string& message, const string& default_value)
{
	return wxGetTextFromUser(message, title, default_value, currentWindow()).ToStdString();
}

// -----------------------------------------------------------------------------
// Prompt for a number (int) and returns what was entered
// -----------------------------------------------------------------------------
static int promptNumber(const string& title, const string& message, int default_value, int min, int max)
{
	return static_cast<int>(wxGetNumberFromUser(message, "", title, default_value, min, max));
}

// -----------------------------------------------------------------------------
// Prompts for a yes/no answer and returns true if yes
// -----------------------------------------------------------------------------
static bool promptYesNo(const string& title, const string& message)
{
	return (wxMessageBox(message, title, wxYES_NO | wxICON_QUESTION) == wxYES);
}

// -----------------------------------------------------------------------------
// Opens the file browser to select a single file
// -----------------------------------------------------------------------------
static string browseFile(string_view title, string_view extensions, string_view filename)
{
	filedialog::FDInfo inf;
	filedialog::openFile(inf, title, extensions, currentWindow(), filename);
	return inf.filenames.empty() ? "" : inf.filenames[0];
}

// -----------------------------------------------------------------------------
// Opens the file browser to select multiple files
// -----------------------------------------------------------------------------
static vector<string> browseFiles(string_view title, string_view extensions)
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
static string saveFile(string_view title, string_view extensions, string_view fn_default = {})
{
	filedialog::FDInfo inf;
	if (filedialog::saveFile(inf, title, extensions, currentWindow(), fn_default))
		return inf.filenames[0];

	return {};
}

// -----------------------------------------------------------------------------
// Opens the file browser to save multiple files
// -----------------------------------------------------------------------------
static std::tuple<string, string> saveFiles(string_view title, string_view extensions)
{
	filedialog::FDInfo inf;
	if (filedialog::saveFiles(inf, title, extensions, currentWindow()))
		return std::make_tuple(inf.path, inf.extension);

	return { {}, {} };
}

// -----------------------------------------------------------------------------
// Registers the UI namespace with lua
// -----------------------------------------------------------------------------
void registerUINamespace(lua_State* lua)
{
	auto ui = luabridge::getGlobalNamespace(lua).beginNamespace("UI");

	// Constants
	// -------------------------------------------------------------------------
	ui.addProperty("MB_ICON_INFO", [] { return static_cast<int>(MessageBoxIcon::Info); });
	ui.addProperty("MB_ICON_QUESTION", [] { return static_cast<int>(MessageBoxIcon::Question); });
	ui.addProperty("MB_ICON_WARNING", [] { return static_cast<int>(MessageBoxIcon::Warning); });
	ui.addProperty("MB_ICON_ERROR", [] { return static_cast<int>(MessageBoxIcon::Error); });

	// Functions
	// -------------------------------------------------------------------------
	ui.addFunction(
		"MessageBox", &messageBox, [](const string& title, const string& message) { messageBox(title, message); });
	ui.addFunction("MessageBoxExt", &messageBoxExtended);
	ui.addFunction("PromptString", &promptString);
	ui.addFunction("PromptNumber", &promptNumber);
	ui.addFunction("PromptYesNo", &promptYesNo);
	ui.addFunction("PromptOpenFile", &browseFile);
	ui.addFunction("PromptOpenFiles", &browseFiles);
	ui.addFunction(
		"PromptSaveFile",
		&saveFile,
		[](string_view title, string_view extensions) { return saveFile(title, extensions); });
	ui.addFunction("PromptSaveFiles", &saveFiles);
	ui.addFunction(
		"ShowSplash",
		[](const string& message) { ui::showSplash(message, false, currentWindow()); },
		[](const string& message, bool progress) { ui::showSplash(message, progress, currentWindow()); });
	ui.addFunction("HideSplash", &ui::hideSplash);
	ui.addFunction("UpdateSplash", &ui::updateSplash);
	ui.addFunction("SplashProgress", &ui::getSplashProgress);
	ui.addFunction("SetSplashMessage", &ui::setSplashMessage);
	ui.addFunction("SetSplashProgressMessage", &ui::setSplashProgressMessage);
	ui.addFunction("SetSplashProgress", luabridge::overload<float>(ui::setSplashProgress));
}
} // namespace slade::lua
