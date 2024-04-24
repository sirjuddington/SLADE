#pragma once

namespace slade
{
class MapSector;

namespace gl
{
	struct Vertex2D;
	struct Vertex3D;
} // namespace gl

namespace polygon
{
	vector<glm::vec2> generateSectorTriangles(const MapSector& sector);
	bool generateTextureCoords(
		vector<gl::Vertex2D>& vertices,
		unsigned              texture,
		float                 scale_x  = 1.0f,
		float                 scale_y  = 1.0f,
		float                 offset_x = 0.0f,
		float                 offset_y = 0.0f,
		float                 rotation = 0.0f);
	bool generateTextureCoords(
		vector<gl::Vertex3D>& vertices,
		unsigned              texture,
		float                 scale_x  = 1.0f,
		float                 scale_y  = 1.0f,
		float                 offset_x = 0.0f,
		float                 offset_y = 0.0f,
		float                 rotation = 0.0f);
} // namespace polygon

} // namespace slade
