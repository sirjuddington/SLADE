#pragma once

#include "UI/Lists/VirtualListView.h"

namespace slade
{
class UndoManager;

class UndoListView : public VirtualListView
{
public:
	UndoListView(wxWindow* parent, UndoManager* manager);
	~UndoListView() override = default;

	void setManager(UndoManager* manager);

protected:
	// Virtual wxListCtrl overrides
	wxString itemText(long item, long column, long index) const override;
	int      itemIcon(long item, long column, long index) const override;
	void     updateItemAttr(long item, long column, long index) const override;

private:
	UndoManager* manager_ = nullptr;

	// Signal connections
	sigslot::scoped_connection sc_recorded_;
	sigslot::scoped_connection sc_undo_;
	sigslot::scoped_connection sc_redo_;

	void updateFromManager();
	void connectManagerSignals();
};

class UndoManagerHistoryPanel : public wxPanel
{
public:
	UndoManagerHistoryPanel(wxWindow* parent, UndoManager* manager);
	~UndoManagerHistoryPanel() override = default;

	void setManager(UndoManager* manager);

private:
	UndoManager*  manager_     = nullptr;
	UndoListView* list_levels_ = nullptr;

	// Events
	void onItemRightClick(wxCommandEvent& e);
	void onMenu(wxCommandEvent& e);
};
} // namespace slade
