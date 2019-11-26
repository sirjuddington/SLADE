#pragma once

namespace ScriptManager
{
struct Script;
}
class TextEditorCtrl;
class FindReplacePanel;
class SToolBar;

class ScriptPanel : public wxPanel
{
public:
	ScriptPanel(wxWindow* parent, ScriptManager::Script* script = nullptr);

	TextEditorCtrl*        editor() const { return text_editor_; }
	ScriptManager::Script* script() const { return script_; }
	string                 currentText() const;
	bool                   modified() const;

	bool close();
	bool save();

	bool handleAction(string_view id);

private:
	ScriptManager::Script* script_             = nullptr;
	TextEditorCtrl*        text_editor_        = nullptr;
	FindReplacePanel*      find_replace_panel_ = nullptr;
	long                   last_saved_         = 0;

	SToolBar* setupToolbar();
};
