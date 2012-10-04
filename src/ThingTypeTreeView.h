
#ifndef __THING_TYPE_TREE_VIEW_H__
#define __THING_TYPE_TREE_VIEW_H__

#include <wx/dataview.h>

class ThingTypeTreeView : public wxDataViewTreeCtrl {
private:
	wxDataViewItem	root;
	wxDialog*		parent_dialog;

	// It's incredibly retarded that I actually have to do this
	struct group_t {
		string			name;
		wxDataViewItem	item;
		group_t(wxDataViewItem i, string name) { this->item = i; this->name = name; }
	};
	vector<group_t> groups;

	wxDataViewItem	getGroup(string group);

public:
	ThingTypeTreeView(wxWindow* parent);
	~ThingTypeTreeView();

	void	setParentDialog(wxDialog* dlg) { parent_dialog = dlg; }

	int		typeNumber(wxDataViewItem item);
	void	showType(int special);
	int		selectedType();

	void	onItemEdit(wxDataViewEvent& e);
	void	onItemActivated(wxDataViewEvent& e);

	static int showDialog(wxWindow* parent, int init = -1);
};

#endif//__THING_TYPE_TREE_VIEW_H__
