#pragma once

#include "TextureXPanel.h"
#include "TextureEditorPanel.h"
#include "PatchTablePanel.h"
#include "Archive/Archive.h"
#include "Graphics/CTexture/PatchTable.h"
#include "PatchBrowser.h"
#include "UI/Controls/STabCtrl.h"

class UndoManager;

class TextureXEditor : public wxPanel, public Listener
{
public:
	TextureXEditor(wxWindow* parent);
	~TextureXEditor();

	Archive*		getArchive() { return archive_; }
	PatchTable&		patchTable() { return patch_table_; }
	void			pnamesModified(bool mod = true) { pnames_modified_ = mod; }
	UndoManager*	getUndoManager() { return undo_manager_; }

	bool	openArchive(Archive* archive);
	void	updateTexturePalette();
	void	saveChanges();
	bool	close();
	void	showTextureMenu(bool show = true);
	void	setSelection(size_t index);
	void	setSelection(ArchiveEntry* entry);
	void	updateMenuStatus();
	void	undo();
	void	redo();

	// Editing
	void	setFullPath(bool enabled = false) { patch_browser_->fullPath = enabled;	}
	bool	removePatch(unsigned index, bool delete_entry = false);
	int		browsePatchTable(string first = "");
	string	browsePatchEntry(string first = "");

	// Checks
	bool	checkTextures();

	// Static
	static bool	setupTextureEntries(Archive* archive);

private:
	Archive*					archive_			= nullptr;	// The archive this editor is handling
	ArchiveEntry*				pnames_				= nullptr;	// The PNAMES entry to modify (can be null)
	PatchTable					patch_table_;					// The patch table for TEXTURE1/2 (ie PNAMES)
	vector<TextureXPanel*>		texture_editors_;				// One panel per TEXTUREX list (ie TEXTURE1/TEXTURE2)
	PatchBrowser*				patch_browser_		= nullptr;	// The patch browser window
	UndoManager*				undo_manager_		= nullptr;

	// UI Stuff
	TabControl*	tabs_			= nullptr;
	wxButton*	btn_save_		= nullptr;
	wxMenu*		menu_texture_	= nullptr;

	bool	pb_update_			= true;
	bool	pnames_modified_	= false;

	// Events
	void	onSaveClicked(wxCommandEvent& e);
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;
	void	onShow(wxShowEvent& e);
};
