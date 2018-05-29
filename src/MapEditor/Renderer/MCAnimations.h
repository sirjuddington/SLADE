
#ifndef __MC_ANIMATIONS_H__
#define __MC_ANIMATIONS_H__

#include "MapEditor/Edit/Edit3D.h"

class MCAnimation
{
protected:
	long	starttime;
	bool	mode_3d;

public:
	MCAnimation(long start, bool mode_3d = false) { starttime = start; this->mode_3d = mode_3d; }
	virtual ~MCAnimation() {}

	bool	mode3d() { return mode_3d; }

	virtual bool update(long time) { return false; }
	virtual void draw() {}
};

class MCASelboxFader : public MCAnimation
{
private:
	fpoint2_t	tl;
	fpoint2_t	br;
	float		fade;

public:
	MCASelboxFader(long start, fpoint2_t tl, fpoint2_t br);
	~MCASelboxFader();

	bool update(long time);
	void draw();
};

class MCAThingSelection : public MCAnimation
{
private:
	double	x;
	double	y;
	double	radius;
	bool	select;
	float	fade;

public:
	MCAThingSelection(long start, double x, double y, double radius, double scale_inv, bool select = true);
	~MCAThingSelection();

	bool update(long time);
	void draw();
};

class MapLine;
class MCALineSelection : public MCAnimation
{
private:
	vector<frect_t>	lines;
	vector<frect_t>	tabs;
	bool	select;
	float	fade;

public:
	MCALineSelection(long start, vector<MapLine*>& lines, bool select = true);
	~MCALineSelection();

	bool update(long time);
	void draw();
};

class MapVertex;
class MCAVertexSelection : public MCAnimation
{
private:
	vector<fpoint2_t>	vertices;
	double				size;
	bool				select;
	float				fade;

public:
	MCAVertexSelection(long start, vector<MapVertex*>& verts, double size, bool select = true);
	~MCAVertexSelection();

	bool update(long time);
	void draw();
};

class Polygon2D;
class MCASectorSelection : public MCAnimation
{
private:
	vector<Polygon2D*>	polygons;
	bool				select;
	float				fade;

public:
	MCASectorSelection(long start, vector<Polygon2D*>& polys, bool select = true);
	~MCASectorSelection();

	bool update(long time);
	void draw();
};

class MCA3dWallSelection : public MCAnimation
{
private:
	fpoint3_t	points[4];
	bool		select;
	float		fade;

public:
	MCA3dWallSelection(long start, fpoint3_t points[4], bool select = true);
	~MCA3dWallSelection();

	bool update(long time);
	void draw();
};

class MapSector;
class MCA3dFlatSelection : public MCAnimation
{
private:
	MapSector*	sector;
	plane_t		plane;
	bool		select;
	float		fade;

public:
	MCA3dFlatSelection(long start, MapSector* sector, plane_t plane, bool select = true);
	~MCA3dFlatSelection();

	bool update(long time);
	void draw();
};

class MapRenderer2D;
class MapObject;
class MCAHilightFade : public MCAnimation
{
private:
	MapObject*		object;
	float			fade;
	float			init_fade;
	MapRenderer2D*	renderer;

public:
	MCAHilightFade(long start, MapObject* object, MapRenderer2D* renderer, float fade_init);
	~MCAHilightFade();

	bool update(long time);
	void draw();
};

class MapRenderer3D;
class MCAHilightFade3D : public MCAnimation
{
private:
	int						item_index;
	MapEditor::ItemType	item_type;
	float					fade;
	float					init_fade;
	MapRenderer3D*			renderer;

public:
	MCAHilightFade3D(
		long start,
		int item_index,
		MapEditor::ItemType item_type,
		MapRenderer3D* renderer,
		float fade_init
	);
	~MCAHilightFade3D();

	bool update(long time);
	void draw();
};

#endif//__MC_ANIMATIONS_H__
