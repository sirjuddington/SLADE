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
		SKY = 4,

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

		Quad()
		{
			colour.set(255, 255, 255, 255, 0);
			texture = nullptr;
			flags   = 0;
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
		float      alpha;
		MapSector* sector;
		long       updated_time;

		Flat()
		{
			light        = 255;
			texture      = nullptr;
			updated_time = 0;
			flags        = 0;
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
	void updateFlatTexCoords(unsigned index, bool floor);
	void updateSector(unsigned index);
	void renderFlat(Flat* flat);
	void renderFlats();
	void renderFlatSelection(const ItemSelection& selection, float alpha = 1.0f);

	// Walls
	void setupQuad(Quad* quad, double x1, double y1, double x2, double y2, double top, double bottom);
	void setupQuad(Quad* quad, double x1, double y1, double x2, double y2, Plane top, Plane bottom);
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
	void onAnnouncement(Announcer* announcer, const string& event_name, MemChunk& event_data) override;

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
	vector<Line>  lines_;
	Quad**        quads_;
	vector<Quad*> quads_transparent_;
	vector<Thing> things_;
	vector<Flat>  floors_;
	vector<Flat>  ceilings_;
	Flat**        flats_;

	// VBOs
	unsigned vbo_floors_;
	unsigned vbo_ceilings_;
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
