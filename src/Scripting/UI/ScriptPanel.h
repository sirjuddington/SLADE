#pragma once

namespace ScriptManager { struct Script; }
class TextEditorCtrl;
class FindReplacePanel;

class ScriptPanel : public wxPanel
{
public:
	ScriptPanel(wxWindow* parent, ScriptManager::Script* script = nullptr);

	TextEditorCtrl*			editor() const { return text_editor_; }
	ScriptManager::Script*	script() const { return script_; }
	string					currentText() const;

	bool handleAction(const string& id);

private:
	ScriptManager::Script*	script_;
	TextEditorCtrl*			text_editor_;
	FindReplacePanel*		find_replace_panel_;
};
