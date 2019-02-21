#pragma once

#include "General/ListenerAnnouncer.h"
#include "UI/Browser/BrowserWindow.h"

class Archive;
class TextureXList;
class PatchTable;

class PatchBrowserItem : public BrowserItem
{
public:
	enum class Type
	{
		Patch,
		CTexture
	};

	PatchBrowserItem(
		wxString name,
		Archive* archive = nullptr,
		Type     type    = Type::Patch,
		wxString nspace  = "",
		unsigned index   = 0) :
		BrowserItem{ name, index, "patch" },
		archive_{ archive },
		type_{ type },
		nspace_{ nspace }
	{
	}

	~PatchBrowserItem();

	bool     loadImage() override;
	wxString itemInfo() override;
	void     clearImage() override;

private:
	Archive* archive_ = nullptr;
	Type     type_    = Type::Patch;
	wxString nspace_;
};

class PatchBrowser : public BrowserWindow, Listener
{
public:
	PatchBrowser(wxWindow* parent);
	~PatchBrowser() = default;

	bool openPatchTable(PatchTable* table);
	bool openArchive(Archive* archive);
	bool openTextureXList(TextureXList* texturex, Archive* parent);
	int  selectedPatch();
	void selectPatch(int pt_index);
	void selectPatch(const wxString& name);
	void setFullPath(bool enabled) { full_path_ = enabled; }

private:
	PatchTable* patch_table_ = nullptr;
	bool        full_path_   = false; // Texture definition format supports full path texture and/or patch names

	// Events
	void onAnnouncement(Announcer* announcer, std::string_view event_name, MemChunk& event_data) override;
};
