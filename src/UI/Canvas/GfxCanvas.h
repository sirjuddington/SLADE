
#ifndef __GFXCANVAS_H__
#define	__GFXCANVAS_H__

#include "OGLCanvas.h"
#include "General/ListenerAnnouncer.h"

// Enumeration for view types
enum
{
	GFXVIEW_DEFAULT,
	GFXVIEW_CENTERED,
	GFXVIEW_SPRITE,
	GFXVIEW_HUD,
	GFXVIEW_TILED,
};

class SImage;
class GLTexture;
class GfxCanvas : public OGLCanvas, Listener
{
private:
	SImage*		image;
	int			view_type;	// 0=default, 1=centered, 2=sprite offsets, 3=hud offsets, 4=tiled
	double		scale;
	fpoint2_t	offset;		// panning offsets (not image offsets)
	GLTexture*	tex_image;
	bool		update_texture;
	bool		image_hilight;
	bool		image_split;
	bool		allow_drag;
	bool		allow_scroll;
	point2_t	drag_pos;
	point2_t	drag_origin;
	point2_t	mouse_prev;

public:
	GfxCanvas(wxWindow* parent, int id);
	~GfxCanvas();

	SImage*	getImage() { return image; }

	void	setViewType(int type) { view_type = type; }
	int		getViewType() { return view_type; }
	void	setScale(double scale) { this->scale = scale; }
	bool	allowDrag() { return allow_drag; }
	void	allowDrag(bool allow) { allow_drag = allow; }
	bool	allowScroll() { return allow_scroll; }
	void	allowScroll(bool allow) { allow_scroll = allow; }

	void	draw();
	void	drawImage();
	void	drawOffsetLines();
	void	updateImageTexture();
	void	endOffsetDrag();

	void	zoomToFit(bool mag = true, float padding = 0.0f);
	void	resetOffsets() { offset.x = offset.y = 0; }

	bool		onImage(int x, int y);
	point2_t	imageCoords(int x, int y);

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	// Events
	void	onMouseLeftDown(wxMouseEvent& e);
	void	onMouseLeftUp(wxMouseEvent& e);
	void	onMouseMovement(wxMouseEvent& e);
	void	onMouseLeaving(wxMouseEvent& e);
	void	onKeyDown(wxKeyEvent& e);
};

DECLARE_EVENT_TYPE(wxEVT_GFXCANVAS_OFFSET_CHANGED, -1)

#endif //__GFXCANVAS_H__
