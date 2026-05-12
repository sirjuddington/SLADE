#pragma once

#include "Geometry/RectFwd.h"

// Forward declarations
namespace slade
{
namespace gl
{
	class View;
}
namespace ui
{
	class ZoomControl;
}
class SBrush;
class Translation;
class SImage;
} // namespace slade


namespace slade
{
enum class GfxView
{
	Default,
	Centered,
	Sprite,
	HUD,
	Tiled
};

enum class GfxEditMode
{
	None,
	Paint,
	Erase,
	Translate
};

class GfxCanvasBase
{
public:
	using View     = GfxView;
	using EditMode = GfxEditMode;

	GfxCanvasBase();
	virtual ~GfxCanvasBase();

	SImage& image() const { return *image_; }

	virtual wxWindow* window() = 0;

	virtual gl::View&       view()       = 0;
	virtual const gl::View& view() const = 0;

	virtual Palette* palette()                      = 0;
	virtual void     setPalette(const Palette* pal) = 0;

	void    setScale(double scale);
	void    setViewType(View type);
	View    viewType() const { return view_type_; }
	bool    allowDrag() const { return allow_drag_; }
	void    allowDrag(bool allow) { allow_drag_ = allow; }
	bool    allowScroll() const { return allow_scroll_; }
	void    allowScroll(bool allow) { allow_scroll_ = allow; }
	bool    showBorder() const { return show_border_; }
	void    showBorder(bool show) { show_border_ = show; }
	bool    showHilight() const { return show_hilight_; }
	void    showHilight(bool show) { show_hilight_ = show; }
	void    setPaintColour(const ColRGBA& col) { paint_colour_.set(col); }
	void    setEditingMode(EditMode mode) { editing_mode_ = mode; }
	void    setTranslation(Translation* tr) { translation_ = tr; }
	void    setBrush(SBrush* br) { brush_ = br; }
	SBrush* brush() const { return brush_; }
	ColRGBA paintColour() const { return paint_colour_; }
	void    linkZoomControl(ui::ZoomControl* zoom_control) { linked_zoom_control_ = zoom_control; }

	void zoomToFit(bool mag = true, double padding = 0.0f);
	void resetViewOffsets();

	bool  onImage(int x, int y) const;
	Vec2i imageCoords(int x, int y) const;

	void setCropRect(const Recti& rect);
	void clearCropRect();

	// Signals
	struct Signals
	{
		sigslot::signal<> view_changed;
		sigslot::signal<> view_reset;
	};
	Signals& signals() { return signals_; }

protected:
	unique_ptr<SImage> image_;
	View               view_type_    = View::Default;
	bool               image_hover_  = false; // True if the cursor is currently on the image
	bool               allow_drag_   = false;
	bool               allow_scroll_ = false;
	bool               show_border_  = false;
	bool               show_hilight_ = false;
	Vec2i              drag_pos_     = { 0, 0 };
	Vec2i              drag_origin_  = { -1, -1 };
	Vec2i              mouse_prev_;
	EditMode           editing_mode_        = EditMode::None;
	ColRGBA            paint_colour_        = ColRGBA::BLACK; // The colour to apply to pixels in editing mode 1
	Translation*       translation_         = nullptr;        // The translation to apply to pixels in editing mode 3
	bool               drawing_             = false;          // True if a drawing operation is ongoing
	bool*              drawing_mask_        = nullptr; // Keeps track of which pixels were already modified in this pass
	SBrush*            brush_               = nullptr; // The brush used to paint the image
	Vec2i              cursor_pos_          = { -1, -1 }; // Position of cursor, relative to image
	Vec2i              prev_pos_            = { -1, -1 }; // Previous position of cursor
	ui::ZoomControl*   linked_zoom_control_ = nullptr;
	Vec2i              zoom_point_          = { -1, -1 };
	unique_ptr<Recti>  crop_rect_;

	// Signals/connections
	Signals                    signals_;
	sigslot::scoped_connection sc_image_changed_;

	void endOffsetDrag();
	void paintPixel(int x, int y);
	void brushCanvas(int x, int y);
	void pickColour(int x, int y);

	virtual void generateBrushShadow() {}
	void         generateBrushShadowImage(SImage& img);

	// Wx event handlers
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
