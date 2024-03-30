#pragma once

#include "UI/Browser/BrowserWindow.h"

namespace slade
{
class TextureXList;
class PatchTable;

class PatchBrowser : public BrowserWindow
{
public:
	PatchBrowser(wxWindow* parent);
	~PatchBrowser() override = default;

	bool openPatchTable(PatchTable* table);
	bool openArchive(Archive* archive);
	bool openTextureXList(const TextureXList* texturex, Archive* parent);
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
