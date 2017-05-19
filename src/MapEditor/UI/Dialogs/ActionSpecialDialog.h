
#ifndef __ACTION_SPECIAL_DIALOG_H__
#define __ACTION_SPECIAL_DIALOG_H__

#include "common.h"
#include "UI/SDialog.h"
#include "UI/WxBasicControls.h"


// A wxDataViewTreeCtrl specialisation showing the
// action specials and groups in a tree structure
class ActionSpecialTreeView : public wxDataViewTreeCtrl
{
private:
	wxDataViewItem	root;
	wxDataViewItem	item_none;
	wxDialog*		parent_dialog;

	// It's incredibly retarded that I actually have to do this
	struct astv_group_t
	{
		string			name;
		wxDataViewItem	item;
		astv_group_t(wxDataViewItem i, string name) { this->item = i; this->name = name; }
	};
	vector<astv_group_t> groups;

	wxDataViewItem	getGroup(string group);

public:
	ActionSpecialTreeView(wxWindow* parent);
	~ActionSpecialTreeView();

	void	setParentDialog(wxDialog* dlg) { parent_dialog = dlg; }

	int		specialNumber(wxDataViewItem item);
	void	showSpecial(int special, bool focus = true);
	int		selectedSpecial();

	void	onItemEdit(wxDataViewEvent& e);
	void	onItemActivated(wxDataViewEvent& e);
};

namespace Game { struct ArgSpec; }
class wxFlexGridSizer;
class ArgsControl;
class ArgsPanel : public wxScrolled<wxPanel>
{
private:
	wxFlexGridSizer*	fg_sizer;
	ArgsControl*		control_args[5];
	wxStaticText*		label_args[5];
	wxStaticText*		label_args_desc[5];

public:
	ArgsPanel(wxWindow* parent);
	~ArgsPanel() {}

	void	setup(const Game::ArgSpec& args, bool udmf);
	void	setValues(int args[5]);
	int		getArgValue(int index);
	void	onSize(wxSizeEvent& event);
};

class GenLineSpecialPanel;
class MapObject;
class NumberTextCtrl;
class ActionSpecialPanel : public wxPanel
{
private:
	ActionSpecialTreeView*	tree_specials;
	wxPanel*				panel_action_special;
	GenLineSpecialPanel*	panel_gen_specials;
	wxRadioButton*			rb_special;
	wxRadioButton*			rb_generalised;
	ArgsPanel*				panel_args;
	vector<wxCheckBox*>		cb_triggers;
	vector<string>			triggers_udmf;
	wxChoice*				choice_trigger;
	bool					show_trigger;
	NumberTextCtrl*			text_special;

public:
	ActionSpecialPanel(wxWindow* parent, bool trigger = true);
	~ActionSpecialPanel();

	void	setupSpecialPanel();
	void	setArgsPanel(ArgsPanel* panel) { panel_args = panel; }
	void	setSpecial(int special);
	void	setTrigger(int index);
	int		selectedSpecial();
	void	showGeneralised(bool show = true);
	void	applyTo(vector<MapObject*>& lines, bool apply_special);
	void	openLines(vector<MapObject*>& lines);

	void	onRadioButtonChanged(wxCommandEvent& e);
	void	onSpecialSelectionChanged(wxDataViewEvent& e);
	void	onSpecialTextChanged(wxCommandEvent& e);
	void	onSpecialItemActivated(wxDataViewEvent& e);
};

class STabCtrl;
class ActionSpecialDialog : public SDialog
{
private:
	ActionSpecialPanel* panel_special;
	ArgsPanel*			panel_args;
	STabCtrl*			stc_tabs;

public:
	ActionSpecialDialog(wxWindow* parent, bool show_args = false);
	~ActionSpecialDialog();

	void	setSpecial(int special);
	void	setArgs(int args[5]);
	int		selectedSpecial();
	int		getArg(int index);
	void	applyTo(vector<MapObject*>& lines, bool apply_special);
	void	openLines(vector<MapObject*>& lines);
};

#endif//__ACTION_SPECIAL_DIALOG_H__
