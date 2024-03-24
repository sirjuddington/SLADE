#pragma once

#include "PrefsPanelBase.h"

namespace slade
{
struct Keypress;
class InputKeyCtrl : public wxTextCtrl
{
public:
	InputKeyCtrl(wxWindow* parent, const Keypress& init);
	~InputKeyCtrl() override;

	const Keypress& key() const { return *key_; }

private:
	unique_ptr<Keypress> key_;

	// Events
	void onKeyDown(wxKeyEvent& e);
	void onMouseDown(wxMouseEvent& e);
	void onEnter(wxCommandEvent& e);
};

class InputPrefsPanel : public PrefsPanelBase
{
public:
	InputPrefsPanel(wxWindow* parent);
	~InputPrefsPanel() override = default;

	wxTreeListItem getListGroupItem(const wxString& group) const;
	void           initBindsList() const;
	void           updateBindsList() const;
	void           changeKey(wxTreeListItem item);
	void           addKey();
	void           removeKey(wxTreeListItem item) const;

	void init() override;
	void applyPreferences() override;

	wxString pageTitle() override { return "Keyboard Shortcuts"; }

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
} // namespace slade
