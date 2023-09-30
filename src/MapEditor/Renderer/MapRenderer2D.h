#pragma once

// Forward declarations
#include "SLADEMap/SLADEMapFwd.h"
namespace slade
{
class ItemSelection;
class ObjectEditGroup;
namespace game
{
	class ThingType;
}
namespace gl
{
	class IndexBuffer;
	class PointSpriteBuffer;
	class LineBuffer;
	class VertexBuffer2D;
	class View;
	class ThingBuffer2D;
	namespace draw2d
	{
		struct Context;
	}
} // namespace gl
namespace mapeditor
{
	struct Item;
}
} // namespace slade


namespace slade
{
class MapRenderer2D
{
public:
	MapRenderer2D(SLADEMap* map, gl::View* view);
	~MapRenderer2D();

	// Vertices
	static unsigned vertexTexture(bool overlay = false);
	float           vertexRadius(float scale = 1.0f) const;
	void            renderVertices(float alpha = 1.0f);
	void            renderVertexHilight(int index, float fade) const;
	void renderVertexSelection(gl::draw2d::Context& dc, const ItemSelection& selection, float fade = 1.0f) const;

	// Lines
	void renderLines(bool show_direction, float alpha = 1.0f);
	void renderLineHilight(gl::draw2d::Context& dc, int index, float fade) const;
	void renderLineSelection(gl::draw2d::Context& dc, const ItemSelection& selection, float fade = 1.0f) const;
	void renderTaggedLines(gl::draw2d::Context& dc, const vector<MapLine*>& lines, float fade) const;
	void renderTaggingLines(gl::draw2d::Context& dc, const vector<MapLine*>& lines, float fade) const;

	// Things
	enum ThingDrawType
	{
		Square,
		Round,
		Sprite,
		SquareSprite,
		FramedSprite,
	};
	void renderThingOverlays(
		gl::draw2d::Context&     dc,
		const vector<MapThing*>& things,
		float                    radius_extra = 0.0f,
		const Vec2d&             offset       = {}) const;
	void renderThings(float alpha = 1.0f, bool force_dir = false);
	void renderThings(const vector<MapThing*>& things, float alpha = 1.0f, const Vec2d& offset = {}) const;
	void renderThingHilight(gl::draw2d::Context& dc, int index, float fade, bool redraw_thing = true) const;
	void renderThingSelection(gl::draw2d::Context& dc, const ItemSelection& selection, float fade = 1.0f) const;
	void renderTaggedThings(gl::draw2d::Context& dc, const vector<MapThing*>& things, float fade) const;
	void renderTaggingThings(gl::draw2d::Context& dc, const vector<MapThing*>& things, float fade) const;
	void renderPathedThings(gl::draw2d::Context& dc, const vector<MapThing*>& things);
	void renderPointLightPreviews(gl::draw2d::Context& dc, float alpha, int hilight_index) const;

	// Flats (sectors)
	void renderFlats(bool ceilings, float alpha = 1.0f);
	void renderFlatHilight(gl::draw2d::Context& dc, int index, float fade) const;
	void renderFlatOverlays(const gl::draw2d::Context& dc, const vector<MapSector*>& sectors) const;
	void renderFlatSelection(gl::draw2d::Context& dc, const ItemSelection& selection, float fade = 1.0f) const;
	void renderTaggedFlats(gl::draw2d::Context& dc, const vector<MapSector*>& sectors, float fade) const;

	// Moving
	void renderMovingVertices(gl::draw2d::Context& dc, const vector<mapeditor::Item>& vertices, const Vec2d& move_vec)
		const;
	void renderMovingLines(gl::draw2d::Context& dc, const vector<mapeditor::Item>& lines, const Vec2d& move_vec) const;
	void renderMovingSectors(gl::draw2d::Context& dc, const vector<mapeditor::Item>& sectors, const Vec2d& move_vec)
		const;
	void renderMovingThings(gl::draw2d::Context& dc, const vector<mapeditor::Item>& things, const Vec2d& move_vec)
		const;

	// Paste
	void renderPasteThings(gl::draw2d::Context& dc, const vector<MapThing*>& things, const Vec2d& pos) const;

	// Object Edit
	void renderObjectEditGroup(gl::draw2d::Context& dc, ObjectEditGroup* group) const;

	// Misc
	void forceUpdate(bool flats_ceilings);
	void clearTextureCache();

private:
	SLADEMap* map_              = nullptr;
	gl::View* view_             = nullptr;
	long      vertices_updated_ = 0;
	long      lines_updated_    = 0;
	long      flats_updated_    = 0;
	long      things_updated_   = 0;

	// OpenGL Buffers
	unique_ptr<gl::LineBuffer>            lines_buffer_;
	unique_ptr<gl::VertexBuffer2D>        lines_buffer_basic_;
	unique_ptr<gl::LineBuffer>            temp_lines_buffer_;
	unique_ptr<gl::PointSpriteBuffer>     vertices_buffer_;
	unique_ptr<gl::VertexBuffer2D>        flats_buffer_;
	vector<unique_ptr<gl::ThingBuffer2D>> thing_buffers_; // One per thing type
	unique_ptr<gl::PointSpriteBuffer>     thing_overlay_buffer_;
	unique_ptr<gl::VertexBuffer2D>        thing_light_preview_buffer_;
	unique_ptr<gl::ThingBuffer2D>         temp_things_buffer_;

	// Flats
	struct Flat
	{
		unsigned texture       = 0;
		long     updated_time  = 0;
		unsigned buffer_offset = 0;
		unsigned vertex_count  = 0;
	};
	struct FlatGroup
	{
		unsigned                    texture = 0;
		unique_ptr<gl::IndexBuffer> index_buffer;
	};
	vector<Flat>      flats_;
	vector<FlatGroup> flat_groups_;

	// Other
	bool     lines_dirs_ = false;
	unsigned n_vertices_ = 0;
	unsigned n_lines_    = 0;

	// Thing paths
	enum class PathType
	{
		Normal,
		NormalBoth,
		Dragon,
		DragonBoth
	};
	struct ThingPath
	{
		unsigned from_index;
		unsigned to_index;
		PathType type;
	};
	vector<ThingPath> thing_paths_;
	long              thing_paths_updated_ = 0;

	// Buffer updates
	void updateVerticesBuffer();
	void updateLinesBuffer(bool show_direction);
	void updateFlatsBuffer(bool ceilings);
	void updateThingBuffers();

	// Misc.
	void   updateThingPaths(const vector<MapThing*>& things);
	double scaledRadius(int radius) const;
};
} // namespace slade
