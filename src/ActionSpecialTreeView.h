
#ifndef __ACTION_SPECIAL_TREE_VIEW_H__
#define __ACTION_SPECIAL_TREE_VIEW_H__

#include <wx/dataview.h>

class ActionSpecialTreeView : public wxDataViewTreeCtrl {
private:
	wxDataViewItem	root;
	wxDialog*		parent_dialog;

	// It's incredibly retarded that I actually have to do this
	struct astv_group_t {
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

	static int showDialog(wxWindow* parent, int init = -1);
};

#endif//__ACTION_SPECIAL_TREE_VIEW_H__
