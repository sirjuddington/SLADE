#pragma once

#include "General/ListenerAnnouncer.h"
#include "UI/Browser/BrowserWindow.h"

class Archive;
class TextureXList;
class PatchTable;

class PatchBrowserItem : public BrowserItem
{
public:
	PatchBrowserItem(
		string   name,
		Archive* archive = nullptr,
		uint8_t  type    = 0,
		string   nspace  = "",
		unsigned index   = 0) :
		BrowserItem{ name, index, "patch" },
		archive_{ archive },
		type_{ type },
		nspace_{ nspace }
	{
	}

	~PatchBrowserItem();

	bool   loadImage() override;
	string itemInfo() override;

private:
	Archive* archive_;
	uint8_t  type_; // 0=patch, 1=ctexture
	string   nspace_;
};

class PatchBrowser : public BrowserWindow, Listener
{
public:
	PatchBrowser(wxWindow* parent);
	~PatchBrowser() {}

	bool openPatchTable(PatchTable* table);
	bool openArchive(Archive* archive);
	bool openTextureXList(TextureXList* texturex, Archive* parent);
	int  selectedPatch();
	void selectPatch(int pt_index);
	void selectPatch(string name);
	void setFullPath(bool enabled) { full_path_ = enabled; }

private:
	PatchTable* patch_table_;
	bool        full_path_; // Texture definition format supports full path texture and/or patch names

	// Events
	void onAnnouncement(Announcer* announcer, const string& event_name, MemChunk& event_data) override;
};
