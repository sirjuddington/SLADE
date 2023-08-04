
#include "Main.h"
#include "GLCanvas.h"
#include "Graphics/Palette/Palette.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace slade;

GLCanvas::GLCanvas(wxWindow* parent, BGStyle bg_style, const ColRGBA& bg_colour, gl::View view) :
	wxGLCanvas(parent, gl::getWxGLAttribs(), -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS),
	view_{ std::move(view) },
	bg_style_{ bg_style },
	bg_colour_{ bg_colour }
{
	// Do nothing on erase background event to avoid flicker
	Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});

	// Custom paint
	Bind(wxEVT_PAINT, &GLCanvas::onPaint, this);

	// Update matrices etc. when resized
	Bind(
		wxEVT_SIZE,
		[this](wxSizeEvent& e)
		{
			view_.setSize(GetSize().x, GetSize().y);
			updateBackgroundVB();
		});
}

GLCanvas::~GLCanvas() = default;

void GLCanvas::setPalette(const Palette* pal)
{
	if (!palette_)
		palette_ = std::make_unique<Palette>(*pal);
	else
		palette_->copyPalette(pal);
}

void GLCanvas::setupMousewheelZoom()
{
	Bind(
		wxEVT_MOUSEWHEEL,
		[&](wxMouseEvent& e)
		{
			if (e.GetWheelRotation() < 0)
				view_.zoomToward(0.8, { e.GetPosition().x, e.GetPosition().y });
			else if (e.GetWheelRotation() > 0)
				view_.zoomToward(1.25, { e.GetPosition().x, e.GetPosition().y });

			Refresh();
		});
}

void GLCanvas::setupMousePanning()
{
	Bind(
		wxEVT_MOTION,
		[&](wxMouseEvent& e)
		{
			if (e.MiddleIsDown())
			{
				auto cpos_current = view_.canvasPos({ e.GetPosition().x, e.GetPosition().y });
				auto cpos_prev    = view_.canvasPos(mouse_prev_);

				view_.pan(cpos_prev.x - cpos_current.x, cpos_prev.y - cpos_current.y);

				Refresh();
			}
			else
				e.Skip();

			mouse_prev_.set(e.GetPosition().x, e.GetPosition().y);
		});
}

// -----------------------------------------------------------------------------
// Sets the current gl context to the canvas' context, and creates it if it
// doesn't exist.
// Returns true if the context is valid, false otherwise
// -----------------------------------------------------------------------------
bool GLCanvas::activateContext()
{
	if (auto* context = gl::getContext(this))
	{
		context->SetCurrent(*this);
		return true;
	}

	return false;
}

void GLCanvas::draw()
{
	static gl::VertexBuffer2D testbuf;
	if (testbuf.empty())
	{
		testbuf.add({ { 50.f, 50.f }, { 1.f, 0.f, 0.f, 1.f }, { 0.f, 0.f } });
		testbuf.add({ { 50.f, 150.f }, { 0.f, 1.f, 0.f, 0.8f }, { 0.f, 0.f } });
		testbuf.add({ { 150.f, 50.f }, { 0.f, 0.f, 1.f, 0.4f }, { 0.f, 0.f } });
	}

	auto& shader = gl::draw2d::defaultShader(false);
	view_.setupShader(shader);
	testbuf.draw();
}

void GLCanvas::init()
{
	gl::init();

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
		vb_background_ = std::make_unique<gl::VertexBuffer2D>();

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
	if (!vb_background_)
		updateBackgroundVB();

	// Set background texture
	glEnable(GL_TEXTURE_2D);
	gl::Texture::bind(gl::Texture::backgroundTexture());

	// Setup default shader
	auto& shader = gl::draw2d::defaultShader();
	shader.bind();
	shader.setUniform("mvp", view_.projectionMatrix());
	shader.setUniform("colour", glm::vec4(1.0f));
	shader.setUniform("viewport_size", glm::vec2(view_.size().x, view_.size().y));

	// Draw
	vb_background_->draw(gl::Primitive::Quads);
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
	gl::Shader::unbind();
	gl::bindVAO(0);
}









// Testing
#include "General/Console.h"

CONSOLE_COMMAND(tgc, 0, false)
{
	wxDialog dlg(
		nullptr, -1, "GLCanvas Test", wxDefaultPosition, { 800, 800 }, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

	auto canvas = new GLCanvas{ &dlg, GLCanvas::BGStyle::Checkered, ColRGBA::BLACK, gl::View{ false, false, false } };

	dlg.SetSizer(new wxBoxSizer(wxVERTICAL));
	dlg.GetSizer()->Add(canvas, 1, wxEXPAND | wxALL, 10);

	canvas->setupMousewheelZoom();
	canvas->setupMousePanning();

	canvas->Bind(
		wxEVT_LEFT_DOWN,
		[&](wxMouseEvent& e)
		{
			auto pos_screen      = e.GetPosition();
			auto pos_canvas      = canvas->view().canvasPos({ pos_screen.x, pos_screen.y });
			auto pos_screen_calc = Vec2i{ canvas->view().screenX(pos_canvas.x), canvas->view().screenY(pos_canvas.y) };
			log::info(
				"Screen: {},{} | Canvas: {},{} | Screen (from Canvas): {},{}",
				pos_screen.x,
				pos_screen.y,
				pos_canvas.x,
				pos_canvas.y,
				pos_screen_calc.x,
				pos_screen_calc.y);
		});

	canvas->Bind(
		wxEVT_KEY_DOWN,
		[&](wxKeyEvent& e)
		{
			if (e.GetKeyCode() == WXK_LEFT)
				canvas->view().pan(-8, 0);
			else if (e.GetKeyCode() == WXK_RIGHT)
				canvas->view().pan(8, 0);
			else if (e.GetKeyCode() == WXK_UP)
				canvas->view().pan(0, -8);
			else if (e.GetKeyCode() == WXK_DOWN)
				canvas->view().pan(0, 8);
			else if (e.GetKeyCode() == '=')
				canvas->view().zoom(1.25);
			else if (e.GetKeyCode() == '-')
				canvas->view().zoom(0.8);

			canvas->Refresh();
		});

	dlg.CenterOnParent();
	dlg.ShowModal();
}