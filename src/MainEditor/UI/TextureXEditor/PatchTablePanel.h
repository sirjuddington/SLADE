#pragma once

#include "General/SActionHandler.h"

namespace slade
{
class PatchTable;
class PatchTableListView;
class GfxCanvasBase;
class TextureXEditor;
class SAuiToolBar;
namespace ui
{
	class ZoomControl;
}

class PatchTablePanel : public wxPanel, SActionHandler
{
public:
	PatchTablePanel(wxWindow* parent, PatchTable* patch_table, TextureXEditor* tx_editor = nullptr);
	~PatchTablePanel() override = default;

	void setupLayout();

private:
	PatchTable*         patch_table_      = nullptr;
	PatchTableListView* list_patches_     = nullptr;
	TextureXEditor*     parent_           = nullptr;
	GfxCanvasBase*      patch_canvas_     = nullptr;
	wxStaticText*       label_dimensions_ = nullptr;
	wxStaticText*       label_textures_   = nullptr;
	ui::ZoomControl*    zc_zoom_          = nullptr;
	SAuiToolBar*        toolbar_          = nullptr;

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
