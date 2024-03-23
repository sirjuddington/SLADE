#pragma once

#include "UI/Browser/BrowserItem.h"
#include "UI/Browser/BrowserWindow.h"

namespace slade
{
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
		const wxString& name,
		Archive*        archive = nullptr,
		Type            type    = Type::Patch,
		const wxString& nspace  = "",
		unsigned        index   = 0) :
		BrowserItem{ name, index, "patch" },
		archive_{ archive },
		patch_type_{ type },
		nspace_{ nspace }
	{
	}

	~PatchBrowserItem() override;

	bool     loadImage() override;
	wxString itemInfo() override;
	void     clearImage() override;

private:
	Archive* archive_    = nullptr;
	Type     patch_type_ = Type::Patch;
	wxString nspace_;
};

class PatchBrowser : public BrowserWindow
{
public:
	PatchBrowser(wxWindow* parent);
	~PatchBrowser() override = default;

	bool openPatchTable(PatchTable* table);
	bool openArchive(Archive* archive);
	bool openTextureXList(TextureXList* texturex, Archive* parent);
	int  selectedPatch() const;
	void selectPatch(int pt_index);
	void selectPatch(const wxString& name);
	void setFullPath(bool enabled) { full_path_ = enabled; }

private:
	PatchTable* patch_table_ = nullptr;
	bool        full_path_   = false; // Texture definition format supports full path texture and/or patch names

	// Signal connections
	sigslot::scoped_connection sc_palette_changed_;
};
} // namespace slade
