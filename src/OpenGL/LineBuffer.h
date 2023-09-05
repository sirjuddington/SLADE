#pragma once

#include "Buffer.h"

namespace slade::gl
{
class Shader;
class View;

class LineBuffer
{
public:
	struct Line
	{
		glm::vec4 v1_pos_width;
		glm::vec4 v1_colour;
		glm::vec4 v2_pos_width;
		glm::vec4 v2_colour;
	};

	LineBuffer() = default;
	~LineBuffer();

	const Buffer<Line>& buffer() const { return buffer_; }
	Buffer<Line>&       buffer() { return buffer_; }
	float               widthMult() const { return width_mult_; }
	glm::vec2           aaRadius() const { return aa_radius_; }

	void setWidthMult(float width) { width_mult_ = width; }
	void setAaRadius(float x, float y) { aa_radius_ = { x, y }; }
	void setDashed(bool dashed, float dash_size = 6.0f, float gap_size = 6.0f)
	{
		dashed_        = dashed;
		dash_size_     = dash_size;
		dash_gap_size_ = gap_size;
	}

	void add(const Line& line);
	void add(const vector<Line>& lines);
	void add2d(float x1, float y1, float x2, float y2, glm::vec4 colour, float width = 1.0f)
	{
		add(Line{ { x1, y1, 0.0f, width }, colour, { x2, y2, 0.0f, width }, colour });
	}
	void addArrow(
		const Rectf& line,
		glm::vec4    colour,
		float        width            = 1.0f,
		float        arrowhead_length = 0.0f,
		float        arrowhead_angle  = 45.0f,
		bool         arrowhead_both   = false);

	void push();

	void draw(
		const View*      view   = nullptr,
		const glm::vec4& colour = glm::vec4{ 1.0f },
		const glm::mat4& model  = glm::mat4{ 1.0f }) const;

	static const Shader& shader();

private:
	float     width_mult_    = 1.0f;
	glm::vec2 aa_radius_     = { 2.0f, 2.0f };
	bool      dashed_        = false;
	float     dash_size_     = 6.0f;
	float     dash_gap_size_ = 6.0f;

	vector<Line> lines_;
	Buffer<Line> buffer_;
	unsigned     vao_ = 0;
};
} // namespace slade::gl
