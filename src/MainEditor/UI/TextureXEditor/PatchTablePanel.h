#pragma once

#include "General/SAction.h"
#include "General/Sigslot.h"
#include "Graphics/CTexture/PatchTable.h"
#include "UI/Lists/VirtualListView.h"

namespace slade
{
class GfxCanvas;
class SZoomSlider;
class TextureXEditor;
class SToolBar;

class PatchTableListView : public VirtualListView
{
protected:
	wxString itemText(long item, long column, long index) const override;
	void     updateItemAttr(long item, long column, long index) const override;

public:
	PatchTableListView(wxWindow* parent, PatchTable* patch_table);
	~PatchTableListView() = default;

	PatchTable* patchTable() const { return patch_table_; }

	void        updateList(bool clear = false) override;
	static bool usageSort(long left, long right);
	void        sortItems() override;

private:
	PatchTable* patch_table_ = nullptr;

	ScopedConnectionList signal_connections_;
};


class PatchTablePanel : public wxPanel, SActionHandler
{
public:
	PatchTablePanel(wxWindow* parent, PatchTable* patch_table, TextureXEditor* tx_editor = nullptr);
	~PatchTablePanel() = default;

private:
	PatchTable*         patch_table_      = nullptr;
	PatchTableListView* list_patches_     = nullptr;
	TextureXEditor*     parent_           = nullptr;
	GfxCanvas*          patch_canvas_     = nullptr;
	wxStaticText*       label_dimensions_ = nullptr;
	wxStaticText*       label_textures_   = nullptr;
	SZoomSlider*        slider_zoom_      = nullptr;
	SToolBar*           toolbar_          = nullptr;

	void setupLayout();
	void updateDisplay();

	void addPatch();
	void addPatchFromFile();
	void removePatch();
	void changePatch();

	// Signal connections
	sigslot::scoped_connection sc_palette_changed_;

	// SAction handler
	bool handleAction(string_view id) override;

	// Events
	void onDisplayChanged(wxCommandEvent& e);
};
} // namespace slade
