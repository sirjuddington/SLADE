#pragma once

#include "General/SActionHandler.h"

// Forward declarations
class wxSplitterWindow;
namespace slade
{
class PatchBrowser;
class SIconButton;
class CTexture;
class SAuiToolBar;
class CTextureCanvasBase;
} // namespace slade
namespace slade::ui
{
class ZoomControl;
}
namespace slade::texeditor
{
class TexturePropGrid;
class TextureEditor;
class TextureTreeView;
} // namespace slade::texeditor

namespace slade::texeditor
{
class TextureEditorPanel : public wxPanel, public SActionHandler
{
public:
	TextureEditorPanel(wxWindow* parent, shared_ptr<Archive> archive);
	~TextureEditorPanel() override;

	Archive* archive() const;
	wxMenu*  textureMenu() const { return menu_texture_; }

private:
	unique_ptr<TextureEditor> editor_;

	wxMenu*           menu_texture_   = nullptr;
	PatchBrowser*     patch_browser_  = nullptr;
	wxSplitterWindow* splitter_left_  = nullptr;
	wxSplitterWindow* splitter_right_ = nullptr;
	wxPanel*          panel_main_     = nullptr;
	wxPanel*          panel_blank_    = nullptr;

	// Texture list
	TextureTreeView* tree_view_       = nullptr;
	SAuiToolBar*     toolbar_texlist_ = nullptr;

	// Texture view
	SAuiToolBar*        toolbar_texture_ = nullptr;
	CTextureCanvasBase* tex_canvas_      = nullptr;
	ui::ZoomControl*    zc_zoom_         = nullptr;

	// Offsets panel
	wxPanel*     panel_offsets_      = nullptr;
	wxSpinCtrl*  spin_offset_x_      = nullptr;
	wxSpinCtrl*  spin_offset_y_      = nullptr;
	wxChoice*    choice_offset_type_ = nullptr;
	SIconButton* btn_auto_offset_    = nullptr;

	// Texture/Patch properties
	wxSplitterWindow*   splitter_props_  = nullptr;
	wxDataViewListCtrl* list_patches_    = nullptr;
	SAuiToolBar*        toolbar_patches_ = nullptr;
	TexturePropGrid*    pg_properties_   = nullptr;

	sigslot::scoped_connection sc_tex_state_changed_;

	wxPanel* createTextureListPanel(wxWindow* parent);
	wxPanel* createMainPanel(wxWindow* parent);
	wxPanel* createTextureViewPanel(wxWindow* parent);
	wxPanel* createRightPanel(wxWindow* parent);
	wxPanel* createPatchListPanel(wxWindow* parent);
	wxPanel* createOffsetsPanel(wxWindow* parent);

	void updateUI(bool texture_changed = false);
	void populatePatchesList() const;

	void   initPatchBrowser();
	string browsePatch(string_view initial = {});
	void   addPatch();
	void   removePatch();
	void   replacePatch();
	void   duplicatePatch();
	void   pushPatch(bool forward);

	void newTexture();
	void newTextureFromFile();
	void deleteTexture() const;
	void moveTexture(Direction direction) const;
	void sortTextures() const;
	void renameTexture(bool each) const;
	void exportTexturesToEntries() const;
	void exportTexturesAsPNG() const;

	// SActionHandler
	bool handleAction(string_view id) override;

	// Events
	void onTextureSelectionChanged(wxDataViewEvent& e);
	void onPatchSelectionChanged(wxDataViewEvent& e);
	void onTreeViewKeyDown(wxKeyEvent& e);
	void onTexCanvasMouseEvent(wxMouseEvent& e);
	void onTexCanvasDragEnd(wxCommandEvent& e);
	void onTexCanvasKeyDown(wxKeyEvent& e);
	void onToolbarButton(wxCommandEvent& e);
	void onTexOffsetXChanged(wxCommandEvent& e);
	void onTexOffsetYChanged(wxCommandEvent& e);
	void onChoiceOffsetTypeSelected(wxCommandEvent& e);
};
} // namespace slade::texeditor
