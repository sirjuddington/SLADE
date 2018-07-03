#pragma once

#include "UI/Browser/BrowserWindow.h"
#include "General/ListenerAnnouncer.h"

class Archive;
class TextureXList;
class PatchTable;

class PatchBrowserItem : public BrowserItem
{
public:
	PatchBrowserItem(
		string name,
		Archive* archive = nullptr,
		uint8_t type = 0,
		string nspace = "",
		unsigned index = 0
	) : BrowserItem{ name, index, "patch" },
		archive_{ archive },
		type_{ type },
		nspace_{ nspace } {}

	~PatchBrowserItem();

	bool	loadImage() override;
	string	itemInfo() override;

private:
	Archive*	archive_;
	uint8_t		type_;		// 0=patch, 1=ctexture
	string		nspace_;
};

class PatchBrowser : public BrowserWindow, Listener
{
public:
	PatchBrowser(wxWindow* parent);
	~PatchBrowser() {}

	bool	openPatchTable(PatchTable* table);
	bool	openArchive(Archive* archive);
	bool	openTextureXList(TextureXList* texturex, Archive* parent);
	int		getSelectedPatch();
	void	selectPatch(int pt_index);
	void	selectPatch(string name);

	bool fullPath; // Texture definition format supports full path texture and/or patch names

private:
	PatchTable*	patch_table_;

	// Events
	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;
};
