#pragma once

namespace slade
{
class MapVertex;
namespace gl::draw2d
{
	struct Context;
}

class VertexInfoOverlay
{
public:
	VertexInfoOverlay()  = default;
	~VertexInfoOverlay() = default;

	void update(MapVertex* vertex);
	void draw(gl::draw2d::Context& dc, float alpha = 1.0f) const;

private:
	vector<string> info_;
};
} // namespace slade
