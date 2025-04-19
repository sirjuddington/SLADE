#pragma once

#include "Archive/ArchiveEntry.h"
#include "General/SAction.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"

namespace slade
{
class UndoManager;

class EntryPanel : public wxPanel, protected SActionHandler
{
public:
	EntryPanel(wxWindow* parent, const string& id, bool left_toolbar = false);
	~EntryPanel() override;

	string        name() const { return id_; }
	ArchiveEntry* entry() const { return entry_.lock().get(); }
	bool          isModified() const { return modified_; }
	bool          isActivePanel();
	void          setUndoManager(UndoManager* manager) { undo_manager_ = manager; }
	MemChunk*     entryData() { return &entry_data_; }
	void          addBorderPadding();

	bool           openEntry(ArchiveEntry* entry);
	bool           openEntry(shared_ptr<ArchiveEntry> entry);
	bool           saveEntry();
	virtual bool   revertEntry(bool confirm = true);
	virtual void   refreshPanel();
	virtual void   closeEntry();
	void           updateStatus();
	virtual string statusString() { return ""; }
	void           addCustomMenu(bool add_entry_menu = false) const;
	void           removeCustomMenu() const;
	virtual bool   fillCustomMenu(wxMenu* custom) { return false; }
	string         customMenuName() const { return custom_menu_name_; }
	void           callRefresh() { refreshPanel(); }
	void           nullEntry() { entry_.reset(); }
	virtual bool   undo() { return false; }
	virtual bool   redo() { return false; }
	void           updateToolbar();
	virtual void   toolbarButtonClick(const string& action_id) {}

protected:
	MemChunk               entry_data_;
	weak_ptr<ArchiveEntry> entry_;
	UndoManager*           undo_manager_ = nullptr;

	wxSizer*        sizer_main_   = nullptr;
	wxSizer*        sizer_bottom_ = nullptr;
	SToolBarButton* stb_save_     = nullptr;
	SToolBarButton* stb_revert_   = nullptr;

	wxMenu*   menu_custom_ = nullptr;
	string    custom_menu_name_;
	string    custom_toolbar_actions_;
	SToolBar* toolbar_      = nullptr;
	SToolBar* toolbar_left_ = nullptr;

	void         setModified(bool c = true);
	virtual bool loadEntry(ArchiveEntry* entry);
	virtual bool writeEntry(ArchiveEntry& entry) { return false; }
	virtual bool handleEntryPanelAction(string_view id) { return false; }
	void         onToolbarButton(wxCommandEvent& e);

private:
	bool   modified_ = false;
	string id_;

	bool handleStandaloneAction(string_view id);

	bool handleAction(string_view id) override
	{
		if (isActivePanel())
		{
			if (handleStandaloneAction(id))
				return true;

			return handleEntryPanelAction(id);
		}

		return false;
	}
};
} // namespace slade
