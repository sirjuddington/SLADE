#pragma once

#include "OGLCanvas.h"

wxDECLARE_EVENT(EVT_DRAG_END, wxCommandEvent);

namespace slade
{
class CTexture;
class Archive;

class CTextureCanvas : public OGLCanvas
{
public:
	enum class View
	{
		Normal,
		Sprite,
		HUD
	};

	CTextureCanvas(wxWindow* parent, int id);
	~CTextureCanvas() = default;

	CTexture* texture() const { return texture_; }
	View      viewType() const { return view_type_; }
	void      setScale(double scale) { scale_ = scale; }
	void      setViewType(View type) { view_type_ = type; }
	void      drawOutside(bool draw = true) { draw_outside_ = draw; }
	Vec2i     mousePrevPos() const { return mouse_prev_; }
	bool      isDragging() const { return dragging_; }
	bool      showGrid() const { return show_grid_; }
	void      showGrid(bool show = true) { show_grid_ = show; }
	void      setBlendRGBA(bool rgba) { blend_rgba_ = rgba; }
	bool      blendRGBA() const { return blend_rgba_; }
	bool      applyTexScale() const { return tex_scale_; }
	void      applyTexScale(bool apply) { tex_scale_ = apply; }

	void selectPatch(int index);
	void deSelectPatch(int index);
	bool patchSelected(int index);

	void clearTexture();
	void clearPatchTextures();
	void updatePatchTextures();
	void updateTexturePreview();
	bool openTexture(CTexture* tex, Archive* parent);
	void draw() override;
	void drawTexture();
	void drawPatch(int num, bool outside = false);
	void drawTextureBorder() const;
	void drawOffsetLines() const;
	void resetOffsets() { offset_.x = offset_.y = 0; }
	void redraw(bool update_tex = false);

	Vec2i screenToTexPosition(int x, int y) const;
	Vec2i texToScreenPosition(int x, int y) const;
	int   patchAt(int x, int y);

	bool swapPatches(size_t p1, size_t p2);

private:
	CTexture*        texture_ = nullptr;
	Archive*         parent_  = nullptr;
	vector<unsigned> patch_textures_;
	unsigned         tex_preview_ = 0;
	vector<bool>     selected_patches_;
	int              hilight_patch_ = -1;
	Vec2d            offset_;
	Vec2i            mouse_prev_;
	double           scale_        = 1.;
	bool             draw_outside_ = true;
	bool             dragging_     = false;
	bool             show_grid_    = false;
	bool             blend_rgba_   = false;
	bool             tex_scale_    = false;
	View             view_type_    = View::Normal;

	// Signal connections
	sigslot::scoped_connection sc_patches_modified_;

	// Events
	void onMouseEvent(wxMouseEvent& e);
};
} // namespace slade
