#pragma once

#include "General/SActionHandler.h"

namespace slade
{
class PatchTable;
class PatchTableListView;
class GfxGLCanvas;
class TextureXEditor;
class SToolBar;

namespace ui
{
	class ZoomControl;
}

class PatchTablePanel : public wxPanel, SActionHandler
{
public:
	PatchTablePanel(wxWindow* parent, PatchTable* patch_table, TextureXEditor* tx_editor = nullptr);
	~PatchTablePanel() override = default;

private:
	PatchTable*         patch_table_      = nullptr;
	PatchTableListView* list_patches_     = nullptr;
	TextureXEditor*     parent_           = nullptr;
	GfxGLCanvas*        patch_canvas_     = nullptr;
	wxStaticText*       label_dimensions_ = nullptr;
	wxStaticText*       label_textures_   = nullptr;
	ui::ZoomControl*    zc_zoom_          = nullptr;
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
