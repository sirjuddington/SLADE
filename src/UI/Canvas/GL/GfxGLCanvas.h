#pragma once

#include "GLCanvas.h"
#include "UI/Canvas/GfxCanvasBase.h"

namespace slade
{
// Forward declarations
class GLTexture;
namespace gl
{
	class LineBuffer;
	namespace draw2d
	{
		struct Context;
	}
} // namespace gl

class GfxGLCanvas : public GLCanvas, public GfxCanvasBase
{
public:
	using View     = GfxView;
	using EditMode = GfxEditMode;

	GfxGLCanvas(wxWindow* parent);
	~GfxGLCanvas() override = default;

	wxWindow* window() override { return this; }

	// ReSharper disable CppHidingFunction
	gl::View&       view() override { return view_; }
	const gl::View& view() const override { return view_; }

	Palette* palette() override { return palette_.get(); }
	void     setPalette(const Palette* pal) override;

	void draw() override;
	void updateImageTexture();
	void generateBrushShadow() override;

private:
	unsigned tex_image_      = 0;
	bool     update_texture_ = false;
	unsigned tex_brush_      = 0; // preview the effect of the brush

	// OpenGL
	unique_ptr<gl::LineBuffer> lb_sprite_;

	void drawImage(gl::draw2d::Context& dc) const;
	void drawImageTiled() const;
	void drawOffsetLines(const gl::draw2d::Context& dc);
	void drawCropRect(gl::draw2d::Context& dc) const;
};
} // namespace slade
