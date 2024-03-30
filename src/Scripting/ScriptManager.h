#pragma once

namespace slade
{
namespace scriptmanager
{
	enum class ScriptType
	{
		Editor,
		Custom,
		Global,
		Archive,
		Entry,
		Map,
		NonEditor
	};

	struct Script
	{
		ScriptType             type = ScriptType::Editor;
		string                 text;
		string                 name;
		string                 path;
		bool                   read_only = false;
		weak_ptr<ArchiveEntry> source_entry;
		string                 source_file;
	};

	typedef vector<unique_ptr<Script>> ScriptList;

	ScriptList& editorScripts(ScriptType type = ScriptType::Editor);

	void init();
	void open();
	void saveUserScripts();

	bool renameScript(Script* script, string_view new_name);
	bool deleteScript(const Script* script);

	Script* createEditorScript(string_view name, ScriptType type);
	void    populateEditorScriptMenu(wxMenu* menu, ScriptType type, string_view action_id);

	void runArchiveScript(Archive* archive, int index, wxWindow* parent = nullptr);
	void runEntryScript(vector<ArchiveEntry*> entries, int index, wxWindow* parent = nullptr);
	void runMapScript(SLADEMap* map, int index, wxWindow* parent = nullptr);
} // namespace scriptmanager
} // namespace slade
