
#ifndef __ACTION_SPECIAL_DIALOG_H__
#define __ACTION_SPECIAL_DIALOG_H__

#include <wx/dataview.h>
#include <wx/notebook.h>

// A wxDataViewTreeCtrl specialisation showing the
// action specials and groups in a tree structure
class ActionSpecialTreeView : public wxDataViewTreeCtrl
{
private:
	wxDataViewItem	root;
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
	void	showSpecial(int special);
	int		selectedSpecial();

	void	onItemEdit(wxDataViewEvent& e);
	void	onItemActivated(wxDataViewEvent& e);
};

class GenLineSpecialPanel;
class ActionSpecialDialog : public wxDialog
{
private:
	wxNotebook*				nb_tabs;
	ActionSpecialTreeView*	tree_specials;
	GenLineSpecialPanel*	panel_gen_specials;

public:
	ActionSpecialDialog(wxWindow* parent);
	~ActionSpecialDialog();

	void	setSpecial(int special);
	int		selectedSpecial();
};

#endif//__ACTION_SPECIAL_DIALOG_H__
