#pragma once

#include "General/ListenerAnnouncer.h"
#include "General/UndoRedo.h"
#include "General/SAction.h"
#include "UI/Lists/ArchiveEntryList.h"
#include "MainEditor/ExternalEditManager.h"

class wxStaticText;
class wxBitmapButton;
class EntryPanel;

class ArchivePanel : public wxPanel, public Listener, SActionHandler
{
public:
	ArchivePanel(wxWindow* parent, Archive* archive);
	virtual ~ArchivePanel() {}

	Archive*		archive() const { return archive_; }
	UndoManager*	undoManager() const { return undo_manager_.get(); }
	bool			saveEntryChanges();
	void			addMenus();
	void			removeMenus();

	// Editing actions - return success

	// General actions
	void	undo();
	void	redo();

	// Archive manipulation actions
	bool	save();
	bool	saveAs();
	bool	newEntry(int type = ENTRY_EMPTY);
	bool	newDirectory();
	bool	importFiles();
	bool	convertArchiveTo();
	bool	cleanupArchive();
	bool	buildArchive();

	// Entry manipulation actions
	bool	renameEntry(bool each = false);
	bool	deleteEntry(bool confirm = true);
	bool	revertEntry();
	bool	moveUp();
	bool	moveDown();
	bool	sort();
	bool	bookmark();
	bool	openTab();
	bool	convertEntryTo();
	bool	importEntry();
	bool	exportEntry();
	bool	exportEntryAs();
	bool	copyEntry();
	bool	cutEntry();
	bool	pasteEntry();
	bool	openEntryExternal();

	// Other entry actions
	bool	gfxConvert();
	bool	gfxRemap();
	bool	gfxColourise();
	bool	gfxTint();
	bool	gfxModifyOffsets();
	bool	gfxExportPNG();
	bool	swanConvert();
	bool	basConvert(bool animdefs = false);
	bool	palConvert();
	bool	reloadCurrentPanel();
	bool	wavDSndConvert();
	bool	dSndWavConvert();
	bool	musMidiConvert();
	bool	optimizePNG();
	bool	compileACS(bool hexen = false);
	bool	convertTextures();
	bool	findTextureErrors();
	bool	mapOpenDb2();
	bool	crc32();

	// Needed for some console commands
	EntryPanel* 			currentArea() { return cur_area_; }
	ArchiveEntry*			currentEntry();
	vector<ArchiveEntry*>	currentEntries();
	ArchiveTreeNode*		currentDir();

	// UI related
	bool	openDir(ArchiveTreeNode* dir);
	bool	openEntry(ArchiveEntry* entry, bool force = false);
	bool	openEntryAsText(ArchiveEntry* entry);
	bool	openEntryAsHex(ArchiveEntry* entry);
	bool	showEntryPanel(EntryPanel* new_area, bool ask_save = true);
	void	focusOnEntry(ArchiveEntry* entry);
	void	focusEntryList() { entry_list_->SetFocus(); }
	void	refreshPanel();
	void	closeCurrentEntry();
	wxMenu*	createEntryOpenMenu(string category);

	// SAction handler
	bool	handleAction(string id) override;

	// Listener
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;

	// Static functions
	static EntryPanel*	createPanelForEntry(ArchiveEntry* entry, wxWindow* parent);

protected:
	Archive*			archive_				= nullptr;
	UndoManager::UPtr	undo_manager_;
	bool				ignore_focus_change_	= false;

	// External edit stuff
	ExternalEditManager::UPtr	ee_manager_;
	string						current_external_exe_category_;
	vector<string>				current_external_exes_;

	// Controls
	ArchiveEntryList*	entry_list_				= nullptr;
	wxTextCtrl*			text_filter_			= nullptr;
	wxButton*			btn_clear_filter_		= nullptr;
	wxChoice*			choice_category_		= nullptr;
	wxStaticText*		label_path_				= nullptr;
	wxBitmapButton*		btn_updir_				= nullptr;
	wxSizer*			sizer_path_controls_	= nullptr;

	// Entry panels
	EntryPanel*	cur_area_		= nullptr;
	EntryPanel*	entry_area_		= nullptr;
	EntryPanel*	default_area_	= nullptr;
	EntryPanel*	text_area_		= nullptr;
	EntryPanel*	ansi_area_		= nullptr;
	EntryPanel*	gfx_area_		= nullptr;
	EntryPanel*	pal_area_		= nullptr;
	EntryPanel* texturex_area_	= nullptr;
	EntryPanel* animated_area_	= nullptr;
	EntryPanel* switches_area_	= nullptr;
	EntryPanel* pnames_area_	= nullptr;
	EntryPanel* hex_area_		= nullptr;
	EntryPanel* map_area_		= nullptr;
	EntryPanel* audio_area_		= nullptr;
	EntryPanel* data_area_		= nullptr;

	enum NewEntries
	{
		ENTRY_EMPTY = 0,
		ENTRY_PALETTE,
		ENTRY_ANIMATED,
		ENTRY_SWITCHES,
	};

	// Events
	void			onEntryListSelectionChange(wxCommandEvent& e);
	void			onEntryListFocusChange(wxListEvent& e);
	void			onEntryListRightClick(wxListEvent& e);
	void			onEntryListKeyDown(wxKeyEvent& e);
	virtual void	onEntryListActivated(wxListEvent& e);
	void			onDEPEditAsText(wxCommandEvent& e);
	void			onDEPViewAsHex(wxCommandEvent& e);
	void			onMEPEditAsText(wxCommandEvent& e);
	void			onTextFilterChanged(wxCommandEvent& e);
	void			onChoiceCategoryChanged(wxCommandEvent& e);
	void			onDirChanged(wxCommandEvent& e);
	void			onBtnUpDir(wxCommandEvent& e);
	void			onBtnClearFilter(wxCommandEvent& e);
};

class EntryDataUS : public UndoStep
{
public:
	EntryDataUS(ArchiveEntry* entry) :
		path_{ entry->getPath() },
		index_{ (unsigned)entry->getParentDir()->entryIndex(entry) },
		archive_{ entry->getParent() }
	{
		data_.importMem(entry->getData(), entry->getSize());
	}

	bool swapData();

	bool doUndo() override
	{
		return swapData();
	}

	bool doRedo() override
	{
		return swapData();
	}

private:
	MemChunk	data_;
	string		path_;
	unsigned	index_;
	Archive*	archive_;
};
