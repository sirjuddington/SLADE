#pragma once

#include "General/ListenerAnnouncer.h"
#include "OGLCanvas.h"

class SImage;
class SBrush;
class GLTexture;
class Translation;

class GfxCanvas : public OGLCanvas, Listener
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

	GfxCanvas(wxWindow* parent, int id);
	~GfxCanvas();

	SImage* getImage() { return image_; }

	void    setViewType(View type) { view_type_ = type; }
	View    viewType() { return view_type_; }
	void    setScale(double scale);
	bool    allowDrag() { return allow_drag_; }
	void    allowDrag(bool allow) { allow_drag_ = allow; }
	bool    allowScroll() { return allow_scroll_; }
	void    allowScroll(bool allow) { allow_scroll_ = allow; }
	void    setPaintColour(const ColRGBA& col) { paint_colour_.set(col); }
	void    setEditingMode(EditMode mode) { editing_mode_ = mode; }
	void    setTranslation(Translation* tr) { translation_ = tr; }
	void    setBrush(SBrush* br) { brush_ = br; }
	SBrush* brush() { return brush_; }
	ColRGBA paintColour() { return paint_colour_; }

	void draw();
	void drawImage();
	void drawOffsetLines();
	void updateImageTexture();
	void endOffsetDrag();
	void paintPixel(int x, int y);
	void brushCanvas(int x, int y);
	void pickColour(int x, int y);
	void generateBrushShadow();

	void zoomToFit(bool mag = true, float padding = 0.0f);
	void resetOffsets() { offset_.x = offset_.y = 0; }

	bool  onImage(int x, int y);
	Vec2i imageCoords(int x, int y);

	void onAnnouncement(Announcer* announcer, const string& event_name, MemChunk& event_data) override;

private:
	SImage*      image_;
	View         view_type_;
	double       scale_;
	Vec2f        offset_; // panning offsets (not image offsets)
	GLTexture*   tex_image_;
	bool         update_texture_;
	bool         image_hilight_;
	bool         image_split_;
	bool         allow_drag_;
	bool         allow_scroll_;
	Vec2i        drag_pos_;
	Vec2i        drag_origin_;
	Vec2i        mouse_prev_;
	EditMode     editing_mode_;
	ColRGBA      paint_colour_; // the colour to apply to pixels in editing mode 1
	Translation* translation_;  // the translation to apply to pixels in editing mode 3
	bool         drawing_;      // true if a drawing operation is ongoing
	bool*        drawing_mask_; // keeps track of which pixels were already modified in this pass
	SBrush*      brush_;        // the brush used to paint the image
	Vec2i        cursor_pos_;   // position of cursor, relative to image
	Vec2i        prev_pos_;     // previous position of cursor
	GLTexture*   tex_brush_;    // preview the effect of the brush

	// Events
	void onMouseLeftDown(wxMouseEvent& e);
	void onMouseRightDown(wxMouseEvent& e);
	void onMouseLeftUp(wxMouseEvent& e);
	void onMouseMovement(wxMouseEvent& e);
	void onMouseLeaving(wxMouseEvent& e);
	void onKeyDown(wxKeyEvent& e);
};

DECLARE_EVENT_TYPE(wxEVT_GFXCANVAS_OFFSET_CHANGED, -1)
DECLARE_EVENT_TYPE(wxEVT_GFXCANVAS_PIXELS_CHANGED, -1)
DECLARE_EVENT_TYPE(wxEVT_GFXCANVAS_COLOUR_PICKED, -1)
