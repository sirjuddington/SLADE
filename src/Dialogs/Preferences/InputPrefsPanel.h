
#ifndef __INPUT_PREFS_PANEL_H__
#define __INPUT_PREFS_PANEL_H__

#include "PrefsPanelBase.h"
#include "General/KeyBind.h"
#include <wx/textctrl.h>
#include <wx/treelist.h>

class InputKeyCtrl : public wxTextCtrl
{
private:
	keypress_t key;

public:
	InputKeyCtrl(wxWindow* parent, keypress_t init);

	keypress_t	getKey() { return key; }

	// Events
	void	onKeyDown(wxKeyEvent& e);
	void	onMouseDown(wxMouseEvent& e);
	void	onEnter(wxCommandEvent& e);
};

class InputPrefsPanel : public PrefsPanelBase
{
private:
	wxTreeListCtrl*	list_binds;
	wxButton*		btn_add;
	wxButton*		btn_remove;
	wxButton*		btn_change;
	wxButton*		btn_defaults;

public:
	InputPrefsPanel(wxWindow* parent);
	~InputPrefsPanel();

	wxTreeListItem	getListGroupItem(string group);
	void			initBindsList();
	void			updateBindsList();
	void			changeKey(wxTreeListItem item);
	void			addKey();
	void			removeKey(wxTreeListItem item);

	void			init();
	void			applyPreferences();

	// Events
	void	onSize(wxSizeEvent& e);
	void	onListSelectionChanged(wxTreeListEvent& e);
	void	onListItemActivated(wxTreeListEvent& e);
	void	onBtnChangeKey(wxCommandEvent& e);
	void	onBtnAddKey(wxCommandEvent& e);
	void	onBtnRemoveKey(wxCommandEvent& e);
	void	onBtnDefaults(wxCommandEvent& e);
	void	onListKeyDown(wxKeyEvent& e);
};

#endif//__INPUT_PREFS_PANEL_H__
