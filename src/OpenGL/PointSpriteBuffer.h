#pragma once

#include "Buffer.h"

namespace slade::gl
{
class Shader;
class View;
class PointSpriteBuffer
{
public:
	struct PointSprite
	{
		glm::vec2 position;
		float     radius;
		PointSprite(const glm::vec2& pos, float radius) : position{ pos }, radius{ radius } {}
	};

	PointSpriteBuffer() = default;
	~PointSpriteBuffer();

	const Buffer<PointSprite>& buffer() const { return buffer_; }
	Buffer<PointSprite>&       buffer() { return buffer_; }

	const glm::vec4& colour() const { return colour_; }
	float            pointRadius() const { return radius_; }
	float            outlineWidth() const { return outline_width_; }
	float            fillOpacity() const { return fill_opacity_; }

	void setColour(const glm::vec4& colour) { colour_ = colour; }
	void setPointRadius(float radius) { radius_ = radius; }
	void setOutlineWidth(float width) { outline_width_ = width; }
	void setFillOpacity(float opacity) { fill_opacity_ = opacity; }

	void add(const glm::vec2& position, float radius = 1.0f);
	void add(const vector<glm::vec2>& positions, float radius = 1.0f);

	void push();

	void draw(PointSpriteType type, const View* view, unsigned count = 0) const;

private:
	vector<PointSprite>           sprites_;
	unsigned                      vao_ = 0;
	Buffer<PointSprite>           buffer_;
	unique_ptr<Buffer<glm::vec2>> buffer_square_;

	glm::vec4 colour_;
	float     radius_        = 1.0f;
	float     outline_width_ = 0.05f; // For outline types
	float     fill_opacity_  = 0.0f;  // For outline types

	void initVAO();
};
} // namespace slade::gl
