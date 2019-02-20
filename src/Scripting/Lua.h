#pragma once

class SLADEMap;
class wxWindow;
namespace sol
{
class state;
}

namespace Lua
{
bool init();
void close();

struct Error
{
	std::string type;
	std::string message;
	int         line_no;
};
Error& error();
void   showErrorDialog(
	  wxWindow*        parent  = nullptr,
	  std::string_view title   = "Script Error",
	  std::string_view message = "An error occurred running the script, see details below");

bool run(const std::string& program);
bool runFile(const std::string& filename);
bool runArchiveScript(const std::string& script, Archive* archive);
bool runEntryScript(const std::string& script, vector<ArchiveEntry*>& entries);
bool runMapScript(const std::string& script, SLADEMap* map);

sol::state& state();

wxWindow* currentWindow();
void      setCurrentWindow(wxWindow* window);
} // namespace Lua
