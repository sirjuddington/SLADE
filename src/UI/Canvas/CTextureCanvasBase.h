#pragma once

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
class CTexture;
class SImage;

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

	CTextureCanvasBase() = default;
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

	virtual void clearTexture();
	virtual void clearPatches();
	virtual void refreshPatch(unsigned index);
	void         refreshTexturePreview();
	bool         openTexture(CTexture* tex, Archive* parent);
	void         resetViewOffsets();
	void         redraw(bool update_tex = false);

	int          patchAt(int x, int y) const;
	virtual bool swapPatches(size_t p1, size_t p2);

	void linkZoomControl(ui::ZoomControl* zoom_control) { linked_zoom_control_ = zoom_control; }

protected:
	struct Patch
	{
		unique_ptr<SImage> image;
		bool               selected;
	};

	CTexture*          texture_ = nullptr;
	Archive*           parent_  = nullptr;
	unique_ptr<SImage> tex_preview_;

	vector<Patch> patches_;
	int           hilight_patch_ = -1;

	ui::ZoomControl* linked_zoom_control_ = nullptr;
	Vec2i            zoom_point_          = { -1, -1 };

	Vec2i mouse_prev_;
	bool  draw_outside_ = true;
	bool  dragging_     = false;
	bool  show_grid_    = false;
	bool  blend_rgba_   = false;
	bool  tex_scale_    = false;
	View  view_type_    = View::Normal;

	// Signal connections
	sigslot::scoped_connection sc_patches_modified_;

	void loadPatchImage(unsigned index);
	void loadTexturePreview();

	// Wx Events
	void onMouseEvent(wxMouseEvent& e);
};
} // namespace slade

wxDECLARE_EVENT(EVT_DRAG_END, wxCommandEvent);
