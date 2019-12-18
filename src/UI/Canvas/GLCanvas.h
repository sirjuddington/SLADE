#pragma once

#include "OpenGL/OpenGL.h"
#include "Utility/Colour.h"

namespace OpenGL
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

	GLCanvas(wxWindow* parent, BGStyle bg_style = BGStyle::Colour, const ColRGBA& bg_colour = ColRGBA::BLACK);
	virtual ~GLCanvas();

	const glm::mat4& orthoProjectionMatrix() const { return projection_ortho_; }

	bool activateContext();

private:
	BGStyle                            bg_style_ = BGStyle::Colour;
	ColRGBA                            bg_colour_;
	glm::mat4                          projection_ortho_;
	unique_ptr<OpenGL::VertexBuffer2D> vb_background_;
	bool                               init_done_ = false;

	void         init();
	virtual void draw();
	void         updateBackgroundVB();
	void         drawCheckeredBackground();

	// Events
	void onPaint(wxPaintEvent& e);
};
