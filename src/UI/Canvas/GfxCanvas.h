#pragma once

#include "GLCanvas.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/GLTexture.h"

namespace slade
{
class SImage;
class SBrush;
class GLTexture;
class Translation;
namespace ui
{
	class ZoomControl;
}
namespace gl
{
	class LineBuffer;
}

class GfxCanvas : public GLCanvas
{
public:
	enum class View
	{
		Default,
		Centered,
		Sprite,
		HUD,
		Tiled
	};

	enum class EditMode
	{
		None,
		Paint,
		Erase,
		Translate
	};

	GfxCanvas(wxWindow* parent);
	~GfxCanvas() override = default;

	SImage&        image() { return image_; }
	const Palette& palette() const { return palette_; }

	void    setPalette(const Palette* pal);
	void    setViewType(View type);
	View    viewType() const { return view_type_; }
	void    setScale(double scale);
	bool    allowDrag() const { return allow_drag_; }
	void    allowDrag(bool allow) { allow_drag_ = allow; }
	bool    allowScroll() const { return allow_scroll_; }
	void    allowScroll(bool allow) { allow_scroll_ = allow; }
	void    setPaintColour(const ColRGBA& col) { paint_colour_.set(col); }
	void    setEditingMode(EditMode mode) { editing_mode_ = mode; }
	void    setTranslation(Translation* tr) { translation_ = tr; }
	void    setBrush(SBrush* br) { brush_ = br; }
	SBrush* brush() const { return brush_; }
	ColRGBA paintColour() const { return paint_colour_; }
	void    linkZoomControl(ui::ZoomControl* zoom_control) { linked_zoom_control_ = zoom_control; }

	void draw() override;
	void updateImageTexture();
	void endOffsetDrag();
	void paintPixel(int x, int y);
	void brushCanvas(int x, int y);
	void pickColour(int x, int y);
	void generateBrushShadow();

	void zoomToFit(bool mag = true, double padding = 0.0f);
	void resetViewOffsets();

	bool  onImage(int x, int y) const;
	Vec2i imageCoords(int x, int y) const;

private:
	SImage           image_;
	Palette          palette_;
	View             view_type_      = View::Default;
	unsigned         tex_image_      = 0;
	bool             update_texture_ = false;
	bool             image_hilight_  = false;
	bool             allow_drag_     = false;
	bool             allow_scroll_   = false;
	Vec2i            drag_pos_       = { 0, 0 };
	Vec2i            drag_origin_    = { -1, -1 };
	Vec2i            mouse_prev_;
	EditMode         editing_mode_        = EditMode::None;
	ColRGBA          paint_colour_        = ColRGBA::BLACK; // the colour to apply to pixels in editing mode 1
	Translation*     translation_         = nullptr;        // the translation to apply to pixels in editing mode 3
	bool             drawing_             = false;          // true if a drawing operation is ongoing
	bool*            drawing_mask_        = nullptr; // keeps track of which pixels were already modified in this pass
	SBrush*          brush_               = nullptr; // the brush used to paint the image
	Vec2i            cursor_pos_          = { -1, -1 }; // position of cursor, relative to image
	Vec2i            prev_pos_            = { -1, -1 }; // previous position of cursor
	unsigned         tex_brush_           = 0;          // preview the effect of the brush
	ui::ZoomControl* linked_zoom_control_ = nullptr;
	Vec2i            zoom_point_          = { -1, -1 };

	// OpenGL
	unique_ptr<gl::LineBuffer> lb_sprite_;
	unique_ptr<gl::LineBuffer> lb_hud_;

	void drawImage() const;
	void drawImageTiled() const;
	void drawOffsetLines();

	// Signal connections
	sigslot::scoped_connection sc_image_changed_;

	// Events
	void onMouseLeftDown(wxMouseEvent& e);
	void onMouseRightDown(wxMouseEvent& e);
	void onMouseLeftUp(wxMouseEvent& e);
	void onMouseMovement(wxMouseEvent& e);
	void onMouseLeaving(wxMouseEvent& e);
	void onMouseWheel(wxMouseEvent& e);
	void onKeyDown(wxKeyEvent& e);
};
} // namespace slade

DECLARE_EVENT_TYPE(wxEVT_GFXCANVAS_OFFSET_CHANGED, -1)
DECLARE_EVENT_TYPE(wxEVT_GFXCANVAS_PIXELS_CHANGED, -1)
DECLARE_EVENT_TYPE(wxEVT_GFXCANVAS_COLOUR_PICKED, -1)
