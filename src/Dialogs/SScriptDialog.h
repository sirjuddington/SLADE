#pragma once

#include "UI/SDialog.h"

class TextEditor;
class wxTreeCtrl;
class SScriptDialog : public SDialog
{
public:
	SScriptDialog(wxWindow* parent);
	~SScriptDialog();

private:
	TextEditor*	text_editor_;
	wxButton*	btn_run_;
	wxTreeCtrl*	tree_scripts_;

	// Setup
	void	bindEvents();
	void	populateScriptsTree();

	// Static
	static string	prev_script_;
};
