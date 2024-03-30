#pragma once

#include "Geometry/Plane.h"
#include "Geometry/RectFwd.h"

namespace slade
{
// Forward declarations
class Polygon2D;
class MapRenderer2D;
class MapRenderer3D;
namespace gl
{
	enum class Blend;
	enum class PointSpriteType;

	class PointSpriteBuffer;
	class VertexBuffer2D;

	namespace draw2d
	{
		struct Context;
	}
} // namespace gl
namespace mapeditor
{
	enum class ItemType;
}

class MCAnimation
{
public:
	MCAnimation(long start, bool mode_3d = false) : starttime_{ start }, mode_3d_{ mode_3d } {}
	virtual ~MCAnimation() = default;

	bool mode3d() const { return mode_3d_; }

	virtual bool update(long time) { return false; }
	virtual void draw() {}
	virtual void draw(gl::draw2d::Context& dc) {}

protected:
	long starttime_;
	bool mode_3d_;
};

// Fading out animation for selection box ending
class MCASelboxFader : public MCAnimation
{
public:
	MCASelboxFader(long start, const Vec2d& tl, const Vec2d& br);
	~MCASelboxFader() override = default;

	bool update(long time) override;
	void draw(gl::draw2d::Context& dc) override;

private:
	Vec2d tl_;
	Vec2d br_;
	float fade_ = 1.f;
};

// Selection/deselection animation for things
class MCAThingSelection : public MCAnimation
{
public:
	MCAThingSelection(
		long                     start,
		const vector<MapThing*>& things,
		float                    view_scale,
		gl::PointSpriteType      ps_type,
		bool                     select = true);
	~MCAThingSelection() override;

	bool update(long time) override;
	void draw(gl::draw2d::Context& dc) override;

private:
	double              x_      = 0.;
	double              y_      = 0.;
	double              radius_ = 0.;
	bool                select_ = true;
	float               fade_   = 1.f;
	gl::PointSpriteType ps_type_;

	unique_ptr<gl::PointSpriteBuffer> buffer_;

	float scaledRadius(float radius, float view_scale);
};

// Selection/deselection animation for lines
class MCALineSelection : public MCAnimation
{
public:
	MCALineSelection(long start, const vector<MapLine*>& lines, bool select = true);
	~MCALineSelection() override = default;

	bool update(long time) override;
	void draw(gl::draw2d::Context& dc) override;

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
	MCAVertexSelection(long start, const vector<MapVertex*>& verts, float size, bool select = true);
	~MCAVertexSelection() override = default;

	bool update(long time) override;
	void draw(gl::draw2d::Context& dc) override;

private:
	vector<Vec2f> vertices_;
	float         size_   = 0.;
	bool          select_ = true;
	float         fade_   = 1.f;
};

// Selection/deselection animation for sectors
class MCASectorSelection : public MCAnimation
{
public:
	MCASectorSelection(long start, const vector<MapSector*>& sectors, bool select = true);
	~MCASectorSelection() override;

	bool update(long time) override;
	void draw(gl::draw2d::Context& dc) override;

private:
	unique_ptr<gl::VertexBuffer2D> vertex_buffer_;
	bool                           select_ = true;
	float                          fade_   = 1.f;
	glm::vec4                      colour_;
	gl::Blend                      blend_;
};

// Selection/deselection animation for 3d mode walls
class MCA3dWallSelection : public MCAnimation
{
public:
	MCA3dWallSelection(long start, Vec3f points[4], bool select = true);
	~MCA3dWallSelection() override = default;

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
	MCA3dFlatSelection(long start, MapSector* sector, const Plane& plane, bool select = true);
	~MCA3dFlatSelection() override = default;

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
	~MCAHilightFade() override = default;

	bool update(long time) override;
	void draw() override;
	void draw(gl::draw2d::Context& dc) override;

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
		mapeditor::ItemType item_type,
		MapRenderer3D*      renderer,
		float               fade_init);
	~MCAHilightFade3D() override = default;

	bool update(long time) override;
	void draw() override;

private:
	int                 item_index_ = 0;
	mapeditor::ItemType item_type_;
	float               fade_      = 1.f;
	float               init_fade_ = 1.f;
	MapRenderer3D*      renderer_  = nullptr;
};
} // namespace slade
