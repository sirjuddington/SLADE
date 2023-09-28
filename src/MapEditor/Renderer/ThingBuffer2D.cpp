
#include "Main.h"
#include "ThingBuffer2D.h"
#include "Game/ThingType.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/Shader.h"
#include "OpenGL/View.h"
#include "Utility/MathStuff.h"

using namespace slade;
using namespace gl;


namespace
{
enum class TexType
{
	None   = 0,
	Icon   = 1,
	Sprite = 2
};

struct ThingShader
{
	unique_ptr<Shader> shader;
	bool               square    = false;
	TexType            tex_type  = TexType::None;
	bool               direction = false;
};
vector<ThingShader> thing_shaders;

} // namespace


namespace
{
Shader* thingShader(bool square, TexType textured, bool direction)
{
	// Check if already loaded
	for (const auto& shader : thing_shaders)
	{
		if (shader.square == square && shader.tex_type == textured && shader.direction == direction)
			return shader.shader.get();
	}

	// Build shader name
	string shader_name = "thing2d";
	if (square)
		shader_name += "_square";
	else
		shader_name += "_round";
	if (textured == TexType::Icon)
		shader_name += "_icon";
	else if (textured == TexType::Sprite)
		shader_name += "_sprite";
	if (direction)
		shader_name += "_arrow";

	// Load the shader with requested properties
	thing_shaders.push_back({ std::make_unique<Shader>(shader_name), square, textured, direction });
	auto shader = thing_shaders.back().shader.get();
	if (square)
		shader->define("SQUARE");
	if (textured == TexType::Icon)
		shader->define("ICON");
	else if (textured == TexType::Sprite)
		shader->define("SPRITE");
	if (direction)
		shader->define("ARROW");
	shader->loadResourceEntries("thing2d.vert", "thing2d.frag");

	return shader;
}
} // namespace


ThingBuffer2D::ThingBuffer2D()
{
	buffer_things_ = std::make_unique<Buffer<ThingInstance>>();
}

void ThingBuffer2D::setup(const game::ThingType& type)
{
	colour_         = type.colour().asVec4();
	radius_         = type.radius();
	arrow_          = type.angled();
	shrink_on_zoom_ = type.shrinkOnZoom();
}

void ThingBuffer2D::setTexture(unsigned texture, bool sprite)
{
	texture_ = texture;
	sprite_  = sprite;

	if (sprite_)
	{
		const auto& tex = Texture::info(texture_);

		if (tex.size.x > tex.size.y)
		{
			tex_size_.x = 1.0f;
			tex_size_.y = static_cast<float>(tex.size.y) / static_cast<float>(tex.size.x);
		}
		else
		{
			tex_size_.y = 1.0f;
			tex_size_.x = static_cast<float>(tex.size.x) / static_cast<float>(tex.size.y);
		}

		tex_size_.x = 1.0f / (tex_size_.x * 0.75f);
		tex_size_.y = 1.0f / (tex_size_.y * 0.75f);
	}
	else
		tex_size_ = glm::vec2{ 1.0f };
}

void ThingBuffer2D::add(float x, float y, float angle, float alpha)
{
	auto dir = math::vectorAngle(math::degToRad(angle));
	things_.emplace_back(glm::vec2{ x, y }, glm::vec2{ dir.x, dir.y }, alpha);
}

void ThingBuffer2D::push()
{
	if (!vao_)
		initVAO();

	buffer_things_->upload(things_);
	things_.clear();
}

void ThingBuffer2D::draw(const View* view, const glm::vec4& colour, bool square, bool force_arrow) const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_things_->empty())
		return;

	// Determine radius
	auto radius = radius_;
	if (shrink_on_zoom_ && view)
	{
		if (view->scale().x > 1.0)
			radius /= view->scale().x;
	}

	// Setup shader
	auto shader = thingShader(
		square, texture_ > 0 ? (sprite_ ? TexType::Sprite : TexType::Icon) : TexType::None, arrow_ || force_arrow);
	shader->bind();
	if (view)
		view->setupShader(*shader);
	shader->setUniform("colour", colour_ * colour);
	shader->setUniform("radius", radius);
	if (sprite_ && !square)
		shader->setUniform("tex_size", tex_size_ * 1.2f); // Smaller sprite if round background
	else
		shader->setUniform("tex_size", tex_size_);
	shader->setUniform("shadow_opacity", shadow_opacity_);

	gl::bindVAO(vao_);
	gl::Texture::bind(texture_);

	gl::drawArraysInstanced(Primitive::Triangles, 0, 6, buffer_things_->size());

	gl::bindVAO(0);
}

void ThingBuffer2D::initVAO()
{
	vao_ = gl::createVAO();
	gl::bindVAO(vao_);

	// -------------------------------------------------------------------------

	// Create+init square buffer
	buffer_square_ = std::make_unique<Buffer<glm::vec2>>();
	buffer_square_->bind();
	buffer_square_->upload(vector<glm::vec2>{
		{ -1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } });

	// Setup vertex attributes for square

	// Position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// -------------------------------------------------------------------------

	// Create thing instance buffer if needed
	if (!buffer_things_)
		buffer_things_ = std::make_unique<Buffer<ThingInstance>>();
	buffer_things_->bind();

	// Setup vertex attributes for thing instances

	// Position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// Direction
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	// Alpha
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	// -------------------------------------------------------------------------

	gl::bindVAO(0);
}
