#pragma once

#include "Archive/ArchiveEntry.h"
#include "General/SAction.h"
#include "General/UndoRedo.h"
#include "MainEditor/ExternalEditManager.h"
#include "UI/Lists/ArchiveEntryTree.h"
#include <wx/splitter.h>

class wxStaticText;
class wxBitmapButton;

namespace slade
{
class EntryPanel;
class SToolBar;

class ArchivePanel : public wxPanel, SActionHandler
{
public:
	ArchivePanel(wxWindow* parent, shared_ptr<Archive>& archive);
	~ArchivePanel() override = default;

	Archive*     archive() const { return archive_.lock().get(); }
	UndoManager* undoManager() const { return undo_manager_.get(); }
	bool         saveEntryChanges(bool force = false) const;
	void         addMenus() const;
	void         removeMenus() const;

	// Editing actions - return success

	// General actions
	void undo() const;
	void redo() const;

	// Archive manipulation actions
	bool newEntry();
	bool newDirectory();
	bool importFiles();
	bool importDir();
	bool convertArchiveTo() const;
	bool cleanupArchive() const;
	bool buildArchive();

	// Entry manipulation actions
	bool renameEntry(bool each = false) const;
	bool deleteEntry(bool confirm = true);
	bool revertEntry() const;
	bool moveUp() const;
	bool moveDown() const;
	bool sort() const;
	bool bookmark() const;
	bool openTab() const;
	bool convertEntryTo() const;
	bool importEntry();
	bool exportEntry() const;
	bool exportEntryAs() const;
	bool copyEntry() const;
	bool cutEntry();
	bool pasteEntry() const;
	bool openEntryExternal() const;

	// Other entry actions
	bool gfxConvert() const;
	bool gfxRemap();
	bool gfxColourise();
	bool gfxTint();
	bool gfxModifyOffsets() const;
	bool gfxExportPNG();
	bool voxelConvert() const;
	bool swanConvert() const;
	bool basConvert(bool animdefs = false);
	bool palConvert() const;
	bool reloadCurrentPanel();
	bool wavDSndConvert() const;
	bool dSndWavConvert() const;
	bool musMidiConvert() const;
	bool optimizePNG() const;
	bool compileACS(bool hexen = false) const;
	bool convertTextures() const;
	bool findTextureErrors() const;
	bool cleanTextureIwadDupes() const;
	bool cleanZdTextureSinglePatch() const;
	bool mapOpenDb2() const;
	bool crc32() const;

	// Needed for some console commands
	EntryPanel*           currentArea() const { return cur_area_; }
	ArchiveEntry*         currentEntry() const;
	vector<ArchiveEntry*> currentEntries() const;
	ArchiveDir*           currentDir() const;

	// UI related
	bool    openDir(const shared_ptr<ArchiveDir>& dir) const;
	bool    openEntry(ArchiveEntry* entry, bool force = false);
	bool    openEntryAsText(ArchiveEntry* entry);
	bool    openEntryAsHex(ArchiveEntry* entry);
	bool    showEntryPanel(EntryPanel* new_area, bool ask_save = true);
	void    focusOnEntry(ArchiveEntry* entry) const;
	void    focusEntryList() const { entry_tree_->SetFocus(); }
	void    refreshPanel();
	void    closeCurrentEntry();
	wxMenu* createEntryOpenMenu(const wxString& category);
	bool    switchToDefaultEntryPanel();

	// SAction handler
	bool handleAction(string_view id) override;

	// Static functions
	static EntryPanel* createPanelForEntry(const ArchiveEntry* entry, wxWindow* parent);
	static wxMenu*     createMaintenanceMenu();

protected:
	weak_ptr<Archive>       archive_;
	unique_ptr<UndoManager> undo_manager_;
	bool                    ignore_focus_change_ = false;

	// External edit stuff
	unique_ptr<ExternalEditManager> ee_manager_;
	string                          current_external_exe_category_;
	vector<string>                  current_external_exes_;

	// Controls
	ui::ArchiveEntryTree* entry_tree_       = nullptr;
	ui::ArchivePathPanel* etree_path_       = nullptr;
	wxTextCtrl*           text_filter_      = nullptr;
	wxButton*             btn_clear_filter_ = nullptr;
	wxChoice*             choice_category_  = nullptr;
	SToolBar*             toolbar_elist_    = nullptr;
	wxPanel*              panel_filter_     = nullptr;
	wxSplitterWindow*     splitter_         = nullptr;

	// Entry panels
	EntryPanel* cur_area_     = nullptr;
	EntryPanel* entry_area_   = nullptr;
	EntryPanel* default_area_ = nullptr;
	EntryPanel* text_area_    = nullptr;
	EntryPanel* ansi_area_    = nullptr;
	EntryPanel* gfx_area_     = nullptr;
	EntryPanel* pal_area_     = nullptr;
	EntryPanel* hex_area_     = nullptr;
	EntryPanel* map_area_     = nullptr;
	EntryPanel* audio_area_   = nullptr;
	EntryPanel* data_area_    = nullptr;

	// Signal connections
	sigslot::scoped_connection sc_archive_saved_;
	sigslot::scoped_connection sc_entry_removed_;
	sigslot::scoped_connection sc_bookmarks_changed_;

	bool canMoveEntries() const;
	void selectionChanged();
	void updateFilter() const;

	// Entry panel getters
	// (used to 'lazy load' the different entry panels as needed)
	EntryPanel* textArea();
	EntryPanel* ansiArea();
	EntryPanel* gfxArea();
	EntryPanel* palArea();
	EntryPanel* hexArea();
	EntryPanel* mapArea();
	EntryPanel* audioArea();
	EntryPanel* dataArea();

	// Events
	void         onEntryListSelectionChange(wxDataViewEvent& e);
	void         onEntryListRightClick(wxDataViewEvent& e);
	void         onEntryListKeyDown(wxKeyEvent& e);
	virtual void onEntryListActivated(wxDataViewEvent& e);
	void         onDEPEditAsText(wxCommandEvent& e);
	void         onDEPViewAsHex(wxCommandEvent& e);
	void         onMEPEditAsText(wxCommandEvent& e);
	void         onTextFilterChanged(wxCommandEvent& e);
	void         onChoiceCategoryChanged(wxCommandEvent& e);
	void         onBtnClearFilter(wxCommandEvent& e);

private:
	void     setup(Archive* archive);
	void     bindEvents(Archive* archive);
	wxPanel* createEntryListPanel(wxWindow* parent);
};

class EntryDataUS : public UndoStep
{
public:
	EntryDataUS(ArchiveEntry* entry) : path_{ entry->path() }, index_{ entry->index() }, archive_{ entry->parent() }
	{
		data_.importMem(entry->rawData(), entry->size());
	}

	bool swapData();
	bool doUndo() override { return swapData(); }
	bool doRedo() override { return swapData(); }

private:
	MemChunk data_;
	wxString path_;
	int      index_   = -1;
	Archive* archive_ = nullptr;
};
} // namespace slade
