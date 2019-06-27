#pragma once

#include "General/ListenerAnnouncer.h"
#include "General/SAction.h"
#include "General/UndoRedo.h"
#include "MainEditor/ExternalEditManager.h"
#include "UI/Lists/ArchiveEntryList.h"

class wxStaticText;
class wxBitmapButton;
class EntryPanel;

class ArchivePanel : public wxPanel, public Listener, SActionHandler
{
public:
	enum class NewEntry
	{
		Empty = 0,
		Palette,
		Animated,
		Switches,
	};

	ArchivePanel(wxWindow* parent, shared_ptr<Archive>& archive);
	virtual ~ArchivePanel() = default;

	Archive*     archive() const { return archive_.lock().get(); }
	UndoManager* undoManager() const { return undo_manager_.get(); }
	bool         saveEntryChanges() const;
	void         addMenus() const;
	void         removeMenus() const;

	// Editing actions - return success

	// General actions
	void undo() const;
	void redo() const;

	// Archive manipulation actions
	bool save();
	bool saveAs();
	bool newEntry(NewEntry type = NewEntry::Empty);
	bool newDirectory() const;
	bool importFiles();
	bool convertArchiveTo() const;
	bool cleanupArchive() const;
	bool buildArchive();

	// Entry manipulation actions
	bool renameEntry(bool each = false) const;
	bool deleteEntry(bool confirm = true);
	bool revertEntry() const;
	bool moveUp();
	bool moveDown();
	bool sort() const;
	bool bookmark() const;
	bool openTab() const;
	bool convertEntryTo() const;
	bool importEntry();
	bool exportEntry();
	bool exportEntryAs() const;
	bool copyEntry() const;
	bool cutEntry();
	bool pasteEntry() const;
	bool openEntryExternal();

	// Other entry actions
	bool gfxConvert() const;
	bool gfxRemap();
	bool gfxColourise();
	bool gfxTint();
	bool gfxModifyOffsets() const;
	bool gfxExportPNG();
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
	void    focusEntryList() const { entry_list_->SetFocus(); }
	void    refreshPanel();
	void    closeCurrentEntry();
	wxMenu* createEntryOpenMenu(const wxString& category);

	// SAction handler
	bool handleAction(string_view id) override;

	// Listener
	void onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data) override;

	// Static functions
	static EntryPanel* createPanelForEntry(ArchiveEntry* entry, wxWindow* parent);

protected:
	weak_ptr<Archive>       archive_;
	unique_ptr<UndoManager> undo_manager_;
	bool                    ignore_focus_change_ = false;

	// External edit stuff
	unique_ptr<ExternalEditManager> ee_manager_;
	string                          current_external_exe_category_;
	vector<string>                  current_external_exes_;

	// Controls
	ArchiveEntryList* entry_list_          = nullptr;
	wxTextCtrl*       text_filter_         = nullptr;
	wxButton*         btn_clear_filter_    = nullptr;
	wxChoice*         choice_category_     = nullptr;
	wxStaticText*     label_path_          = nullptr;
	wxBitmapButton*   btn_updir_           = nullptr;
	wxSizer*          sizer_path_controls_ = nullptr;

	// Entry panels
	EntryPanel* cur_area_      = nullptr;
	EntryPanel* entry_area_    = nullptr;
	EntryPanel* default_area_  = nullptr;
	EntryPanel* text_area_     = nullptr;
	EntryPanel* ansi_area_     = nullptr;
	EntryPanel* gfx_area_      = nullptr;
	EntryPanel* pal_area_      = nullptr;
	EntryPanel* texturex_area_ = nullptr;
	EntryPanel* pnames_area_   = nullptr;
	EntryPanel* hex_area_      = nullptr;
	EntryPanel* map_area_      = nullptr;
	EntryPanel* audio_area_    = nullptr;
	EntryPanel* data_area_     = nullptr;

	// Events
	void         onEntryListSelectionChange(wxCommandEvent& e);
	void         onEntryListFocusChange(wxListEvent& e);
	void         onEntryListRightClick(wxListEvent& e);
	void         onEntryListKeyDown(wxKeyEvent& e);
	virtual void onEntryListActivated(wxListEvent& e);
	void         onDEPEditAsText(wxCommandEvent& e);
	void         onDEPViewAsHex(wxCommandEvent& e);
	void         onMEPEditAsText(wxCommandEvent& e);
	void         onTextFilterChanged(wxCommandEvent& e);
	void         onChoiceCategoryChanged(wxCommandEvent& e);
	void         onDirChanged(wxCommandEvent& e);
	void         onBtnUpDir(wxCommandEvent& e);
	void         onBtnClearFilter(wxCommandEvent& e);
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
