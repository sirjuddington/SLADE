#pragma once

#include "GLCanvas.h"
#include "OpenGL/Draw2D.h"
#include "UI/Canvas/CTextureCanvasBase.h"

namespace slade
{
// Forward declarations
namespace ui
{
	class ZoomControl;
}
namespace gl
{
	class LineBuffer;
	class Shader;
} // namespace gl

class CTextureGLCanvas : public GLCanvas, public CTextureCanvasBase
{
public:
	CTextureGLCanvas(wxWindow* parent);
	~CTextureGLCanvas() override;

	wxWindow* window() override { return this; }

	// ReSharper disable CppHidingFunction
	gl::View&       view() override { return view_; }
	const gl::View& view() const override { return view_; }

	Palette* palette() override { return palette_.get(); }
	void     setPalette(const Palette* pal) override { GLCanvas::setPalette(pal); }

	void clearTexture() override;
	void clearPatches() override;
	void refreshPatch(unsigned index) override;

protected:
	// CTextureCanvasBase drawing overrides
	void initDrawing(const Rectd& tex_rect) override;
	void drawOffsetLines() override;
	void drawTexture(const Rectd& tex_rect) override;
	void drawTextureBorder(const Rectd& tex_rect) override;
	void drawTextureGrid(const Rectd& tex_rect) override;
	void drawPatch(const Rectd& patch_rect, int index, float alpha, bool highlight) override;
	void drawPatchOutline(const Rectd& patch_rect, const ColRGBA& colour, double line_width) override;
	void drawTextOverlays() override;
	void loadTexturePreview() override;

private:
	unsigned                      gl_tex_preview_ = 0;
	vector<unsigned>              patch_gl_textures_;
	unique_ptr<gl::LineBuffer>    lb_sprite_;
	unique_ptr<gl::LineBuffer>    lb_square_;
	static unique_ptr<gl::Shader> shader_;
	gl::draw2d::Context           dc_;

	void draw() override;
	void initShader() const;
};
} // namespace slade
