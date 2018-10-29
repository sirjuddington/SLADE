#pragma once

#include "General/ListenerAnnouncer.h"
#include "MapEditor/Edit/Edit3D.h"
#include "MapEditor/SLADEMap/SLADEMap.h"

class ItemSelection;
class GLTexture;
class Polygon2D;

namespace Game
{
class ThingType;
}

class MapRenderer3D : public Listener
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
		CEIL = 8,


		// Thing flags
		ICON  = 4,
		DRAWN = 8,
		ZETH  = 16,
	};
	struct GLVertex
	{
		float x, y, z;
		float tx, ty;
	};
	struct Quad
	{
		GLVertex   points[4];
		ColRGBA    colour;
		ColRGBA    fogcolour;
		uint8_t    light;
		GLTexture* texture;
		uint8_t    flags;
		float      alpha;
		int        control_line;
		int        control_side;

		Quad()
		{
			colour.set(255, 255, 255, 255, 0);
			texture      = nullptr;
			flags        = 0;
			control_line = -1;
			control_side = -1;
		}
	};
	struct Line
	{
		vector<Quad> quads;
		long         updated_time;
		bool         visible;
		MapLine*     line;

		Line()
		{
			updated_time = 0;
			visible      = true;
		}
	};
	struct Thing
	{
		uint8_t                flags;
		Game::ThingType const* type;
		MapSector*             sector;
		float                  z;
		float                  height;
		GLTexture*             sprite;
		long                   updated_time;

		Thing()
		{
			flags        = 0;
			type         = nullptr;
			sector       = nullptr;
			sprite       = nullptr;
			z            = 0;
			updated_time = 0;
		}
	};
	struct Flat
	{
		uint8_t    flags;
		uint8_t    light;
		ColRGBA    colour;
		ColRGBA    fogcolour;
		GLTexture* texture;
		Plane      plane;
		float      base_alpha;
		float      alpha;
		MapSector* sector;
		MapSector* control_sector;
		int        extra_floor_index;
		long       updated_time;
		unsigned   vbo_offset;

		Flat()
		{
			light        = 255;
			texture      = nullptr;
			updated_time = 0;
			flags        = 0;
			base_alpha   = 1.0f;
			alpha        = 1.0f;
			sector       = nullptr;
		}
	};

	MapRenderer3D(SLADEMap* map = nullptr);
	~MapRenderer3D();

	bool fullbrightEnabled() { return fullbright_; }
	bool fogEnabled() { return fog_; }
	void enableFullbright(bool enable = true) { fullbright_ = enable; }
	void enableFog(bool enable = true) { fog_ = enable; }
	int  itemDistance() { return item_dist_; }
	void enableHilight(bool render) { render_hilight_ = render; }
	void enableSelection(bool render) { render_selection_ = render; }

	bool init();
	void refresh();
	void clearData();
	void buildSkyCircle();

	Quad* getQuad(MapEditor::Item item);
	Flat* getFlat(MapEditor::Item item);
	vector<vector<Flat>>* getSectorFlats() { return &sector_flats_; };

	// Camera
	void cameraMove(double distance, bool z = true);
	void cameraTurn(double angle);
	void cameraMoveUp(double distance);
	void cameraStrafe(double distance);
	void cameraPitch(double amount);
	void cameraUpdateVectors();
	void cameraSet(Vec3f position, Vec2f direction);
	void cameraSetPosition(Vec3f position);
	void cameraApplyGravity(double mult);
	void cameraLook(double xrel, double yrel);

	double camPitch() { return cam_pitch_; }
	Vec3f  camPosition() { return cam_position_; }
	Vec2f  camDirection() { return cam_direction_; }

	// -- Rendering --
	void setupView(int width, int height);
	void setLight(ColRGBA& colour, uint8_t light, float alpha = 1.0f);
	void setFog(ColRGBA& fogcol, uint8_t light);
	void renderMap();
	void renderSkySlice(
		float top,
		float bottom,
		float atop,
		float abottom,
		float size,
		float tx = 0.125f,
		float ty = 2.0f);
	void renderSky();

	// Flats
	void updateFlatTexCoords(unsigned index, unsigned flat_index);
	void updateSector(unsigned index);
	void updateSectorFlats(unsigned index);
	void updateSectorVBOs(unsigned index);
	bool isSectorStale(unsigned index);
	void renderFlat(Flat* flat);
	void renderFlats();
	void renderFlatSelection(const ItemSelection& selection, float alpha = 1.0f);

	// Walls
	void setupQuad(Quad* quad, Seg2f seg, double top, double bottom);
	void setupQuad(Quad* quad, Seg2f seg, Plane top, Plane bottom);
	void setupQuadTexCoords(
		Quad*  quad,
		int    length,
		double o_left,
		double o_top,
		double h_top,
		double h_bottom,
		bool   pegbottom = false,
		double sx        = 1,
		double sy        = 1);
	void updateLine(unsigned index);
	void renderQuad(Quad* quad, float alpha = 1.0f);
	void renderWalls();
	void renderTransparentWalls();
	void renderWallSelection(const ItemSelection& selection, float alpha = 1.0f);

	// Things
	void updateThing(unsigned index, MapThing* thing);
	void renderThings();
	void renderThingSelection(const ItemSelection& selection, float alpha = 1.0f);

	// VBO stuff
	void updateFlatsVBO();
	void updateWallsVBO();

	// Visibility checking
	void  quickVisDiscard();
	float calcDistFade(double distance, double max = -1);
	void  checkVisibleQuads();
	void  checkVisibleFlats();

	// Hilight
	MapEditor::Item determineHilight();
	void            renderHilight(MapEditor::Item hilight, float alpha = 1.0f);

	// Listener stuff
	void onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

private:
	SLADEMap*  map_;
	bool       fullbright_;
	bool       fog_;
	GLTexture* tex_last_;
	unsigned   n_quads_;
	unsigned   n_flats_;
	int        flat_last_;
	bool       render_hilight_;
	bool       render_selection_;
	ColRGBA    fog_colour_last_;
	float      fog_depth_last_;

	// Visibility
	vector<float> dist_sectors_;

	// Camera
	Vec3f  cam_position_;
	Vec2f  cam_direction_;
	double cam_pitch_;
	double cam_angle_;
	Vec3f  cam_dir3d_;
	Vec3f  cam_strafe_;
	double gravity_;
	int    item_dist_;

	// Map Structures
	vector<Line>          lines_;
	Quad**                quads_;
	vector<Quad*>         quads_transparent_;
	vector<Thing>         things_;
	vector<vector<Flat>>  sector_flats_;
	Flat**                flats_;

	// VBOs
	unsigned vbo_flats_;
	unsigned vbo_walls_;

	// Sky
	struct GLVertexEx
	{
		float x, y, z;
		float tx, ty;
		float alpha;
	};
	string  skytex1_;
	string  skytex2_;
	ColRGBA skycol_top_;
	ColRGBA skycol_bottom_;
	Vec2f   sky_circle_[32];
};
