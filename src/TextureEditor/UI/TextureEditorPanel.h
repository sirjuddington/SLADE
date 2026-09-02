#pragma once

#include "General/SActionHandler.h"

// Forward declarations
class wxSplitterWindow;
namespace slade
{
class UndoManager;
class PatchBrowser;
class SIconButton;
class CTexture;
class SAuiToolBar;
} // namespace slade
namespace slade::ui
{
class ZoomControl;
class Splitter;
} // namespace slade::ui
namespace slade::texeditor
{
class CTextureCanvasBase;
class TexturePropGrid;
class TextureEditor;
class TextureTreeView;
class PatchTablePanel;
} // namespace slade::texeditor

namespace slade::texeditor
{
class TextureEditorPanel : public wxPanel, public SActionHandler
{
public:
	TextureEditorPanel(wxWindow* parent, shared_ptr<Archive> archive);
	~TextureEditorPanel() override;

	Archive*     archive() const;
	wxMenu*      textureMenu() const { return menu_texture_; }
	UndoManager* undoManager() const;

	void undo();
	void redo();

	void saveAll() const;
	bool close();

private:
	unique_ptr<TextureEditor> editor_;

	wxMenu*       menu_texture_   = nullptr;
	PatchBrowser* patch_browser_  = nullptr;
	ui::Splitter* splitter_left_  = nullptr;
	ui::Splitter* splitter_right_ = nullptr;
	wxPanel*      panel_main_     = nullptr;
	wxPanel*      panel_blank_    = nullptr;

	// Left panel (texture list / patch table)
	TextureTreeView* textures_tree_view_ = nullptr;
	SAuiToolBar*     toolbar_texlist_    = nullptr;
	PatchTablePanel* patch_table_panel_  = nullptr;

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
	ui::Splitter*       splitter_props_  = nullptr;
	wxDataViewListCtrl* list_patches_    = nullptr;
	SAuiToolBar*        toolbar_patches_ = nullptr;
	TexturePropGrid*    pg_properties_   = nullptr;

	sigslot::scoped_connection sc_tex_modified_;
	sigslot::scoped_connection sc_tex_deleted_;

	wxPanel* createLeftPanel(wxWindow* parent);
	wxPanel* createTextureListPanel(wxWindow* parent);
	wxPanel* createPatchTablePanel(wxWindow* parent);
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
	void   dropPatchOnCanvas(string_view patch, int x, int y);
	void   updatePatchDropPreview(int x, int y);
	void   clearPatchDropPreview() const;

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
	void onBtnAutoOffset(wxCommandEvent& e);
	void onChoiceOffsetTypeSelected(wxCommandEvent& e);
};
} // namespace slade::texeditor
