
#ifndef __TEXTUREX_EDITOR_H__
#define __TEXTUREX_EDITOR_H__

#include "TextureXPanel.h"
#include "TextureEditorPanel.h"
#include "PatchTablePanel.h"
#include "Archive/Archive.h"
#include "Graphics/CTexture/PatchTable.h"
#include "PatchBrowser.h"
#include "UI/STabCtrl.h"

class UndoManager;
class TextureXEditor : public wxPanel, public Listener
{
private:
	Archive*					archive;			// The archive this editor is handling
	ArchiveEntry*				pnames;				// The PNAMES entry to modify (can be null)
	PatchTable					patch_table;		// The patch table for TEXTURE1/2 (ie PNAMES)
	vector<TextureXPanel*>		texture_editors;	// One panel per TEXTUREX list (ie TEXTURE1/TEXTURE2)
	PatchBrowser*				patch_browser;		// The patch browser window
	UndoManager*				undo_manager;

	// UI Stuff
	TabControl*	tabs;
	wxButton*	btn_save;
	wxMenu*		menu_texture;

	bool	pb_update;
	bool	pnames_modified;

public:
	TextureXEditor(wxWindow* parent);
	~TextureXEditor();

	Archive*		getArchive() { return archive; }
	PatchTable&		patchTable() { return patch_table; }
	void			pnamesModified(bool mod = true) { pnames_modified = mod; }
	UndoManager*	getUndoManager() { return undo_manager; }

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
	bool	removePatch(unsigned index, bool delete_entry = false);
	int		browsePatchTable(string first = "");
	string	browsePatchEntry(string first = "");

	// Checks
	bool	checkTextures();

	// Events
	void	onSaveClicked(wxCommandEvent& e);
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
	void	onShow(wxShowEvent& e);

	// Static
	static bool	setupTextureEntries(Archive* archive);
};

#endif//__TEXTUREX_EDITOR_H__
