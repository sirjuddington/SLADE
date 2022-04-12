#pragma once

#include "UI/Controls/STabCtrl.h"
#include "UI/SDialog.h"

class wxFlexGridSizer;

namespace slade
{
namespace game
{
	struct ArgSpec;
}

class ArgsControl;
class GenLineSpecialPanel;
class MapObject;
class NumberTextCtrl;

// A wxDataViewTreeCtrl specialisation showing the action specials and groups in a tree structure
class ActionSpecialTreeView : public wxDataViewTreeCtrl
{
public:
	ActionSpecialTreeView(wxWindow* parent);
	~ActionSpecialTreeView() = default;

	void setParentDialog(wxDialog* dlg) { parent_dialog_ = dlg; }

	int  specialNumber(wxDataViewItem item) const;
	void showSpecial(int special, bool focus = true);
	int  selectedSpecial() const;

private:
	wxDataViewItem root_;
	wxDataViewItem item_none_;
	wxDialog*      parent_dialog_ = nullptr;

	struct ASTVGroup
	{
		wxString       name;
		wxDataViewItem item;
		ASTVGroup(wxDataViewItem i, const wxString& name) : name{ name }, item{ i } {}
	};
	vector<ASTVGroup> groups_;

	wxDataViewItem getGroup(const wxString& group_name);
};

class ArgsPanel : public wxScrolled<wxPanel>
{
public:
	ArgsPanel(wxWindow* parent);
	~ArgsPanel() = default;

	void setup(const game::ArgSpec& args, bool udmf);
	void setValues(int args[5]);
	int  argValue(int index);
	void onSize(wxSizeEvent& event);

private:
	wxFlexGridSizer* fg_sizer_           = nullptr;
	ArgsControl*     control_args_[5]    = {};
	wxStaticText*    label_args_[5]      = {};
	wxStaticText*    label_args_desc_[5] = {};
};

class ActionSpecialPanel : public wxPanel
{
public:
	ActionSpecialPanel(wxWindow* parent, bool trigger = true);
	~ActionSpecialPanel() = default;

	void setupSpecialPanel();
	void setArgsPanel(ArgsPanel* panel) { panel_args_ = panel; }
	void setSpecial(int special);
	void setTrigger(int index);
	void setTrigger(const wxString& trigger);
	void clearTrigger();
	int  selectedSpecial() const;
	void showGeneralised(bool show = true);
	void applyTo(vector<MapObject*>& lines, bool apply_special);
	void openLines(vector<MapObject*>& lines);

	void onRadioButtonChanged(wxCommandEvent& e);
	void onSpecialSelectionChanged(wxDataViewEvent& e);
	void onSpecialItemActivated(wxDataViewEvent& e);
	void onSpecialPresetClicked(wxCommandEvent& e);

private:
	ActionSpecialTreeView* tree_specials_        = nullptr;
	wxPanel*               panel_action_special_ = nullptr;
	GenLineSpecialPanel*   panel_gen_specials_   = nullptr;
	wxRadioButton*         rb_special_           = nullptr;
	wxRadioButton*         rb_generalised_       = nullptr;
	ArgsPanel*             panel_args_           = nullptr;
	wxChoice*              choice_trigger_       = nullptr;
	bool                   show_trigger_         = false;
	NumberTextCtrl*        text_special_         = nullptr;
	wxButton*              btn_preset_           = nullptr;

	struct FlagHolder
	{
		wxCheckBox* check_box;
		int         index;
		wxString    udmf;
	};
	vector<FlagHolder> flags_;
};

class ActionSpecialDialog : public SDialog
{
public:
	ActionSpecialDialog(wxWindow* parent, bool show_args = false);
	~ActionSpecialDialog() = default;

	void setSpecial(int special) const;
	void setArgs(int args[5]) const;
	int  selectedSpecial() const;
	int  argValue(int index) const;
	void applyTo(vector<MapObject*>& lines, bool apply_special) const;
	void openLines(vector<MapObject*>& lines) const;

private:
	ActionSpecialPanel* panel_special_ = nullptr;
	ArgsPanel*          panel_args_    = nullptr;
	TabControl*         stc_tabs_      = nullptr;
};
} // namespace slade
