#pragma once

#include "Geometry/Plane.h"
#include "Geometry/RectFwd.h"
#include "MapEditor/Item.h"
#include "Utility/ColRGBA.h"

namespace slade
{
class ItemSelection;
class Polygon2D;

namespace game
{
	class ThingType;
}

class MapRenderer3D
{
public:
	// Structs
	enum
	{
		// Common flags
		TRANSADD = 2,

		// Quad/flat flags
		SKY      = 4,
		DRAWBOTH = 128,

		// Quad flags
		BACK   = 8,
		UPPER  = 16,
		LOWER  = 32,
		MIDTEX = 64,

		// Flat flags
		CEIL     = 8,
		FLATFLIP = 16,

		// Thing flags
		ICON  = 4,
		DRAWN = 8,
		ZETH  = 16,
	};
	struct GLVertex
	{
		float x = 0.f, y = 0.f, z = 0.f;
		float tx = 0.f, ty = 0.f;
	};
	struct Quad
	{
		GLVertex points[4] = { {}, {}, {}, {} };
		ColRGBA  colour;
		ColRGBA  fogcolour;
		uint8_t  light        = 0;
		unsigned texture      = 0;
		uint8_t  flags        = 0;
		float    alpha        = 1.f;
		int      control_line = -1;
		int      control_side = -1;

		Quad() : colour{ 255, 255, 255, 255, 0 } {}
	};
	struct Line
	{
		vector<Quad> quads;
		long         updated_time = 0;
		bool         visible      = true;
		MapLine*     line         = nullptr;
	};
	struct Thing
	{
		uint8_t                flags        = 0;
		game::ThingType const* type         = nullptr;
		MapSector*             sector       = nullptr;
		float                  z            = 0.f;
		float                  height       = 0.f;
		unsigned               sprite       = 0;
		long                   updated_time = 0;
	};
	struct Flat
	{
		uint8_t    flags = 0;
		uint8_t    light = 255;
		ColRGBA    colour;
		ColRGBA    fogcolour;
		unsigned   texture = 0;
		Vec2d      scale;
		Plane      plane;
		float      base_alpha        = 1.f;
		float      alpha             = 1.f;
		MapSector* sector            = nullptr;
		MapSector* control_sector    = nullptr;
		int        extra_floor_index = -1;
		long       updated_time      = 0;
		unsigned   vbo_offset        = 0;
	};

	MapRenderer3D(SLADEMap* map = nullptr);
	~MapRenderer3D();

	bool fullbrightEnabled() const { return fullbright_; }
	bool fogEnabled() const { return fog_; }
	void enableFullbright(bool enable = true) { fullbright_ = enable; }
	void enableFog(bool enable = true) { fog_ = enable; }
	int  itemDistance() const { return item_dist_; }
	void enableHilight(bool render) { render_hilight_ = render; }
	void enableSelection(bool render) { render_selection_ = render; }

	bool init();
	void refresh();
	void refreshTextures();
	void clearData();
	void buildSkyCircle();

	Quad*                 getQuad(mapeditor::Item item);
	Flat*                 getFlat(mapeditor::Item item);
	vector<vector<Flat>>& getSectorFlats() { return sector_flats_; }

	// Camera
	void cameraMove(double distance, bool z = true);
	void cameraTurn(double angle);
	void cameraMoveUp(double distance);
	void cameraStrafe(double distance);
	void cameraPitch(double amount);
	void cameraUpdateVectors();
	void cameraSet(const Vec3d& position, const Vec2d& direction);
	void cameraSetPosition(const Vec3d& position);
	void cameraApplyGravity(double mult);
	void cameraLook(double xrel, double yrel);

	double camPitch() const { return cam_pitch_; }
	Vec3d  camPosition() const { return cam_position_; }
	Vec2d  camDirection() const { return cam_direction_; }

	// -- Rendering --
	void setupView(int width, int height) const;
	void setLight(const ColRGBA& colour, uint8_t light, float alpha = 1.0f) const;
	void setFog(const ColRGBA& fogcol, uint8_t light);
	void renderMap();
	void renderSkySlice(
		float top,
		float bottom,
		float atop,
		float abottom,
		float size,
		float tx = 0.125f,
		float ty = 2.0f) const;
	void renderSky();

	// Flats
	void updateFlatTexCoords(unsigned index, unsigned flat_index) const;
	void updateSector(unsigned index);
	void updateSectorFlats(unsigned index);
	void updateSectorVBOs(unsigned index) const;
	bool isSectorStale(unsigned index) const;
	void renderFlat(const Flat* flat);
	void renderFlats();
	void renderFlatSelection(const ItemSelection& selection, float alpha = 1.0f) const;

	// Walls
	void setupQuad(Quad* quad, const Seg2d& seg, double top, double bottom) const;
	void setupQuad(Quad* quad, const Seg2d& seg, const Plane& top, const Plane& bottom) const;
	void setupQuadTexCoords(
		Quad*  quad,
		int    length,
		double o_left,
		double o_top,
		double h_top,
		double h_bottom,
		bool   pegbottom = false,
		double sx        = 1,
		double sy        = 1) const;
	void updateLine(unsigned index);
	void renderQuad(const Quad* quad, float alpha = 1.0f);
	void renderWalls();
	void renderTransparentWalls();
	void renderWallSelection(const ItemSelection& selection, float alpha = 1.0f);

	// Things
	void updateThing(unsigned index, const MapThing* thing);
	void renderThings();
	void renderThingSelection(const ItemSelection& selection, float alpha = 1.0f);

	// VBO stuff
	void updateFlatsVBO();
	void updateWallsVBO() const;

	// Visibility checking
	void  quickVisDiscard();
	float calcDistFade(double distance, double max = -1) const;
	void  checkVisibleQuads();
	void  checkVisibleFlats();

	// Hilight
	mapeditor::Item determineHilight();
	void            renderHilight(mapeditor::Item hilight, float alpha = 1.0f);

private:
	SLADEMap* map_;
	bool      fullbright_       = false;
	bool      fog_              = true;
	unsigned  n_quads_          = 0;
	unsigned  n_flats_          = 0;
	int       flat_last_        = 0;
	bool      render_hilight_   = true;
	bool      render_selection_ = true;
	ColRGBA   fog_colour_last_;
	float     fog_depth_last_ = 0.f;

	// Visibility
	vector<float> dist_sectors_;

	// Camera
	Vec3d  cam_position_;
	Vec2d  cam_direction_;
	double cam_pitch_ = 0.;
	Vec3d  cam_dir3d_;
	Vec3d  cam_strafe_;
	int    item_dist_ = 0;

	// Map Structures
	vector<Line>         lines_;
	vector<Quad*>        quads_;
	vector<Quad*>        quads_transparent_;
	vector<Thing>        things_;
	vector<vector<Flat>> sector_flats_;
	vector<Flat*>        flats_;

	// VBOs
	unsigned vbo_flats_ = 0;
	unsigned vbo_walls_ = 0;

	// Sky
	struct GLVertexEx
	{
		float x = 0.f, y = 0.f, z = 0.f;
		float tx = 0.f, ty = 0.f;
		float alpha = 1.f;
	};
	string  skytex1_ = "SKY1";
	string  skytex2_;
	ColRGBA skycol_top_;
	ColRGBA skycol_bottom_;
	Vec2d   sky_circle_[32];

	// Signal connections
	sigslot::scoped_connection sc_resources_updated_;
	sigslot::scoped_connection sc_palette_changed_;
};
} // namespace slade
