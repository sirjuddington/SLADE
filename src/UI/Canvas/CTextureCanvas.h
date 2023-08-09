#pragma once

#include "GLCanvas.h"

wxDECLARE_EVENT(EVT_DRAG_END, wxCommandEvent);

namespace slade
{
// Forward declarations
class CTexture;
class Archive;
namespace ui
{
	class ZoomControl;
}
namespace gl
{
	class LineBuffer;
	class Shader;
	namespace draw2d
	{
		struct Context;
	}
} // namespace gl

class CTextureCanvas : public GLCanvas
{
public:
	enum class View
	{
		Normal,
		Sprite,
		HUD,
	};

	CTextureCanvas(wxWindow* parent);
	~CTextureCanvas() override;

	CTexture* texture() const { return texture_; }
	View      viewType() const { return view_type_; }
	void      setScale(double scale);
	void      setViewType(View type);
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
	bool patchSelected(int index) const;

	void clearTexture();
	void clearPatches();
	void updatePatchTextures();
	void updateTexturePreview();
	bool openTexture(CTexture* tex, Archive* parent);
	void draw() override;
	void resetViewOffsets();
	void redraw(bool update_tex = false);

	Vec2i screenToTexPosition(int x, int y) const;
	Vec2i texToScreenPosition(int x, int y) const;
	int   patchAt(int x, int y) const;

	bool swapPatches(size_t p1, size_t p2);

	void linkZoomControl(ui::ZoomControl* zoom_control) { linked_zoom_control_ = zoom_control; }

private:
	struct Patch
	{
		unsigned texture;
		bool     selected;
		Rectf    rect;
	};

	CTexture*        texture_ = nullptr;
	Archive*         parent_  = nullptr;
	vector<Patch>    patches_;
	unsigned         tex_preview_   = 0;
	int              hilight_patch_ = -1;
	Vec2i            mouse_prev_;
	bool             draw_outside_        = true;
	bool             dragging_            = false;
	bool             show_grid_           = false;
	bool             blend_rgba_          = false;
	bool             tex_scale_           = false;
	View             view_type_           = View::Normal;
	ui::ZoomControl* linked_zoom_control_ = nullptr;
	Vec2i            zoom_point_          = { -1, -1 };

	// OpenGL
	unique_ptr<gl::LineBuffer>    lb_sprite_;
	unique_ptr<gl::LineBuffer>    lb_border_;
	unique_ptr<gl::LineBuffer>    lb_grid_;
	unique_ptr<gl::LineBuffer>    lb_square_;
	static unique_ptr<gl::Shader> shader_;

	// Signal connections
	sigslot::scoped_connection sc_patches_modified_;

	// Private functions
	void drawOffsetLines(const gl::draw2d::Context& dc);
	void drawTexture(gl::draw2d::Context& dc, glm::vec2 scale, glm::vec2 offset, bool draw_patches);
	void drawPatch(int num, bool outside = false);
	void drawPatchOutline(const gl::draw2d::Context& dc, int num) const;
	void drawTextureBorder(glm::vec2 scale, glm::vec2 offset);
	void initShader() const;

	// Events
	void onMouseEvent(wxMouseEvent& e);
};
} // namespace slade
