#pragma once

namespace ScriptManager { struct Script; }
class TextEditor;

class ScriptPanel : public wxPanel
{
public:
	ScriptPanel(wxWindow* parent, ScriptManager::Script* script = nullptr);

	ScriptManager::Script*	script() const { return script_; }

private:
	ScriptManager::Script*	script_;
	TextEditor*				text_editor_;
};
