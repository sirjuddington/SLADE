#pragma once

#include "UI/SDialog.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/WxBasicControls.h"

namespace Game { struct ArgSpec; }
class wxFlexGridSizer;
class ArgsControl;
class GenLineSpecialPanel;
class MapObject;
class NumberTextCtrl;

// A wxDataViewTreeCtrl specialisation showing the
// action specials and groups in a tree structure
class ActionSpecialTreeView : public wxDataViewTreeCtrl
{
public:
	ActionSpecialTreeView(wxWindow* parent);
	~ActionSpecialTreeView() {}

	void	setParentDialog(wxDialog* dlg) { parent_dialog_ = dlg; }

	int		specialNumber(wxDataViewItem item);
	void	showSpecial(int special, bool focus = true);
	int		selectedSpecial();

	void	onItemEdit(wxDataViewEvent& e);
	void	onItemActivated(wxDataViewEvent& e);

private:
	wxDataViewItem	root_;
	wxDataViewItem	item_none_;
	wxDialog*		parent_dialog_ = nullptr;

	// It's incredibly retarded that I actually have to do this
	struct ASTVGroup
	{
		string			name;
		wxDataViewItem	item;
		ASTVGroup(wxDataViewItem i, string name) { this->item = i; this->name = name; }
	};
	vector<ASTVGroup> groups_;

	wxDataViewItem	getGroup(string group);
};

class ArgsPanel : public wxScrolled<wxPanel>
{
public:
	ArgsPanel(wxWindow* parent);
	~ArgsPanel() {}

	void	setup(const Game::ArgSpec& args, bool udmf);
	void	setValues(int args[5]);
	int		getArgValue(int index);
	void	onSize(wxSizeEvent& event);

private:
	wxFlexGridSizer*	fg_sizer_			= nullptr;
	ArgsControl*		control_args_[5]	= { nullptr, nullptr, nullptr, nullptr, nullptr };
	wxStaticText*		label_args_[5]		= { nullptr, nullptr, nullptr, nullptr, nullptr };
	wxStaticText*		label_args_desc_[5]	= { nullptr, nullptr, nullptr, nullptr, nullptr };
};

class ActionSpecialPanel : public wxPanel
{
public:
	ActionSpecialPanel(wxWindow* parent, bool trigger = true);
	~ActionSpecialPanel() {}

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
public:
	ActionSpecialDialog(wxWindow* parent, bool show_args = false);
	~ActionSpecialDialog() {}

	void	setSpecial(int special);
	void	setArgs(int args[5]);
	int		selectedSpecial();
	int		getArg(int index);
	void	applyTo(vector<MapObject*>& lines, bool apply_special);
	void	openLines(vector<MapObject*>& lines);

private:
	ActionSpecialPanel* panel_special_	= nullptr;
	ArgsPanel*			panel_args_		= nullptr;
	TabControl*			stc_tabs_		= nullptr;
};
