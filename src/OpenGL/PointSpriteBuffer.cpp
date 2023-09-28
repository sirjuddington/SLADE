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
	shader_ps_textured->loadResourceEntries("point_sprite.vert", "default2d.frag", "point_sprite.geom");

	shader_ps_circle = std::make_unique<Shader>("ps_circle");
	shader_ps_circle->loadResourceEntries("point_sprite.vert", "circle.frag", "point_sprite.geom");

	shader_ps_circle_outline = std::make_unique<Shader>("ps_circle_outline");
	shader_ps_circle_outline->define("OUTLINE");
	shader_ps_circle_outline->loadResourceEntries("point_sprite.vert", "circle.frag", "point_sprite.geom");

	shader_ps_rsquare = std::make_unique<Shader>("ps_rsquare");
	shader_ps_rsquare->loadResourceEntries("point_sprite.vert", "rounded_square.frag", "point_sprite.geom");

	shader_ps_rsquare_outline = std::make_unique<Shader>("ps_rsquare_outline");
	shader_ps_rsquare_outline->define("OUTLINE");
	shader_ps_rsquare_outline->loadResourceEntries("point_sprite.vert", "rounded_square.frag", "point_sprite.geom");
}

Shader* pointSpriteShader(PointSpriteType type)
{
	if (!shader_ps_textured)
		loadShaders();

	switch (type)
	{
	case PointSpriteType::Textured: return shader_ps_textured.get();
	case PointSpriteType::Circle: return shader_ps_circle.get();
	case PointSpriteType::CircleOutline: return shader_ps_circle_outline.get();
	case PointSpriteType::RoundedSquare: return shader_ps_rsquare.get();
	case PointSpriteType::RoundedSquareOutline: return shader_ps_rsquare_outline.get();
	default: return shader_ps_textured.get();
	}
}

unsigned initVAO(Buffer<PointSpriteBuffer::PointSprite>& buffer)
{
	auto vao = createVAO();
	bindVAO(vao);

	buffer.bind();

	// Position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Radius
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	return vao;
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
		vao_ = initVAO(buffer_);

	buffer_.upload(sprites_);
	sprites_.clear();
}

void PointSpriteBuffer::draw(PointSpriteType type, const View* view, unsigned first, unsigned count) const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_.empty())
		return;

	// Check valid parameters
	if (count == 0)
		count = buffer_.size() - first;
	if (first >= buffer_.size() || first + count > buffer_.size())
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
	gl::drawArrays(Primitive::Points, first, count);
	gl::bindVAO(0);
}
