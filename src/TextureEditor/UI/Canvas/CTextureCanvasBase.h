#pragma once

#include "General/Sigslot.h"
#include "Geometry/Rect.h"

namespace slade
{
class CTexture;
class SImage;
} // namespace slade
namespace slade::texeditor
{
class TextureEditor;
}
namespace slade::gl
{
class View;
}
namespace slade::gl::draw2d
{
enum class Align;
}
namespace slade::ui
{
class ZoomControl;
}


namespace slade::texeditor
{
enum class CTextureView
{
	Normal,
	Sprite,
	HUD,
};

class CTextureCanvasBase
{
public:
	using View = CTextureView;

	CTextureCanvasBase(TextureEditor& editor);
	virtual ~CTextureCanvasBase();

	virtual wxWindow* window() = 0;

	virtual gl::View&       view()       = 0;
	virtual const gl::View& view() const = 0;

	virtual Palette* palette()                      = 0;
	virtual void     setPalette(const Palette* pal) = 0;

	CTexture* texture() const { return texture_; }
	View      viewType() const { return view_type_; }
	void      setScale(double scale);
	void      setViewType(View type);
	View      autoDetectViewType() const;
	void      drawOutside(bool draw = true) { draw_outside_ = draw; }
	bool      isDragging() const { return dragging_; }
	bool      showGrid() const { return show_grid_; }
	void      showGrid(bool show = true) { show_grid_ = show; }
	void      setBlendRGBA(bool rgba) { blend_rgba_ = rgba; }
	bool      blendRGBA() const { return blend_rgba_; }
	bool      applyTexScale() const { return tex_scale_; }
	void      applyTexScale(bool apply) { tex_scale_ = apply; }
	Vec2i     dragOrigin() const { return { drag_origin_.x, drag_origin_.y }; }
	Vec2i     dragOffset(bool grid_snap = true) const;

	virtual void clearTexture();
	virtual void clearPatches();
	virtual void refreshPatch(unsigned index);
	virtual void refreshTexturePreview();
	bool         openTexture(CTexture* tex);
	void         resetViewOffsets();
	void         redraw(bool update_tex = false);
	void         setDropPatchOutline(const Rectd& rect);
	void         clearDropPatchOutline();

	int          patchAt(int x, int y) const;
	virtual bool swapPatches(size_t p1, size_t p2);
	Rectd        textureRect(bool scale = false, bool offset = false) const;
	Rectd        patchRect(int index, bool scale = false) const;

	void linkZoomControl(ui::ZoomControl* zoom_control) { linked_zoom_control_ = zoom_control; }

	// Signals
	struct Signals
	{
		sigslot::signal<> view_changed;
		sigslot::signal<> view_reset;
	};
	Signals& signals() { return signals_; }

	// Wx Events (public because we need to call them from outside)
	void onMouseEvent(wxMouseEvent& e);

	static CTextureCanvasBase* createCanvas(wxWindow* parent, TextureEditor& editor);

protected:
	CTexture*          texture_ = nullptr;
	unique_ptr<SImage> tex_preview_;
	TextureEditor*     editor_ = nullptr;

	vector<unique_ptr<SImage>> patch_images_;
	int                        hilight_patch_ = -1;

	ui::ZoomControl* linked_zoom_control_ = nullptr;
	Vec2i            zoom_point_          = { -1, -1 };

	Vec2i drag_origin_ = { -1, -1 };
	Vec2i mouse_pos_   = { -1, -1 };
	bool  dragging_    = false;

	bool  show_grid_ = false;
	Vec2i grid_size_ = { 8, 8 };

	bool  draw_outside_ = true;
	bool  blend_rgba_   = false;
	bool  tex_scale_    = false;
	View  view_type_    = View::Normal;
	Rectd drop_patch_outline_;
	bool  show_drop_patch_outline_ = false;

	struct Text
	{
		string            text;
		Vec2d             position;
		gl::draw2d::Align alignment;
		bool              above = false;
	};
	vector<Text> texts_;

	// Signal connections
	Signals              signals_;
	ScopedConnectionList connections_;

	void         drawContent();
	virtual void initDrawing(const Rectd& tex_rect) {}
	virtual void drawOffsetLines() {}
	virtual void drawTexture(const Rectd& tex_rect) {}
	virtual void drawTextureBorder(const Rectd& tex_rect) {}
	virtual void drawTextureGrid(const Rectd& tex_rect) {}
	virtual void drawPatch(const Rectd& patch_rect, int index, float alpha, bool highlight) {}
	virtual void drawPatchOutline(const Rectd& patch_rect, const ColRGBA& colour, double line_width) {}
	virtual void drawTextOverlays() {}

	void         loadPatchImage(unsigned index);
	virtual void loadTexturePreview();
};
} // namespace slade::texeditor

wxDECLARE_EVENT(EVT_DRAG_END, wxCommandEvent);
