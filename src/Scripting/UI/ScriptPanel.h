#pragma once

namespace ScriptManager { struct Script; }
class TextEditorCtrl;

class ScriptPanel : public wxPanel
{
public:
	ScriptPanel(wxWindow* parent, ScriptManager::Script* script = nullptr);

	ScriptManager::Script*	script() const { return script_; }
	string					currentText() const;

private:
	ScriptManager::Script*	script_;
	TextEditorCtrl*				text_editor_;
};
