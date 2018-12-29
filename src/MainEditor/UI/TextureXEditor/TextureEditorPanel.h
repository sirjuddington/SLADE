#pragma once

#include "General/SAction.h"
#include "UI/Lists/ListView.h"

class TextureXEditor;
class TextureXList;
class CTextureCanvas;
class CTexture;
class Palette;
class SZoomSlider;

class TextureEditorPanel : public wxPanel, SActionHandler
{
public:
	TextureEditorPanel(wxWindow* parent, TextureXEditor* tx_editor);
	virtual ~TextureEditorPanel() = default;

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

	bool openTexture(CTexture* tex, TextureXList* list);
	void clearTexture();
	void setPalette(Palette* pal) const;

	Palette* palette() const;
	bool     blendRGBA() const;

	// Editing
	virtual void addPatch();
	void         removePatch();
	void         patchBack();
	void         patchForward();
	virtual void replacePatch();
	void         duplicatePatch(int xoff = 8, int yoff = 8);

	// SAction handler
	bool handleAction(const string& id) override;

	// Events
	void onDrawOutsideChanged(wxCommandEvent& e);
	void onTexCanvasMouseEvent(wxMouseEvent& e);
	void onTexCanvasDragEnd(wxCommandEvent& e);
	void onTexCanvasKeyDown(wxKeyEvent& e);
	void onTexNameChanged(wxCommandEvent& e);
	void onTexWidthChanged(wxCommandEvent& e);
	void onTexHeightChanged(wxCommandEvent& e);
	void onTexScaleXChanged(wxCommandEvent& e);
	void onTexScaleYChanged(wxCommandEvent& e);
	void onTexWorldPanningChanged(wxCommandEvent& e);
	void onPatchListSelect(wxListEvent& e);
	void onPatchListDeSelect(wxListEvent& e);
	void onPatchPositionXChanged(wxCommandEvent& e);
	void onPatchPositionYChanged(wxCommandEvent& e);
	void onContextMenu(wxCommandEvent& e);
	void onApplyScaleChanged(wxCommandEvent& e);
	void onARCChanged(wxCommandEvent& e);

protected:
	TextureXEditor*           tx_editor_ = nullptr;
	std::unique_ptr<CTexture> tex_current_;
	bool                      tex_modified_ = false;

	// View controls
	SZoomSlider*    slider_zoom_     = nullptr;
	wxCheckBox*     cb_draw_outside_ = nullptr;
	wxCheckBox*     cb_blend_rgba_   = nullptr;
	wxCheckBox*     cb_tex_scale_    = nullptr;
	wxCheckBox*     cb_tex_arc_      = nullptr;
	wxStaticText*   label_viewtype_  = nullptr;
	wxChoice*       choice_viewtype_ = nullptr;
	CTextureCanvas* tex_canvas_      = nullptr;

	// Texture controls
	wxTextCtrl*   text_tex_name_        = nullptr;
	wxSpinCtrl*   spin_tex_width_       = nullptr;
	wxSpinCtrl*   spin_tex_height_      = nullptr;
	wxSpinCtrl*   spin_tex_scalex_      = nullptr;
	wxSpinCtrl*   spin_tex_scaley_      = nullptr;
	wxStaticText* label_scaled_size_    = nullptr;
	wxCheckBox*   cb_tex_world_panning_ = nullptr;

	// Texture patches list + related controls
	ListView* list_patches_        = nullptr;
	wxButton* btn_patch_add_       = nullptr;
	wxButton* btn_patch_remove_    = nullptr;
	wxButton* btn_patch_back_      = nullptr;
	wxButton* btn_patch_forward_   = nullptr;
	wxButton* btn_patch_replace_   = nullptr;
	wxButton* btn_patch_duplicate_ = nullptr;

	// Patch controls
	wxSpinCtrl* spin_patch_left_ = nullptr;
	wxSpinCtrl* spin_patch_top_  = nullptr;

	// Input
	bool alt_press_ = false;
};
