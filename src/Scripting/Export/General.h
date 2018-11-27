
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

// Show an extended message box
void messageBoxExtended(const string& title, const string& message, const string& extra)
{
	ExtMessageDialog dlg(Lua::currentWindow(), title);
	dlg.setMessage(message);
	dlg.setExt(extra);
	dlg.CenterOnParent();
	dlg.ShowModal();
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
		"heightAt", sol::resolve<double(fpoint2_t) const>(&plane_t::height_at)
	);
}

void registerAppNamespace(sol::state& lua)
{
	sol::table app = lua.create_named_table("App");
	app.set_function("logMessage",				&logMessage);
	app.set_function("globalError",				[]() { return Global::error; });
	app.set_function("messageBox",				&messageBox);
	app.set_function("messageBoxExt",			&messageBoxExtended);
	app.set_function("promptString",			&promptString);
	app.set_function("promptNumber",			&promptNumber);
	app.set_function("promptYesNo",				&promptYesNo);
	app.set_function("browseFile",				&browseFile);
	app.set_function("browseFiles",				&browseFiles);
	app.set_function("currentArchive",			&MainEditor::currentArchive);
	app.set_function("currentEntry",			&MainEditor::currentEntry);
	app.set_function("currentEntrySelection",	&MainEditor::currentEntrySelection);
	app.set_function("showArchive",				&showArchive);
	app.set_function("showEntry",				&MainEditor::openEntry);
	app.set_function("mapEditor",				&MapEditor::editContext);
}

void registerSplashWindowNamespace(sol::state& lua)
{
	sol::table splash = lua.create_named_table("SplashWindow");

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
