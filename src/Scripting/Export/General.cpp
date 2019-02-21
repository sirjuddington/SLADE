
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archive.cpp
// Description: Functions to export general/misc. types and namespaces to lua
//              using sol3
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
#include "Archive/Archive.h"
#include "Dialogs/ExtMessageDialog.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditContext.h"
#include "Scripting/Lua.h"
#include "Utility/SFileDialog.h"
#include "thirdparty/sol/sol.hpp"


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace Lua
{
// -----------------------------------------------------------------------------
// Writes a log [message]
// -----------------------------------------------------------------------------
void logMessage(const char* message)
{
	Log::message(Log::MessageType::Script, message);
}

// -----------------------------------------------------------------------------
// Shows a message box with a [title] and [message]
// -----------------------------------------------------------------------------
void messageBox(const std::string& title, const std::string& message)
{
	wxMessageBox(message, title, 5L, currentWindow());
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
std::string browseFile(const std::string& title, const std::string& extensions, const std::string& filename)
{
	SFileDialog::FDInfo inf;
	SFileDialog::openFile(inf, title, extensions, currentWindow(), filename);
	return inf.filenames.empty() ? "" : inf.filenames[0].ToStdString();
}

// -----------------------------------------------------------------------------
// Opens the file browser to select multiple files
// -----------------------------------------------------------------------------
vector<std::string> browseFiles(const std::string& title, const std::string& extensions)
{
	SFileDialog::FDInfo inf;
	vector<std::string> filenames;
	if (SFileDialog::openFiles(inf, title, extensions, currentWindow()))
		for (const auto& file : inf.filenames)
			filenames.push_back(file.ToStdString());
	return filenames;
}

// -----------------------------------------------------------------------------
// Switches to the tab for [archive], opening it if necessary
// -----------------------------------------------------------------------------
bool showArchive(Archive* archive)
{
	if (!archive)
		return false;

	MainEditor::openArchiveTab(archive);
	return true;
}

// -----------------------------------------------------------------------------
// Registers some misc. types with lua
// -----------------------------------------------------------------------------
void registerMiscTypes(sol::state& lua)
{
	// Point type
	auto lua_vec2f = lua.new_usertype<Vec2f>("Point", sol::constructors<Vec2f(), Vec2f(double, double)>());
	lua_vec2f["x"] = &Vec2f::x;
	lua_vec2f["y"] = &Vec2f::y;

	// Colour type
	auto lua_colour = lua.new_usertype<ColRGBA>(
		"Colour",
		sol::
			constructors<ColRGBA(), ColRGBA(uint8_t, uint8_t, uint8_t), ColRGBA(uint8_t, uint8_t, uint8_t, uint8_t)>());
	lua_colour["r"] = &ColRGBA::r;
	lua_colour["g"] = &ColRGBA::g;
	lua_colour["b"] = &ColRGBA::b;
	lua_colour["a"] = &ColRGBA::a;

	// Plane type
	auto lua_plane = lua.new_usertype<Plane>(
		"Plane", sol::constructors<Plane(), Plane(double, double, double, double)>());
	lua_plane["a"]        = &Plane::a;
	lua_plane["b"]        = &Plane::b;
	lua_plane["c"]        = &Plane::c;
	lua_plane["d"]        = &Plane::d;
	lua_plane["heightAt"] = sol::resolve<double(Vec2d) const>(&Plane::heightAt);
}

// -----------------------------------------------------------------------------
// Registers the App namespace with lua
// -----------------------------------------------------------------------------
void registerAppNamespace(sol::state& lua)
{
	auto app = lua.create_named_table("App");

	app["logMessage"]            = &logMessage;
	app["globalError"]           = []() { return Global::error; };
	app["messageBox"]            = &messageBox;
	app["messageBoxExt"]         = &messageBoxExtended;
	app["promptString"]          = &promptString;
	app["promptNumber"]          = &promptNumber;
	app["promptYesNo"]           = &promptYesNo;
	app["browseFile"]            = &browseFile;
	app["browseFiles"]           = &browseFiles;
	app["currentArchive"]        = &MainEditor::currentArchive;
	app["currentEntry"]          = &MainEditor::currentEntry;
	app["currentEntrySelection"] = &MainEditor::currentEntrySelection;
	app["showArchive"]           = &showArchive;
	app["showEntry"]             = &MainEditor::openEntry;
	app["mapEditor"]             = &MapEditor::editContext;
}

// -----------------------------------------------------------------------------
// Registers the SplashWindow namespace with lua
// -----------------------------------------------------------------------------
void registerSplashWindowNamespace(sol::state& lua)
{
	auto splash = lua.create_named_table("SplashWindow");

	splash["show"] = sol::overload(
		[](const std::string& message) { UI::showSplash(message, false, currentWindow()); },
		[](const std::string& message, bool progress) { UI::showSplash(message, progress, currentWindow()); });
	splash["hide"]               = &UI::hideSplash;
	splash["update"]             = &UI::updateSplash;
	splash["progress"]           = &UI::getSplashProgress;
	splash["setMessage"]         = &UI::setSplashMessage;
	splash["setProgressMessage"] = &UI::setSplashProgressMessage;
	splash["setProgress"]        = &UI::setSplashProgress;
}

} // namespace Lua
