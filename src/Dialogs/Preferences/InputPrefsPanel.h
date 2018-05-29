#pragma once

#include "common.h"
#include "PrefsPanelBase.h"
#include "General/KeyBind.h"

class InputKeyCtrl : public wxTextCtrl
{
public:
	InputKeyCtrl(wxWindow* parent, keypress_t init);

	keypress_t	key() const { return key_; }

private:
	keypress_t key_;

	// Events
	void	onKeyDown(wxKeyEvent& e);
	void	onMouseDown(wxMouseEvent& e);
	void	onEnter(wxCommandEvent& e);
};

class InputPrefsPanel : public PrefsPanelBase
{
public:
	InputPrefsPanel(wxWindow* parent);
	~InputPrefsPanel();

	wxTreeListItem	getListGroupItem(string group);
	void			initBindsList();
	void			updateBindsList();
	void			changeKey(wxTreeListItem item);
	void			addKey();
	void			removeKey(wxTreeListItem item);

	void			init() override;
	void			applyPreferences() override;

	string pageTitle() override { return "Keyboard Shortcuts"; }

private:
	wxTreeListCtrl*	list_binds_;
	wxButton*		btn_add_;
	wxButton*		btn_remove_;
	wxButton*		btn_change_;
	wxButton*		btn_defaults_;

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
