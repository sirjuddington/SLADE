#pragma once

#include "Buffer.h"
#include "Geometry/RectFwd.h"

namespace slade
{
struct Plane;
}

namespace slade::gl
{
class Camera;
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

	LineBuffer(bool map_3d = false) : map_3d_{ map_3d } {}
	~LineBuffer();

	const Buffer<Line>& buffer() const { return buffer_; }
	Buffer<Line>&       buffer() { return buffer_; }
	float               widthMult() const { return width_mult_; }
	glm::vec2           aaRadius() const { return aa_radius_; }
	unsigned            queueSize() const { return lines_.size(); }
	Shader*             shader(bool map_3d = false) const;
	unsigned            vao() const { return vao_; }

	void setWidthMult(float width) { width_mult_ = width; }
	void setAaRadius(float x, float y) { aa_radius_ = { x, y }; }
	void setDashed(bool dashed, float dash_size = 6.0f, float gap_size = 6.0f)
	{
		dashed_        = dashed;
		dash_size_     = dash_size;
		dash_gap_size_ = gap_size;
	}
	void setMaxDistance(float max_distance) { max_dist_ = max_distance; }
	void setShaderUniforms(const Shader& shader) const;

	void add(const Line& line);
	void add(const vector<Line>& lines);
	void add2d(float x1, float y1, float x2, float y2, glm::vec4 colour, float width = 1.0f)
	{
		add(Line{ { x1, y1, 0.0f, width }, colour, { x2, y2, 0.0f, width }, colour });
	}
	void add3d(glm::vec3 start, glm::vec3 end, glm::vec4 colour, float width = 1.0f)
	{
		add(Line{ { start.x, start.y, start.z, width }, colour, { end.x, end.y, end.z, width }, colour });
	}
	void add3d(glm::vec2 start, glm::vec2 end, const Plane& plane, glm::vec4 colour, float width = 1.0f);
	void addArrow(
		const Rectf& line,
		glm::vec4    colour,
		float        width            = 1.0f,
		float        arrowhead_length = 0.0f,
		float        arrowhead_angle  = 45.0f,
		bool         arrowhead_both   = false);
	void addArrow3d(
		glm::vec3 start,
		glm::vec3 end,
		glm::vec4 colour,
		float     width            = 1.0f,
		float     arrowhead_length = 0.0f,
		float     arrowhead_angle  = 45.0f,
		bool      arrowhead_both   = false);

	void push(bool clear = true);
	bool pull();

	void draw(
		const View*      view   = nullptr,
		const glm::vec4& colour = glm::vec4{ 1.0f },
		const glm::mat4& model  = glm::mat4{ 1.0f }) const;

	void draw(
		const Camera&    camera,
		glm::vec2        viewport_size,
		const glm::vec4& colour = glm::vec4{ 1.0f },
		const glm::mat4& model  = glm::mat4{ 1.0f }) const;

private:
	float     width_mult_    = 1.0f;
	glm::vec2 aa_radius_     = { 2.0f, 2.0f };
	bool      dashed_        = false;
	float     dash_size_     = 6.0f;
	float     dash_gap_size_ = 6.0f;
	bool      map_3d_        = false; // If true this line buffer is for the map editor 3d mode (enable distance fade)
	float     max_dist_      = 0.0f;

	vector<Line> lines_;
	Buffer<Line> buffer_;
	unsigned     vao_ = 0;
};
} // namespace slade::gl
