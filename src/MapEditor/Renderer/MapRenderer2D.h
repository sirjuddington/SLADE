#pragma once

#include "MapEditor/MapEditor.h"

// Forward declarations
class GLTexture;
class ItemSelection;
class MapLine;
class MapSector;
class MapThing;
class ObjectEditGroup;
class SLADEMap;
namespace Game
{
class ThingType;
}

class MapRenderer2D
{
public:
	MapRenderer2D(SLADEMap* map);
	~MapRenderer2D();

	double viewScaleInv() { return view_scale_inv_; }

	// Vertices
	bool setupVertexRendering(float size_scale, bool overlay = false);
	void renderVertices(float alpha = 1.0f);
	void renderVerticesVBO();
	void renderVerticesImmediate();
	void renderVertexHilight(int index, float fade);
	void renderVertexSelection(const ItemSelection& selection, float fade = 1.0f);

	// Lines
	ColRGBA lineColour(MapLine* line, bool ignore_filter = false);
	void    renderLines(bool show_direction, float alpha = 1.0f);
	void    renderLinesVBO(bool show_direction, float alpha);
	void    renderLinesImmediate(bool show_direction, float alpha);
	void    renderLineHilight(int index, float fade);
	void    renderLineSelection(const ItemSelection& selection, float fade = 1.0f);
	void    renderTaggedLines(vector<MapLine*>& lines, float fade);
	void    renderTaggingLines(vector<MapLine*>& lines, float fade);

	// Things
	enum ThingDrawType
	{
		Square,
		Round,
		Sprite,
		SquareSprite,
		FramedSprite,
	};
	bool setupThingOverlay();
	void renderThingOverlay(double x, double y, double radius, bool point);
	void renderRoundThing(
		double                 x,
		double                 y,
		double                 angle,
		const Game::ThingType& type,
		float                  alpha       = 1.0f,
		double                 radius_mult = 1.0);
	bool renderSpriteThing(
		double                 x,
		double                 y,
		double                 angle,
		const Game::ThingType& type,
		unsigned               index,
		float                  alpha     = 1.0f,
		bool                   fitradius = false);
	void renderSimpleSquareThing(double x, double y, double angle, const Game::ThingType& type, float alpha = 1.0f);
	bool renderSquareThing(
		double                 x,
		double                 y,
		double                 angle,
		const Game::ThingType& type,
		float                  alpha    = 1.0f,
		bool                   showicon = true,
		bool                   framed   = false);
	void renderThings(float alpha = 1.0f, bool force_dir = false);
	void renderThingsImmediate(float alpha);
	void renderThingHilight(int index, float fade);
	void renderThingSelection(const ItemSelection& selection, float fade = 1.0f);
	void renderTaggedThings(vector<MapThing*>& things, float fade);
	void renderTaggingThings(vector<MapThing*>& things, float fade);
	void renderPathedThings(vector<MapThing*>& things);

	// Flats (sectors)
	void renderFlats(int type = 0, bool texture = true, float alpha = 1.0f);
	void renderFlatsImmediate(int type, bool texture, float alpha);
	void renderFlatsVBO(int type, bool texture, float alpha);
	void renderFlatHilight(int index, float fade);
	void renderFlatSelection(const ItemSelection& selection, float fade = 1.0f);
	void renderTaggedFlats(vector<MapSector*>& sectors, float fade);

	// Moving
	void renderMovingVertices(const vector<MapEditor::Item>& vertices, Vec2f move_vec);
	void renderMovingLines(const vector<MapEditor::Item>& lines, Vec2f move_vec);
	void renderMovingSectors(const vector<MapEditor::Item>& sectors, Vec2f move_vec);
	void renderMovingThings(const vector<MapEditor::Item>& things, Vec2f move_vec);

	// Paste
	void renderPasteThings(vector<MapThing*>& things, Vec2f pos);

	// Object Edit
	void renderObjectEditGroup(ObjectEditGroup* group);

	// VBOs
	void updateVerticesVBO();
	void updateLinesVBO(bool show_direction, float alpha);
	void updateFlatsVBO();

	// Misc
	void setScale(double scale)
	{
		view_scale_     = scale;
		view_scale_inv_ = 1.0 / scale;
	}
	void   updateVisibility(Vec2f view_tl, Vec2f view_br);
	void   forceUpdate(float line_alpha = 1.0f);
	double scaledRadius(int radius);
	bool   visOK();
	void   clearTextureCache() { tex_flats_.clear(); }

private:
	SLADEMap*  map_;
	GLTexture* tex_last_;
	long       vertices_updated_;
	long       lines_updated_;
	long       flats_updated_;

	// VBOs etc
	unsigned         vbo_vertices_;
	unsigned         vbo_lines_;
	unsigned         vbo_flats_;
	vector<unsigned> sector_vbo_offsets_;

	// Display lists
	unsigned list_vertices_;
	unsigned list_lines_;

	// Visibility
	enum
	{
		VIS_LEFT  = 1,
		VIS_RIGHT = 2,
		VIS_ABOVE = 4,
		VIS_BELOW = 8,
		VIS_SMALL = 16,
	};
	vector<uint8_t> vis_v_;
	vector<uint8_t> vis_l_;
	vector<uint8_t> vis_t_;
	vector<uint8_t> vis_s_;

	// Structs
	struct GLVert
	{
		float x, y;
		float r, g, b, a;
	};
	struct GLLine
	{
		GLVert v1, v2;   // The line itself
		GLVert dv1, dv2; // Direction tab
	};

	// Other
	bool   lines_dirs_;
	int    n_vertices_;
	int    n_lines_;
	int    n_things_;
	double view_scale_;
	double view_scale_inv_;
	bool   things_angles_;

	vector<GLTexture*> tex_flats_;
	int                last_flat_type_;
	vector<GLTexture*> thing_sprites_;
	long               thing_sprites_updated_;

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
	long              thing_paths_updated_;
};
