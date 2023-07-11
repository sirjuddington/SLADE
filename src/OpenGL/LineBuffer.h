#pragma once

#include "OpenGL.h"

namespace slade::gl
{
class Shader;

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

	~LineBuffer()
	{
		gl::deleteVBO(vbo_);
		gl::deleteVAO(vao_);
	}

	// Vector-like access to lines
	unsigned    size() const { return lines_.size(); }
	void        clear() { lines_.clear(); }
	bool        empty() const { return lines_.empty(); }
	const Line& operator[](unsigned index) const { return lines_[index]; }
	Line&       operator[](unsigned index)
	{
		lines_updated_ = true;
		return lines_[index];
	}

	void add(const Line& line);
	void add(const vector<Line>& lines);

	void draw() const;

	static const Shader& shader(float aa_radius = 2.0f);

private:
	vector<Line>     lines_;
	mutable unsigned vao_           = 0;
	mutable unsigned vbo_           = 0;
	mutable bool     lines_updated_ = false;

	void initVAO() const;
	void updateVBO() const;
};
} // namespace slade::gl
