#pragma once

// Forward declarations
namespace slade::gl
{
class LineBuffer;
}
namespace slade::mapeditor
{
struct MGVertex;
struct Quad3D;
struct Flat3D;
} // namespace slade::mapeditor


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
TexTransformInfo getSectorTextureTransformInfo(const MapSector& sector, bool ceiling, Vec2d tex_scale);

std::tuple<vector<Flat3D>, vector<MGVertex>> generateSectorFlats(const MapSector& sector, unsigned vertex_index);
std::tuple<vector<Quad3D>, vector<MGVertex>> generateLineQuads(const MapLine& line, unsigned vertex_index);
} // namespace slade::mapeditor
