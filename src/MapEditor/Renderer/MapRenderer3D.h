#pragma once

#include "SelectionOverlay3D.h"

namespace slade
{
class MCA3dSelection;
}
// Forward declarations
namespace slade::gl
{
class Camera;
class IndexBuffer;
class LineBuffer;
class Shader;
class View;
struct Vertex3D;
} // namespace slade::gl
namespace slade::mapeditor
{
class ItemSelection;
class MapGeometryBuffer3D;
class Renderer;
class Skybox;
enum class RenderPass : u8;
struct Flat3D;
struct Item;
struct Quad3D;
} // namespace slade::mapeditor


namespace slade::mapeditor
{
class MapRenderer3D
{
public:
	MapRenderer3D(SLADEMap* map, Renderer* renderer);
	~MapRenderer3D();

	bool fogEnabled() const { return fog_; }
	bool fullbrightEnabled() const { return fullbright_; }

	void enableHighlight(bool enable = true) { highlight_enabled_ = enable; }
	void enableSelection(bool enable = true) { selection_enabled_ = enable; }
	void enableFog(bool enable = true) { fog_ = enable; }
	void enableFullbright(bool enable = true) { fullbright_ = enable; }
	void setSkyTexture(string_view tex1, string_view tex2 = "") const;

	void render(const gl::Camera& camera);
	void renderHighlight(const Item& item, const gl::Camera& camera, const gl::View& view, float alpha = 1.0f);

	void updateSelection(const ItemSelection& selection);
	void populateSelectionOverlay(SelectionOverlay3D& overlay, const vector<Item>& items) const;
	void renderSelectionOverlay(
		const gl::Camera&         camera,
		const gl::View&           view,
		const SelectionOverlay3D& overlay,
		glm::vec4                 colour,
		float                     alpha = 1.0f) const;
	void renderSelection(const gl::Camera& camera, const gl::View& view) const;

	void clearData();

	Item findHighlightedItem(const gl::Camera& camera, const gl::View& view, const Vec2i& cursor_pos);

	// Testing
	unsigned flatsBufferSize() const;
	unsigned quadsBufferSize() const;

private:
	SLADEMap*              map_      = nullptr;
	Renderer*              renderer_ = nullptr;
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
		bool             visible = true;
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
		bool           visible = true;
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
	vector<RenderGroup> flat_groups_;
	vector<RenderGroup> quad_groups_;
	bool                force_update_flat_groups_ = false;
	bool                force_update_quad_groups_ = false;

	// Highlighted/selected items
	bool                        highlight_enabled_ = true;
	bool                        selection_enabled_ = true;
	unique_ptr<gl::LineBuffer>  highlight_lines_;
	unique_ptr<gl::IndexBuffer> highlight_fill_;
	SelectionOverlay3D          selection_overlay_;

	void updateFlatVisibility(const gl::Camera& camera, float max_dist);
	void updateWallVisibility(const gl::Camera& camera, float max_dist);

	void updateFlats(bool vis_check = false);
	void updateWalls(bool vis_check = false);

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
