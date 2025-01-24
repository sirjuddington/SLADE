#pragma once

// Forward declarations
namespace slade
{
namespace gl
{
	struct Vertex3D;
}
namespace mapeditor
{
	struct Flat3D;
}
} // namespace slade


namespace slade::mapeditor
{
struct TexTransformInfo
{
	double ox  = 0.;
	double oy  = 0.;
	double sx  = 1.;
	double sy  = 1.;
	double rot = 0.;
};
TexTransformInfo getTextureTransformInfo(const MapSector& sector, bool ceiling, Vec2d tex_scale);

std::tuple<vector<Flat3D>, vector<gl::Vertex3D>> generateSectorFlats(const MapSector& sector, unsigned vertex_index);
void                                             updateFlat(Flat3D& flat, vector<gl::Vertex3D>& vertices);
} // namespace slade::mapeditor
