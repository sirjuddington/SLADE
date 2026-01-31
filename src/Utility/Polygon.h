#pragma once

namespace slade
{
class MapSector;

namespace gl
{
	struct Vertex2D;
} // namespace gl

namespace mapeditor
{
	struct TexTransformInfo;
}

namespace polygon
{
	vector<glm::vec2> generateSectorTriangles(const MapSector& sector);

	glm::vec2 calculateTexCoords(
		float x,
		float y,
		float tex_width,
		float tex_height,
		float scale_x,
		float scale_y,
		float offset_x,
		float offset_y,
		float rotation);

	glm::vec2 calculateTexCoords(glm::vec2 vertex, glm::vec2 tex_size, const mapeditor::TexTransformInfo& tti);

	bool generateTextureCoords(
		vector<gl::Vertex2D>& vertices,
		unsigned              texture,
		float                 scale_x  = 1.0f,
		float                 scale_y  = 1.0f,
		float                 offset_x = 0.0f,
		float                 offset_y = 0.0f,
		float                 rotation = 0.0f);
} // namespace polygon

} // namespace slade
