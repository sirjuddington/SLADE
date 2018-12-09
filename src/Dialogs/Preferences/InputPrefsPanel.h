#pragma once

#include "General/KeyBind.h"
#include "PrefsPanelBase.h"

class InputKeyCtrl : public wxTextCtrl
{
public:
	InputKeyCtrl(wxWindow* parent, Keypress init);

	const Keypress& key() const { return key_; }

private:
	Keypress key_;

	// Events
	void onKeyDown(wxKeyEvent& e);
	void onMouseDown(wxMouseEvent& e);
	void onEnter(wxCommandEvent& e);
};

class InputPrefsPanel : public PrefsPanelBase
{
public:
	InputPrefsPanel(wxWindow* parent);
	~InputPrefsPanel() = default;

	wxTreeListItem getListGroupItem(const string& group) const;
	void           initBindsList() const;
	void           updateBindsList() const;
	void           changeKey(wxTreeListItem item);
	void           addKey();
	void           removeKey(wxTreeListItem item) const;

	void init() override;
	void applyPreferences() override;

	string pageTitle() override { return "Keyboard Shortcuts"; }

private:
	wxTreeListCtrl* list_binds_   = nullptr;
	wxButton*       btn_add_      = nullptr;
	wxButton*       btn_remove_   = nullptr;
	wxButton*       btn_change_   = nullptr;
	wxButton*       btn_defaults_ = nullptr;

	// Events
	void onSize(wxSizeEvent& e);
	void onListSelectionChanged(wxTreeListEvent& e);
	void onListItemActivated(wxTreeListEvent& e);
	void onBtnChangeKey(wxCommandEvent& e);
	void onBtnDefaults(wxCommandEvent& e);
	void onListKeyDown(wxKeyEvent& e);
};
