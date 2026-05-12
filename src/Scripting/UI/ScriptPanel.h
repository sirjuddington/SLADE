#pragma once

namespace slade
{
namespace scriptmanager
{
	struct Script;
}
class TextEditorCtrl;
class FindReplacePanel;
class SAuiToolBar;

class ScriptPanel : public wxPanel
{
public:
	ScriptPanel(wxWindow* parent, scriptmanager::Script* script = nullptr);

	TextEditorCtrl*        editor() const { return text_editor_; }
	scriptmanager::Script* script() const { return script_; }
	string                 currentText() const;
	bool                   modified() const;

	bool close();
	bool save();

	bool handleAction(string_view id);

private:
	scriptmanager::Script* script_             = nullptr;
	TextEditorCtrl*        text_editor_        = nullptr;
	FindReplacePanel*      find_replace_panel_ = nullptr;
	long                   last_saved_         = 0;

	SAuiToolBar* setupToolbar();
};
} // namespace slade
