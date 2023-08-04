#pragma once

#include "UI/Lists/ListView.h"

namespace slade
{
class TextureXEditor;
class TextureXList;
class CTextureCanvas;
class CTexture;
class Palette;
class SToolBar;
namespace ui
{
	class ZoomControl;
}

class TextureEditorPanel : public wxPanel
{
public:
	TextureEditorPanel(wxWindow* parent, TextureXEditor* tx_editor);
	~TextureEditorPanel() override = default;

	bool      texModified() const { return tex_modified_; }
	CTexture* texture() const { return tex_current_.get(); }

	// UI Stuff
	virtual void     setupLayout();
	virtual wxPanel* createTextureControls(wxWindow* parent);
	virtual void     updateTextureControls();
	virtual void     updateTextureScaleLabel();
	virtual wxPanel* createPatchControls(wxWindow* parent);
	virtual void     populatePatchList();
	virtual void     updatePatchControls();
	void             updateTextureName(const wxString& new_name) const;

	bool openTexture(const CTexture* tex, TextureXList* list);
	void clearTexture();
	void setPalette(const Palette* pal) const;

	const Palette* palette() const;
	bool           blendRGBA() const;

	// Editing
	virtual void addPatch();
	void         removePatch();
	void         patchBack();
	void         patchForward();
	virtual void replacePatch();
	void         duplicatePatch(int xoff = 8, int yoff = 8);

	// SAction handler
	bool handleSAction(string_view id);

	// Events
	void         onDrawOutsideChanged(wxCommandEvent& e);
	void         onTexCanvasMouseEvent(wxMouseEvent& e);
	void         onTexCanvasDragEnd(wxCommandEvent& e);
	void         onTexCanvasKeyDown(wxKeyEvent& e);
	void         onTexNameChanged(wxCommandEvent& e);
	void         onTexWidthChanged(wxCommandEvent& e);
	void         onTexHeightChanged(wxCommandEvent& e);
	virtual void onTexScaleXChanged(wxCommandEvent& e);
	virtual void onTexScaleYChanged(wxCommandEvent& e);
	virtual void onTexWorldPanningChanged(wxCommandEvent& e);
	void         onPatchListSelect(wxListEvent& e);
	void         onPatchListDeSelect(wxListEvent& e);
	void         onPatchPositionXChanged(wxCommandEvent& e);
	void         onPatchPositionYChanged(wxCommandEvent& e);
	void         onApplyScaleChanged(wxCommandEvent& e);
	void         onARCChanged(wxCommandEvent& e);

protected:
	TextureXEditor*      tx_editor_ = nullptr;
	unique_ptr<CTexture> tex_current_;
	bool                 tex_modified_ = false;

	// View controls
	ui::ZoomControl* zc_zoom_         = nullptr;
	wxCheckBox*      cb_draw_outside_ = nullptr;
	wxCheckBox*      cb_blend_rgba_   = nullptr;
	wxCheckBox*      cb_tex_scale_    = nullptr;
	wxCheckBox*      cb_tex_arc_      = nullptr;
	wxStaticText*    label_viewtype_  = nullptr;
	wxChoice*        choice_viewtype_ = nullptr;
	CTextureCanvas*  tex_canvas_      = nullptr;

	// Texture controls
	wxTextCtrl*   text_tex_name_        = nullptr;
	wxSpinCtrl*   spin_tex_width_       = nullptr;
	wxSpinCtrl*   spin_tex_height_      = nullptr;
	wxSpinCtrl*   spin_tex_scalex_      = nullptr;
	wxSpinCtrl*   spin_tex_scaley_      = nullptr;
	wxStaticText* label_scaled_size_    = nullptr;
	wxCheckBox*   cb_tex_world_panning_ = nullptr;

	// Texture patches list + related controls
	ListView* list_patches_ = nullptr;
	SToolBar* tb_patches_   = nullptr;

	// Patch controls
	wxSpinCtrl* spin_patch_left_ = nullptr;
	wxSpinCtrl* spin_patch_top_  = nullptr;

	// Input
	bool alt_press_ = false;
};
} // namespace slade
