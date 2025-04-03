#include "Main.h"
#include "PointSpriteBuffer.h"
#include "Shader.h"
#include "View.h"

using namespace slade;
using namespace gl;

namespace
{
unique_ptr<Shader> shader_ps_textured;
unique_ptr<Shader> shader_ps_circle;
unique_ptr<Shader> shader_ps_circle_outline;
unique_ptr<Shader> shader_ps_rsquare;
unique_ptr<Shader> shader_ps_rsquare_outline;
} // namespace

namespace
{
void loadShaders()
{
	shader_ps_textured = std::make_unique<Shader>("ps_textured");
	shader_ps_textured->define("TEXTURED");
	shader_ps_textured->loadResourceEntries("point_sprite.vert", "default2d.frag");

	shader_ps_circle = std::make_unique<Shader>("ps_circle");
	shader_ps_circle->loadResourceEntries("point_sprite.vert", "circle.frag");

	shader_ps_circle_outline = std::make_unique<Shader>("ps_circle_outline");
	shader_ps_circle_outline->define("OUTLINE");
	shader_ps_circle_outline->loadResourceEntries("point_sprite.vert", "circle.frag");

	shader_ps_rsquare = std::make_unique<Shader>("ps_rsquare");
	shader_ps_rsquare->loadResourceEntries("point_sprite.vert", "rounded_square.frag");

	shader_ps_rsquare_outline = std::make_unique<Shader>("ps_rsquare_outline");
	shader_ps_rsquare_outline->define("OUTLINE");
	shader_ps_rsquare_outline->loadResourceEntries("point_sprite.vert", "rounded_square.frag");
}

Shader* pointSpriteShader(PointSpriteType type)
{
	if (!shader_ps_textured)
		loadShaders();

	switch (type)
	{
	case PointSpriteType::Textured:             return shader_ps_textured.get();
	case PointSpriteType::Circle:               return shader_ps_circle.get();
	case PointSpriteType::CircleOutline:        return shader_ps_circle_outline.get();
	case PointSpriteType::RoundedSquare:        return shader_ps_rsquare.get();
	case PointSpriteType::RoundedSquareOutline: return shader_ps_rsquare_outline.get();
	default:                                    return shader_ps_textured.get();
	}
}
} // namespace

PointSpriteBuffer::~PointSpriteBuffer()
{
	gl::deleteVAO(vao_);
}

void PointSpriteBuffer::add(const glm::vec2& position, float radius)
{
	sprites_.emplace_back(position, radius);
}

void PointSpriteBuffer::add(const vector<glm::vec2>& positions, float radius)
{
	for (const auto& pos : positions)
		sprites_.emplace_back(pos, radius);
}

void PointSpriteBuffer::push()
{
	if (!vao_)
		initVAO();

	buffer_.upload(sprites_);
	sprites_.clear();
}

void PointSpriteBuffer::draw(PointSpriteType type, const View* view, unsigned count) const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_.empty())
		return;

	// Check valid parameters
	if (count == 0)
		count = buffer_.size();
	if (count > buffer_.size())
		return;

	// Setup shader
	auto shader = pointSpriteShader(type);
	if (view)
		view->setupShader(*shader);
	shader->setUniform("point_radius", radius_);
	shader->setUniform("colour", colour_);
	if (type == PointSpriteType::CircleOutline || type == PointSpriteType::RoundedSquareOutline)
	{
		shader->setUniform("outline_width", outline_width_);
		shader->setUniform("fill_opacity", fill_opacity_);
	}

	// Draw
	gl::bindVAO(vao_);
	gl::drawArraysInstanced(Primitive::Triangles, 0, 6, count);
	gl::bindVAO(0);
}

void PointSpriteBuffer::initVAO()
{
	vao_ = createVAO();
	bindVAO(vao_);

	// -------------------------------------------------------------------------

	// Create+init square buffer
	buffer_square_ = std::make_unique<Buffer<glm::vec2>>();
	buffer_square_->bind();
	buffer_square_->upload(
		vector<glm::vec2>{
			{ -1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } });

	// Setup vertex attributes for square

	// Position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// -------------------------------------------------------------------------

	// Setup vertex attributes for sprite instances

	buffer_.bind();

	// Position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// Radius
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	gl::bindVAO(0);
}
