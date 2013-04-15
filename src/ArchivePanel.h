
#ifndef __ARCHIVEPANEL_H__
#define __ARCHIVEPANEL_H__

#include "Archive.h"
#include "EntryPanel.h"
#include "ListenerAnnouncer.h"
#include "ArchiveEntryList.h"
#include "MainApp.h"
#include "UndoRedo.h"
#include <wx/textctrl.h>
#include <wx/choice.h>

class ArchivePanel : public wxPanel, public Listener, SActionHandler {
protected:
	Archive*			archive;
	ArchiveEntryList*	entry_list;
	wxTextCtrl*			text_filter;
	wxChoice*			choice_category;
	wxStaticText*		label_path;
	wxBitmapButton*		btn_updir;
	wxSizer*			sizer_path_controls;
	UndoManager*		undo_manager;

	// Entry panels
	EntryPanel*	cur_area;
	EntryPanel*	entry_area;
	EntryPanel*	default_area;
	EntryPanel*	text_area;
	EntryPanel*	ansi_area;
	EntryPanel*	gfx_area;
	EntryPanel*	pal_area;
	EntryPanel* texturex_area;
	EntryPanel* animated_area;
	EntryPanel* switches_area;
	EntryPanel* pnames_area;
	EntryPanel* hex_area;
	EntryPanel* map_area;
	EntryPanel* audio_area;

	enum NewEntries {
		ENTRY_EMPTY = 0,
		ENTRY_PALETTE,
		ENTRY_ANIMATED,
		ENTRY_SWITCHES,
	};

public:
	ArchivePanel(wxWindow *parent, Archive* archive);
	~ArchivePanel();

	Archive*		getArchive() { return archive; }
	UndoManager*	getUndoManager() { return undo_manager; }
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

	// Entry manipulation actions
	bool	renameEntry(bool each = false);
	bool	deleteEntry(bool confirm = true);
	bool	revertEntry();
	bool	moveUp();
	bool	moveDown();
	bool	bookmark();
	bool	openTab();
	bool	convertEntryTo();
	bool	importEntry();
	bool	exportEntry();
	bool	exportEntryAs();
	bool	copyEntry();
	bool	cutEntry();
	bool	pasteEntry();

	// Other entry actions
	bool	gfxConvert();
	bool	gfxRemap();
	bool	gfxModifyOffsets();
	bool	gfxExportPNG();
	bool	basConvert();
	bool	palConvert();
	bool	reloadCurrentPanel();
	bool	wavDSndConvert();
	bool	dSndWavConvert();
	bool	musMidiConvert();
	bool	optimizePNG();
	bool	compileACS(bool hexen = false);
	bool	convertTextures();
	bool	mapOpenDb2();
	bool	crc32();

	// Needed for some console commands
	EntryPanel *			currentArea() { return cur_area;}
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
	void	focusEntryList() { entry_list->SetFocus(); }
	void	refreshPanel();

	// SAction handler
	bool	handleAction(string id);

	// Listener
	virtual void onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	// Static functions
	static EntryPanel*	createPanelForEntry(ArchiveEntry* entry, wxWindow* parent);

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
};

class EntryDataUS : public UndoStep {
private:
	MemChunk	data;
	string		path;
	unsigned	index;
	Archive*	archive;

public:
	EntryDataUS(ArchiveEntry* entry) {
		archive = entry->getParent();
		path = entry->getPath();
		index = entry->getParentDir()->entryIndex(entry);
		data.importMem(entry->getData(), entry->getSize());
	}

	bool swapData();

	bool doUndo() {
		return swapData();
	}

	bool doRedo() {
		return swapData();
	}
};

#endif //__ARCHIVEPANEL_H__
