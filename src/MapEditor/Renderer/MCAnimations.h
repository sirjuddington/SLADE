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
	MCAnimation(long start, bool mode_3d = false) : starttime_{ start }, mode_3d_{ mode_3d } {}
	virtual ~MCAnimation() = default;

	bool mode3d() const { return mode_3d_; }

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
	MCASelboxFader(long start, Vec2d tl, Vec2d br);
	~MCASelboxFader() = default;

	bool update(long time) override;
	void draw() override;

private:
	Vec2d tl_;
	Vec2d br_;
	float fade_ = 1.f;
};

// Selection/deselection animation for things
class MCAThingSelection : public MCAnimation
{
public:
	MCAThingSelection(long start, double x, double y, double radius, double scale_inv, bool select = true);
	~MCAThingSelection() = default;

	bool update(long time) override;
	void draw() override;

private:
	double x_      = 0.;
	double y_      = 0.;
	double radius_ = 0.;
	bool   select_ = true;
	float  fade_   = 1.f;
};

// Selection/deselection animation for lines
class MCALineSelection : public MCAnimation
{
public:
	MCALineSelection(long start, const vector<MapLine*>& lines, bool select = true);
	~MCALineSelection() = default;

	bool update(long time) override;
	void draw() override;

private:
	vector<Rectf> lines_;
	vector<Rectf> tabs_;
	bool          select_ = true;
	float         fade_   = 1.f;
};

// Selection/deselection animation for vertices
class MCAVertexSelection : public MCAnimation
{
public:
	MCAVertexSelection(long start, const vector<MapVertex*>& verts, double size, bool select = true);
	~MCAVertexSelection() = default;

	bool update(long time) override;
	void draw() override;

private:
	vector<Vec2d> vertices_;
	double        size_   = 0.;
	bool          select_ = true;
	float         fade_   = 1.f;
};

// Selection/deselection animation for sectors
class MCASectorSelection : public MCAnimation
{
public:
	MCASectorSelection(long start, const vector<Polygon2D*>& polys, bool select = true);
	~MCASectorSelection() = default;

	bool update(long time) override;
	void draw() override;

private:
	vector<Polygon2D*> polygons_;
	bool               select_ = true;
	float              fade_   = 1.f;
};

// Selection/deselection animation for 3d mode walls
class MCA3dWallSelection : public MCAnimation
{
public:
	MCA3dWallSelection(long start, Vec3f points[4], bool select = true);
	~MCA3dWallSelection() = default;

	bool update(long time) override;
	void draw() override;

private:
	Vec3f points_[4];
	bool  select_ = true;
	float fade_   = 1.f;
};

// Selection/deselection animation for 3d mode flats
class MCA3dFlatSelection : public MCAnimation
{
public:
	MCA3dFlatSelection(long start, MapSector* sector, Plane plane, bool select = true);
	~MCA3dFlatSelection() = default;

	bool update(long time) override;
	void draw() override;

private:
	MapSector* sector_ = nullptr;
	Plane      plane_;
	bool       select_ = true;
	float      fade_   = 1.f;
};

// Fading out animation for object hilights
class MCAHilightFade : public MCAnimation
{
public:
	MCAHilightFade(long start, MapObject* object, MapRenderer2D* renderer, float fade_init);
	~MCAHilightFade() = default;

	bool update(long time) override;
	void draw() override;

private:
	MapObject*     object_    = nullptr;
	float          fade_      = 1.f;
	float          init_fade_ = 1.f;
	MapRenderer2D* renderer_  = nullptr;
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
	~MCAHilightFade3D() = default;

	bool update(long time) override;
	void draw() override;

private:
	int                 item_index_ = 0;
	MapEditor::ItemType item_type_;
	float               fade_      = 1.f;
	float               init_fade_ = 1.f;
	MapRenderer3D*      renderer_  = nullptr;
};
