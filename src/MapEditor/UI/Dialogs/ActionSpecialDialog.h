
#ifndef __ACTION_SPECIAL_DIALOG_H__
#define __ACTION_SPECIAL_DIALOG_H__

#include "common.h"
#include "UI/SDialog.h"
#include "UI/STabCtrl.h"
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
public:
	ActionSpecialPanel(wxWindow* parent, bool trigger = true);
	~ActionSpecialPanel();

	void	setupSpecialPanel();
	void	setArgsPanel(ArgsPanel* panel) { panel_args_ = panel; }
	void	setSpecial(int special);
	void	setTrigger(int index);
	void	setTrigger(string trigger);
	void	clearTrigger();
	int		selectedSpecial();
	void	showGeneralised(bool show = true);
	void	applyTo(vector<MapObject*>& lines, bool apply_special);
	void	openLines(vector<MapObject*>& lines);

	void	onRadioButtonChanged(wxCommandEvent& e);
	void	onSpecialSelectionChanged(wxDataViewEvent& e);
	void	onSpecialItemActivated(wxDataViewEvent& e);
	void	onSpecialPresetClicked(wxCommandEvent& e);

private:
	ActionSpecialTreeView*	tree_specials_;
	wxPanel*				panel_action_special_;
	GenLineSpecialPanel*	panel_gen_specials_;
	wxRadioButton*			rb_special_;
	wxRadioButton*			rb_generalised_;
	ArgsPanel*				panel_args_;
	wxChoice*				choice_trigger_;
	bool					show_trigger_;
	NumberTextCtrl*			text_special_;
	wxButton*				btn_preset_;

	struct FlagHolder
	{
		wxCheckBox*	check_box;
		int			index;
		string		udmf;
	};
	vector<FlagHolder>	flags_;
};

class ActionSpecialDialog : public SDialog
{
private:
	ActionSpecialPanel* panel_special;
	ArgsPanel*			panel_args;
	TabControl*			stc_tabs;

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
