
// logMessage: Prints a log message
void logMessage(const char* message)
{
	Log::message(Log::MessageType::Script, message);
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

void registerMiscTypes(sol::state& lua)
{
	lua.new_simple_usertype<fpoint2_t>(
		"Point",

		sol::constructors<fpoint2_t(), fpoint2_t(double, double)>(),

		// Properties
		"x", &fpoint2_t::x,
		"y", &fpoint2_t::y
	);

	lua.new_simple_usertype<rgba_t>(
		"Colour",

		sol::constructors<
			rgba_t(),
			rgba_t(uint8_t, uint8_t, uint8_t),
			rgba_t(uint8_t, uint8_t, uint8_t, uint8_t)
		>(),

		// Properties
		"r", &rgba_t::r,
		"g", &rgba_t::g,
		"b", &rgba_t::b,
		"a", &rgba_t::a
	);

	lua.new_simple_usertype<plane_t>(
		"Plane",

		sol::constructors<plane_t(), plane_t(double, double, double, double)>(),

		// Properties
		"a", &plane_t::a,
		"b", &plane_t::b,
		"c", &plane_t::c,
		"d", &plane_t::d,

		// Functions
		"heightAt", sol::resolve<double(fpoint2_t)>(&plane_t::height_at)
	);
}

void registerSLADENamespace(sol::state& lua)
{
	sol::table slade = lua["slade"];
	slade.set_function("logMessage",			&logMessage);
	slade.set_function("globalError",			[]() { return Global::error; });
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

void registerSplashWindowNamespace(sol::state& lua)
{
	sol::table splash = lua["splashWindow"];

	splash.set_function("show", sol::overload(
		[](const string& message) { UI::showSplash(message, false, Lua::currentWindow()); },
		[](const string& message, bool progress) { UI::showSplash(message, progress, Lua::currentWindow()); }
	));
	splash.set_function("hide",					&UI::hideSplash);
	splash.set_function("update",				&UI::updateSplash);
	splash.set_function("progress",				&UI::getSplashProgress);
	splash.set_function("setMessage",			&UI::setSplashMessage);
	splash.set_function("setProgressMessage",	&UI::setSplashProgressMessage);
	splash.set_function("setProgress",			&UI::setSplashProgress);
}
