#pragma once

#include "Archive/ArchiveEntry.h"
#include "General/ListenerAnnouncer.h"
#include "General/SAction.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"

class UndoManager;

class EntryPanel : public wxPanel, public Listener, protected SActionHandler
{
public:
	EntryPanel(wxWindow* parent, const wxString& id);
	~EntryPanel();

	wxString      name() const { return id_; }
	ArchiveEntry* entry() const { return entry_; }
	bool          isModified() const { return modified_; }
	bool          isActivePanel();
	void          setUndoManager(UndoManager* manager) { undo_manager_ = manager; }
	MemChunk*     entryData() { return &entry_data_; }

	bool             openEntry(ArchiveEntry* entry);
	virtual bool     loadEntry(ArchiveEntry* entry);
	virtual bool     saveEntry();
	virtual bool     revertEntry(bool confirm = true);
	virtual void     refreshPanel();
	virtual void     closeEntry();
	void             updateStatus();
	virtual wxString statusString() { return ""; }
	virtual void     addCustomMenu();
	void             removeCustomMenu() const;
	virtual bool     fillCustomMenu(wxMenu* custom) { return false; }
	wxString         customMenuName() const { return custom_menu_name_; }
	void             callRefresh() { refreshPanel(); }
	void             nullEntry() { entry_ = nullptr; }
	virtual bool     undo() { return false; }
	virtual bool     redo() { return false; }
	void             updateToolbar();
	virtual void     toolbarButtonClick(const wxString& action_id) {}

protected:
	MemChunk      entry_data_;
	ArchiveEntry* entry_        = nullptr;
	UndoManager*  undo_manager_ = nullptr;

	wxSizer*        sizer_main_   = nullptr;
	wxSizer*        sizer_bottom_ = nullptr;
	SToolBarButton* stb_save_     = nullptr;
	SToolBarButton* stb_revert_   = nullptr;

	wxMenu*   menu_custom_ = nullptr;
	wxString  custom_menu_name_;
	wxString  custom_toolbar_actions_;
	SToolBar* toolbar_ = nullptr;

	void setModified(bool c = true);

	void         onAnnouncement(Announcer* announcer, const wxString& event_name, MemChunk& event_data) override {}
	virtual bool handleEntryPanelAction(const wxString& id) { return false; }
	void         onToolbarButton(wxCommandEvent& e);

private:
	bool         modified_;
	wxStaticBox* frame_;
	wxString     id_;

	bool handleAction(const wxString& id) override
	{
		if (isActivePanel())
			return handleEntryPanelAction(id);

		return false;
	}
};
