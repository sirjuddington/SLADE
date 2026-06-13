#pragma once

// Forward declarations
class wxSplitterWindow;
namespace slade
{
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
class TextureEditorPanel : public wxPanel
{
public:
	TextureEditorPanel(wxWindow* parent, shared_ptr<Archive> archive);
	~TextureEditorPanel() override;

private:
	unique_ptr<TextureEditor> editor_;
	TextureTreeView*          tree_view_          = nullptr;
	wxSplitterWindow*         splitter_left_      = nullptr;
	wxSplitterWindow*         splitter_right_     = nullptr;
	CTextureCanvasBase*       tex_canvas_         = nullptr;
	SAuiToolBar*              toolbar_texlist_    = nullptr;
	TexturePropGrid*          pg_properties_      = nullptr;
	ui::ZoomControl*          zc_zoom_            = nullptr;
	wxDataViewListCtrl*       list_patches_       = nullptr;
	SAuiToolBar*              toolbar_patches_    = nullptr;
	wxSpinCtrl*               spin_offset_x_      = nullptr;
	wxSpinCtrl*               spin_offset_y_      = nullptr;
	wxChoice*                 choice_offset_type_ = nullptr;
	SIconButton*              btn_auto_offset_    = nullptr;

	wxPanel* createTextureListPanel(wxWindow* parent);
	wxPanel* createMainPanel(wxWindow* parent);
	wxPanel* createTextureViewPanel(wxWindow* parent);
	wxPanel* createPatchPropertiesPanel(wxWindow* parent);
	void     setupPropertyGrid() const;

	void openTexture(CTexture& tex) const;
	void clearTexture() const;

	// Events
	void onTextureSelectionChanged(wxDataViewEvent& e);
	void onPatchSelectionChanged(wxDataViewEvent& e);
	void onTexCanvasMouseEvent(wxMouseEvent& e);
	void onTexCanvasDragEnd(wxCommandEvent& e);
	void onTexCanvasKeyDown(wxKeyEvent& e);
};
} // namespace slade::texeditor
