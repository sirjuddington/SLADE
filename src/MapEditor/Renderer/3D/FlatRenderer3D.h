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
struct Flat3D;
struct RenderGroup;
struct Item;
enum class RenderPass : u8;
class MapGeometryBuffer3D;
class MapRenderer3D;
} // namespace slade::mapeditor


namespace slade::mapeditor
{
class FlatRenderer3D
{
public:
	struct SectorFlats
	{
		const MapSector* sector;
		vector<Flat3D>   flats;
		unsigned         vertex_buffer_offset;
		long             updated_time;
		bool             visible = true;
	};

	FlatRenderer3D(MapRenderer3D* renderer);
	~FlatRenderer3D();

	void clear();
	void updateVisibility(const gl::Camera& camera, float max_dist);
	bool update(bool vis_check = false);
	void render(const gl::Shader& shader, RenderPass pass) const;
	void renderSkyGeometry() const;
	void addOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const;
	void addItemIndices(const Item& item, vector<GLuint>& indices) const;

	std::pair<Item, float> nearestIntersectingFlat(const Ray& ray, float max_dist) const;

	const vector<SectorFlats>& sectorFlats() const { return sector_flats_; }
	unsigned                   vao() const;
	unsigned                   bufferSize() const;

private:
	MapRenderer3D*                  renderer_;
	vector<SectorFlats>             sector_flats_;
	unique_ptr<MapGeometryBuffer3D> vb_flats_;
	vector<RenderGroup>             flat_groups_;
	long                            flats_updated_               = 0;
	int                             sector_flats_processed_      = -1;
	unsigned                        sector_vb_processing_offset_ = 0;
	bool                            force_update_flat_groups_    = false;
};
} // namespace slade::mapeditor
