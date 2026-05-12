#pragma once

struct lua_State;
class wxWindow;

namespace slade
{
namespace scripting
{
	class LuaException : public std::runtime_error
	{
	public:
		LuaException(string_view type, const string& message) : std::runtime_error(message), error_type_{ type } {}

		const string& errorType() const { return error_type_; }

	private:
		string error_type_;
	};

	struct Environment
	{
		lua_State* state = nullptr;

		Environment(lua_State* state = nullptr);
		~Environment();

		void apply(lua_State* state_to_apply);
	};

	struct Error
	{
		string type;
		string message;
		int    line_no;
	};

	bool init();
	void close();

	bool run(const string& program);
	bool runFile(const string& filename);
	bool runArchiveScript(const string& script, Archive* archive);
	bool runEntryScript(const string& script, vector<ArchiveEntry*>& entries);
	bool runMapScript(const string& script, SLADEMap* map);

	lua_State* luaState();

	Error& error();
	void   showErrorDialog(
		  wxWindow*   parent  = nullptr,
		  string_view title   = "Script Error",
		  string_view message = "An error occurred running the script, see details below");

	wxWindow* currentWindow();
	void      setCurrentWindow(wxWindow* window);
} // namespace scripting
} // namespace slade
