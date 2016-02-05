
#ifndef __PATCH_BROWSER_H__
#define __PATCH_BROWSER_H__

#include "UI/Browser/BrowserWindow.h"
#include "General/ListenerAnnouncer.h"
#include "PatchTable.h"

class PatchBrowserItem : public BrowserItem
{
private:
	Archive*	archive;
	uint8_t		type;		// 0=patch, 1=ctexture
	string		nspace;

public:
	PatchBrowserItem(string name, Archive* archive = NULL, uint8_t type = 0, string nspace = "", unsigned index = 0);
	~PatchBrowserItem();

	bool	loadImage();
	string	itemInfo();
};

class TextureXList;
class PatchBrowser : public BrowserWindow, Listener
{
private:
	PatchTable*		patch_table;

public:
	PatchBrowser(wxWindow* parent);
	~PatchBrowser();

	bool	openPatchTable(PatchTable* table);
	bool	openArchive(Archive* archive);
	bool	openTextureXList(TextureXList* texturex, Archive* parent);
	int		getSelectedPatch();
	void	selectPatch(int pt_index);
	void	selectPatch(string name);

	// Events
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

#endif//__PATCH_BROWSER_H__
