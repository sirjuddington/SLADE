#pragma once

// Forward declarations
namespace slade
{
struct Ray;
} // namespace slade
namespace slade::gl
{
class Camera;
class LineBuffer;
class Shader;
} // namespace slade::gl
namespace slade::mapeditor
{
struct RenderGroup;
struct Item;
enum class RenderPass : u8;
struct Quad3D;
class MapGeometryBuffer3D;
class MapRenderer3D;
} // namespace slade::mapeditor


namespace slade::mapeditor
{
class WallRenderer3D
{
public:
	struct LineQuads
	{
		const MapLine* line;
		vector<Quad3D> quads;
		unsigned       vertex_buffer_offset;
		long           updated_time;
		bool           visible = true;
	};

	WallRenderer3D(MapRenderer3D* renderer);
	~WallRenderer3D();

	void clear();

	void  updateVisibility(const gl::Camera& camera, float max_dist);
	bool  update(bool vis_check = false);
	float updateProgress() const;

	void render(const gl::Shader& shader, RenderPass pass) const;
	void renderSkyGeometry() const;

	void addOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const;
	void addItemIndices(const Item& item, vector<GLuint>& indices) const;

	std::pair<Item, float> nearestIntersectingWall(const Ray& ray, float max_dist) const;

	const vector<LineQuads>& lineQuads() const { return line_quads_; }
	unsigned                 vao() const;
	unsigned                 bufferSize() const;

private:
	MapRenderer3D*                  renderer_;
	vector<LineQuads>               line_quads_;
	unique_ptr<MapGeometryBuffer3D> vb_quads_;
	vector<RenderGroup>             quad_groups_;
	long                            quads_updated_             = 0;
	int                             line_quads_processed_      = -1;
	unsigned                        quad_vb_processing_offset_ = 0;
	bool                            force_update_quad_groups_  = false;
};
} // namespace slade::mapeditor
