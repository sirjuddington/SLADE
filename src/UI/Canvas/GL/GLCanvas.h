#pragma once

#include "OpenGL/OpenGL.h"
#include "OpenGL/View.h"
#include "Utility/Colour.h"

namespace slade
{
class Palette;
namespace gl
{
	class VertexBuffer2D;
}

class GLCanvas : public wxGLCanvas
{
public:
	enum class BGStyle
	{
		Colour,
		Checkered,
		System
	};

	GLCanvas(
		wxWindow*       parent,
		BGStyle         bg_style  = BGStyle::Colour,
		const ColRGBA&  bg_colour = ColRGBA::BLACK,
		const gl::View& view      = {});
	~GLCanvas() override;

	const gl::View& view() const { return view_; }
	gl::View&       view() { return view_; }
	const Palette*  palette() const { return palette_.get(); }
	ColRGBA         backgroundColour() const { return bg_colour_; }

	void         setView(const gl::View& view) { view_ = view; }
	virtual void setPalette(const Palette* pal);
	void         setBackground(BGStyle style, ColRGBA colour)
	{
		bg_colour_ = colour;
		bg_style_  = style;
	}

	void setupMousewheelZoom();
	void setupMousePanning();

	bool activateContext();

protected:
	gl::View            view_;
	unique_ptr<Palette> palette_;

private:
	BGStyle                        bg_style_ = BGStyle::Colour;
	ColRGBA                        bg_colour_;
	unique_ptr<gl::VertexBuffer2D> vb_background_;
	bool                           init_done_ = false;

	// View panning
	Vec2i mouse_prev_;

	void         init();
	virtual void draw();
	void         updateBackgroundVB();
	void         drawCheckeredBackground();

	// Events
	void onPaint(wxPaintEvent& e);
};
} // namespace slade
