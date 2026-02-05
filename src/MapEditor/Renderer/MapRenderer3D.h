#pragma once

// Forward declarations
namespace slade::gl
{
class LineBuffer;
class IndexBuffer;
struct Vertex3D;
class Camera;
class View;
class Shader;
} // namespace slade::gl
namespace slade::mapeditor
{
enum class RenderPass : u8;
struct Item;
struct Flat3D;
struct Quad3D;
class Skybox;
class MapGeometryBuffer3D;
} // namespace slade::mapeditor


namespace slade::mapeditor
{
class MapRenderer3D
{
public:
	MapRenderer3D(SLADEMap* map);
	~MapRenderer3D();

	bool fogEnabled() const { return fog_; }
	bool fullbrightEnabled() const { return fullbright_; }

	void enableHighlight(bool enable = true) { highlight_enabled_ = enable; }
	void enableSelection(bool enable = true) {}
	void enableFog(bool enable = true) { fog_ = enable; }
	void enableFullbright(bool enable = true) { fullbright_ = enable; }
	void setSkyTexture(string_view tex1, string_view tex2 = "") const;

	void render(const gl::Camera& camera);
	void renderHighlight(const Item& item, const gl::Camera& camera, const gl::View& view, float alpha = 1.0f);

	void clearData();

	Item findHighlightedItem(const gl::Camera& camera, const gl::View& view, const Vec2i& cursor_pos);

	// Testing
	unsigned flatsBufferSize() const;
	unsigned quadsBufferSize() const;

private:
	SLADEMap*              map_ = nullptr;
	unique_ptr<gl::Shader> shader_3d_;
	unique_ptr<gl::Shader> shader_3d_alphatest_;
	unique_ptr<Skybox>     skybox_;
	bool                   fullbright_ = false;
	bool                   fog_        = true;

	// Flats
	struct SectorFlats
	{
		const MapSector* sector;
		vector<Flat3D>   flats;
		unsigned         vertex_buffer_offset;
		long             updated_time;
	};
	vector<SectorFlats>             sector_flats_;
	unique_ptr<MapGeometryBuffer3D> vb_flats_; // Vertex buffer for all flats
	long                            flats_updated_ = 0;

	// Wall quads
	struct LineQuads
	{
		const MapLine* line;
		vector<Quad3D> quads;
		unsigned       vertex_buffer_offset;
		long           updated_time;
	};
	vector<LineQuads>               line_quads_;
	unique_ptr<MapGeometryBuffer3D> vb_quads_; // Vertex buffer for all wall quads
	long                            quads_updated_ = 0;

	// Used to organise quads/flats into groups for rendering
	struct RenderGroup
	{
		unsigned                    texture;
		glm::vec4                   colour;
		unique_ptr<gl::IndexBuffer> index_buffer;
		RenderPass                  render_pass;
		bool                        trans_additive = false;
	};
	vector<RenderGroup> quad_groups_;
	vector<RenderGroup> flat_groups_;

	// Highlighted/selected items
	bool                        highlight_enabled_ = true;
	unique_ptr<gl::LineBuffer>  highlight_lines_;
	unique_ptr<gl::IndexBuffer> highlight_fill_;

	void updateFlats();
	void updateWalls();

	void renderSkyFlatsQuads() const;
	void renderGroups(
		const MapGeometryBuffer3D& buffer,
		const vector<RenderGroup>& groups,
		const gl::Shader&          shader,
		RenderPass                 pass);

	void addFlatOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const;
	void addQuadOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const;
	void addItemFlatIndices(const Item& item, vector<GLuint>& indices) const;
	void addItemQuadIndices(const Item& item, vector<GLuint>& indices) const;
};
} // namespace slade::mapeditor
