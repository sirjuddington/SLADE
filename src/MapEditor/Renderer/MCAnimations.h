#pragma once

#include "MapEditor/Edit/Edit3D.h"

class MapLine;
class MapVertex;
class Polygon2D;
class MapSector;
class MapRenderer2D;
class MapObject;
class MapRenderer3D;

class MCAnimation
{
public:
	MCAnimation(long start, bool mode_3d = false)
	{
		starttime_     = start;
		this->mode_3d_ = mode_3d;
	}
	virtual ~MCAnimation() {}

	bool mode3d() { return mode_3d_; }

	virtual bool update(long time) { return false; }
	virtual void draw() {}

protected:
	long starttime_;
	bool mode_3d_;
};

// Fading out animation for selection box ending
class MCASelboxFader : public MCAnimation
{
public:
	MCASelboxFader(long start, fpoint2_t tl, fpoint2_t br);
	~MCASelboxFader();

	bool update(long time);
	void draw();

private:
	fpoint2_t tl_;
	fpoint2_t br_;
	float     fade_;
};

// Selection/deselection animation for things
class MCAThingSelection : public MCAnimation
{
public:
	MCAThingSelection(long start, double x, double y, double radius, double scale_inv, bool select = true);
	~MCAThingSelection();

	bool update(long time);
	void draw();

private:
	double x_;
	double y_;
	double radius_;
	bool   select_;
	float  fade_;
};

// Selection/deselection animation for lines
class MCALineSelection : public MCAnimation
{
public:
	MCALineSelection(long start, vector<MapLine*>& lines, bool select = true);
	~MCALineSelection();

	bool update(long time);
	void draw();

private:
	vector<frect_t> lines_;
	vector<frect_t> tabs_;
	bool            select_;
	float           fade_;
};

// Selection/deselection animation for vertices
class MCAVertexSelection : public MCAnimation
{
public:
	MCAVertexSelection(long start, vector<MapVertex*>& verts, double size, bool select = true);
	~MCAVertexSelection();

	bool update(long time);
	void draw();

private:
	vector<fpoint2_t> vertices_;
	double            size_;
	bool              select_;
	float             fade_;
};

// Selection/deselection animation for sectors
class MCASectorSelection : public MCAnimation
{
public:
	MCASectorSelection(long start, vector<Polygon2D*>& polys, bool select = true);
	~MCASectorSelection();

	bool update(long time);
	void draw();

private:
	vector<Polygon2D*> polygons_;
	bool               select_;
	float              fade_;
};

// Selection/deselection animation for 3d mode walls
class MCA3dWallSelection : public MCAnimation
{
public:
	MCA3dWallSelection(long start, fpoint3_t points[4], bool select = true);
	~MCA3dWallSelection();

	bool update(long time);
	void draw();

private:
	fpoint3_t points_[4];
	bool      select_;
	float     fade_;
};

// Selection/deselection animation for 3d mode flats
class MCA3dFlatSelection : public MCAnimation
{
public:
	MCA3dFlatSelection(long start, MapSector* sector, plane_t plane, bool select = true);
	~MCA3dFlatSelection();

	bool update(long time);
	void draw();

private:
	MapSector* sector_;
	plane_t    plane_;
	bool       select_;
	float      fade_;
};

// Fading out animation for object hilights
class MCAHilightFade : public MCAnimation
{
public:
	MCAHilightFade(long start, MapObject* object, MapRenderer2D* renderer, float fade_init);
	~MCAHilightFade();

	bool update(long time);
	void draw();

private:
	MapObject*     object_;
	float          fade_;
	float          init_fade_;
	MapRenderer2D* renderer_;
};

// Fading out animation for 3d mode wall/flat/thing hilights
class MCAHilightFade3D : public MCAnimation
{
public:
	MCAHilightFade3D(
		long                start,
		int                 item_index,
		MapEditor::ItemType item_type,
		MapRenderer3D*      renderer,
		float               fade_init);
	~MCAHilightFade3D();

	bool update(long time);
	void draw();

private:
	int                 item_index_;
	MapEditor::ItemType item_type_;
	float               fade_;
	float               init_fade_;
	MapRenderer3D*      renderer_;
};
