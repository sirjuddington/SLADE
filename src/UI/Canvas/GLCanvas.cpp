
#include "Main.h"
#include "GLCanvas.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "Utility/Colour.h"
#include <SFML/Graphics/RenderTarget.hpp>


GLCanvas::GLCanvas(wxWindow* parent, int id) :
	wxGLCanvas{ parent, id, OpenGL::getWxGLAttribs(), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS }
{
	Bind(wxEVT_PAINT, [&](wxPaintEvent&) {
		wxPaintDC dc(this);
		render();
	});
	Bind(wxEVT_ERASE_BACKGROUND, [&](wxEraseEvent&) {});
	Bind(wxEVT_SIZE, [&](wxSizeEvent&) { render(); });
}

sf::Vector2u GLCanvas::getSize() const
{
	return { (unsigned)GetSize().x, (unsigned)GetSize().y };
}

bool GLCanvas::activateContext()
{
	if (const auto context = OpenGL::getContext(this))
	{
		context->SetCurrent(*this);
		Drawing::setRenderTarget(this);
		return true;
	}

	return false;
}

void GLCanvas::setup2D()
{
	int width  = GetSize().x;
	int height = GetSize().y;

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0., width, height, 0., -1., 1.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);
}

void GLCanvas::clear(ColRGBA colour)
{
	glClearColor(colour.fr(), colour.fg(), colour.fb(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLCanvas::reset()
{
	OpenGL::resetBlend();

	// Init GL settings
	glClearDepth(1.0);
	glShadeModel(GL_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_NONE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glEnable(GL_ALPHA_TEST);
}

void GLCanvas::drawCheckeredBackground() const
{
	// Save current matrix
	glPushMatrix();

	// Enable textures
	glEnable(GL_TEXTURE_2D);

	// Bind background texture
	OpenGL::Texture::bind(OpenGL::Texture::backgroundTexture());

	// Draw background
	Rectf rect(0, 0, GetSize().x, GetSize().y);
	OpenGL::setColour(ColRGBA::WHITE);
	glBegin(GL_QUADS);
	glTexCoord2d(rect.x1() * 0.0625, rect.y1() * 0.0625);
	glVertex2d(rect.x1(), rect.y1());
	glTexCoord2d(rect.x1() * 0.0625, rect.y2() * 0.0625);
	glVertex2d(rect.x1(), rect.y2());
	glTexCoord2d(rect.x2() * 0.0625, rect.y2() * 0.0625);
	glVertex2d(rect.x2(), rect.y2());
	glTexCoord2d(rect.x2() * 0.0625, rect.y1() * 0.0625);
	glVertex2d(rect.x2(), rect.y1());
	glEnd();

	// Disable textures
	glDisable(GL_TEXTURE_2D);

	// Restore previous matrix
	glPopMatrix();
}

void GLCanvas::beginSFMLDraw()
{
	if (!sfml_draw_)
	{
		// Push related states
		glPushMatrix();
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glPushAttrib(GL_VIEWPORT_BIT);
		resetGLStates();
	}
	sfml_draw_ = true;
}

void GLCanvas::endSFMLDraw()
{
	if (sfml_draw_)
	{
		// Pop related states
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	sfml_draw_ = false;
}

void GLCanvas::drawContent()
{
	auto width  = GetSize().x;
	auto height = GetSize().y;

	setup2D();
	clear();

	OpenGL::setColour(ColRGBA::WHITE);

	glLineWidth(2.f);
	glEnable(GL_LINE_SMOOTH);
	glDisable(GL_TEXTURE_2D);

	glBegin(GL_LINES);
	glVertex2i(0, 0);
	glVertex2i(width, 0);
	glVertex2i(width, 0);
	glVertex2i(width, height);
	glVertex2i(width, height);
	glVertex2i(0, height);
	glVertex2i(0, height);
	glVertex2i(0, 0);
	glVertex2i(0, 0);
	glVertex2i(width, height);
	glVertex2i(width, 0);
	glVertex2i(0, height);
	glEnd();


	// Draw an SFML circle
	beginSFMLDraw();

	sf::CircleShape shape{ 100 };
	shape.setPosition((float)width * 0.5f - shape.getRadius(), (float)height * 0.5f - shape.getRadius());
	shape.setFillColor(sf::Color::Red);
	shape.setOutlineColor(sf::Color::Green);
	shape.setOutlineThickness(4.f);
	draw(shape);

	endSFMLDraw();
}

void GLCanvas::render()
{
	auto width  = GetSize().x;
	auto height = GetSize().y;

	if (!activateContext())
		return;

	// Set viewport to canvas size
	setView(sf::View{ { 0.f, 0.f, (float)width, (float)height } });
	glViewport(0, 0, width, height);

	// Draw content
	reset();
	drawContent();
	endSFMLDraw(); // Ensure all matrices etc are popped

	// Display content
	SwapBuffers();
}

bool GLCanvas::setActive(bool active)
{
	return activateContext();
}




// Testing
#include "General/Console/Console.h"
#include "UI/STopWindow.h"

CONSOLE_COMMAND(tgc, 0, false)
{
	wxDialog dlg(
		STopWindow::currentActive(),
		-1,
		"GLCanvas Test",
		wxDefaultPosition,
		{ 800, 800 },
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

	dlg.SetSizer(new wxBoxSizer(wxVERTICAL));
	dlg.GetSizer()->Add(new GLCanvas{ &dlg }, 1, wxEXPAND | wxALL, 10);
	dlg.CenterOnParent();
	dlg.ShowModal();
}
