#pragma once

#include "GLCanvas.h"
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
	namespace draw2d
	{
		struct Context;
	}
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
	void draw() override;

private:
	unsigned                      gl_tex_preview_ = 0;
	vector<unsigned>              patch_gl_textures_;
	unique_ptr<gl::LineBuffer>    lb_sprite_;
	unique_ptr<gl::LineBuffer>    lb_border_;
	unique_ptr<gl::LineBuffer>    lb_grid_;
	unique_ptr<gl::LineBuffer>    lb_square_;
	static unique_ptr<gl::Shader> shader_;

	// Private functions
	void drawOffsetLines(const gl::draw2d::Context& dc);
	void drawTexture(gl::draw2d::Context& dc, glm::vec2 scale, glm::vec2 offset, bool draw_patches);
	void drawPatch(int num);
	void drawPatchOutline(const gl::draw2d::Context& dc, int num) const;
	void drawTextureBorder(glm::vec2 scale, glm::vec2 offset);
	void initShader() const;
};
} // namespace slade
