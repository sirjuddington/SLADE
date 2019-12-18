
#include "Main.h"
#include "GLCanvas.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include <glm/gtc/matrix_transform.hpp>


GLCanvas::GLCanvas(wxWindow* parent, BGStyle bg_style, const ColRGBA& bg_colour) :
	wxGLCanvas(parent, -1, OpenGL::getWxGLAttribs(), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS),
	bg_style_{ bg_style },
	bg_colour_{ bg_colour }
{
	// Do nothing on erase background event to avoid flicker
	Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});

	// Custom paint
	Bind(wxEVT_PAINT, &GLCanvas::onPaint, this);

	// Update matrices etc. when resized
	Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
		projection_ortho_ = glm::ortho(
			0.f, static_cast<float>(GetSize().x), static_cast<float>(GetSize().y), 0.f, -1.f, 1.f);
		updateBackgroundVB();
	});
}

GLCanvas::~GLCanvas() {}

// -----------------------------------------------------------------------------
// Sets the current gl context to the canvas' context, and creates it if it
// doesn't exist.
// Returns true if the context is valid, false otherwise
// -----------------------------------------------------------------------------
bool GLCanvas::activateContext()
{
	auto context = OpenGL::getContext(this);

	if (context)
	{
		context->SetCurrent(*this);
		return true;
	}
	else
		return false;
}

void GLCanvas::draw()
{
	static OpenGL::VertexBuffer2D testbuf;
	if (testbuf.empty())
	{
		testbuf.add({ { 50.f, 50.f }, { 1.f, 0.f, 0.f, 1.f }, { 0.f, 0.f } });
		testbuf.add({ { 50.f, 150.f }, { 0.f, 1.f, 0.f, 0.8f }, { 0.f, 0.f } });
		testbuf.add({ { 150.f, 50.f }, { 0.f, 0.f, 1.f, 0.4f }, { 0.f, 0.f } });
	}

	auto& shader = OpenGL::Draw2D::defaultShader(false);
	OpenGL::Draw2D::setupFor2D(shader, *this);
	testbuf.draw();
}

void GLCanvas::init()
{
	OpenGL::init();

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

	init_done_ = true;
}

void GLCanvas::updateBackgroundVB()
{
	if (bg_style_ != BGStyle::Checkered)
		return;

	if (!vb_background_)
		vb_background_ = std::make_unique<OpenGL::VertexBuffer2D>();

	auto widthf  = static_cast<float>(GetSize().x);
	auto heightf = static_cast<float>(GetSize().y);

	vb_background_->clear();
	vb_background_->add({ { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f } });
	vb_background_->add({ { 0.f, heightf }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, heightf / 16.f } });
	vb_background_->add({ { widthf, heightf }, { 1.f, 1.f, 1.f, 1.f }, { widthf / 16.f, heightf / 16.f } });
	vb_background_->add({ { widthf, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { widthf / 16.f, 0.f } });
}

void GLCanvas::drawCheckeredBackground()
{
	using namespace OpenGL;

	if (!vb_background_)
		updateBackgroundVB();

	// Set background texture
	glEnable(GL_TEXTURE_2D);
	Texture::bind(Texture::backgroundTexture());

	// Setup default shader
	auto& shader = Draw2D::defaultShader();
	Draw2D::setupFor2D(shader, *this);

	// Draw
	vb_background_->draw(Primitive::Quads);
}

void GLCanvas::onPaint(wxPaintEvent& e)
{
	wxPaintDC(this);

	if (!IsShown())
		return;

	// Set context to this window
	if (!activateContext())
		return;

	if (!init_done_)
		init();

	// Set viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Clear
	glClearColor(bg_colour_.fr(), bg_colour_.fg(), bg_colour_.fb(), 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw checkered background if needed
	if (bg_style_ == BGStyle::Checkered)
		drawCheckeredBackground();

	// Draw content & show
	draw();
	SwapBuffers();

	// Cleanup state
	OpenGL::Shader::unbind();
	OpenGL::bindVAO(0);
}









// Testing
#include "General/Console/Console.h"

CONSOLE_COMMAND(tgc, 0, false)
{
	wxDialog dlg(
		nullptr, -1, "GLCanvas Test", wxDefaultPosition, { 800, 800 }, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

	dlg.SetSizer(new wxBoxSizer(wxVERTICAL));
	dlg.GetSizer()->Add(new GLCanvas{ &dlg }, 1, wxEXPAND | wxALL, 10);
	dlg.CenterOnParent();
	dlg.ShowModal();
}
