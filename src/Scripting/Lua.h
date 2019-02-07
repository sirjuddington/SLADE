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
	wxString type;
	wxString message;
	int      line_no;
};
Error& error();
void   showErrorDialog(
	  wxWindow*       parent  = nullptr,
	  const wxString& title   = "Script Error",
	  const wxString& message = "An error occurred running the script, see details below");

bool run(wxString program);
bool runFile(wxString filename);
bool runArchiveScript(const wxString& script, Archive* archive);
bool runEntryScript(const wxString& script, vector<ArchiveEntry*> entries);
bool runMapScript(const wxString& script, SLADEMap* map);

sol::state& state();

wxWindow* currentWindow();
void      setCurrentWindow(wxWindow* window);
} // namespace Lua
