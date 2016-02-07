
#ifndef __CTEXTURECANVAS_H__
#define __CTEXTURECANVAS_H__

#include "OGLCanvas.h"
#include "CTexture.h"
#include "Palette.h"
#include "GLTexture.h"
#include "ListenerAnnouncer.h"
#include "PatchTable.h"

wxDECLARE_EVENT(EVT_DRAG_END, wxCommandEvent);

class CTextureCanvas : public OGLCanvas, Listener
{
private:
	CTexture*			texture;
	Archive*			parent;
	vector<GLTexture*>	patch_textures;
	GLTexture			tex_preview;
	vector<bool>		selected_patches;
	int					hilight_patch;
	fpoint2_t			offset;
	point2_t			mouse_prev;
	double				scale;
	bool				draw_outside;
	bool				dragging;
	bool				show_grid;
	bool				blend_rgba;
	bool				tex_scale;
	int					view_type;	// 0=normal, 1=sprite offsets, 2=hud offsets

public:
	CTextureCanvas(wxWindow* parent, int id);
	~CTextureCanvas();

	CTexture*	getTexture() { return texture; }
	int			getViewType() { return view_type; }
	void		setScale(double scale) { this->scale = scale; }
	void		setViewType(int type) { this->view_type = type; }
	void		drawOutside(bool draw = true) { draw_outside = draw; }
	point2_t	getMousePrevPos() { return mouse_prev; }
	bool		isDragging() { return dragging; }
	bool		showGrid() { return show_grid; }
	void		showGrid(bool show = true) { show_grid = show; }
	void		blendRGBA(bool rgba) { blend_rgba = rgba; }
	bool		getBlendRGBA() { return blend_rgba; }
	bool		applyTexScale() { return tex_scale; }
	void		applyTexScale(bool apply) { tex_scale = apply; }

	void	selectPatch(int index);
	void	deSelectPatch(int index);
	bool	patchSelected(int index);

	void	clearTexture();
	void	clearPatchTextures();
	void	updatePatchTextures();
	void	updateTexturePreview();
	bool	openTexture(CTexture* tex, Archive* parent);
	void	draw();
	void	drawTexture();
	void	drawPatch(int num, bool outside = false);
	void	drawTextureBorder();
	void	drawOffsetLines();
	void	resetOffsets() { offset.x = offset.y = 0; }
	void	redraw(bool update_tex = false);

	point2_t	screenToTexPosition(int x, int y);
	point2_t	texToScreenPosition(int x, int y);
	int			patchAt(int x, int y);

	bool	swapPatches(size_t p1, size_t p2);

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	// Events
	void	onMouseEvent(wxMouseEvent& e);
};

#endif//__CTEXTURECANVAS_H__
