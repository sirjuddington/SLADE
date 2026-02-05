#pragma once

#include "Geometry/Plane.h"
#include "Geometry/Rect.h"
#include "MapEditor/Item.h"
#include "SelectionOverlay3D.h"

namespace slade
{
// Forward declarations
class Polygon2D;
namespace gl
{
	class View;
	class Camera;
	class IndexBuffer;
	class LineBuffer;
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
	class MapGeometryBuffer3D;
	class MapEditContext;
	class MapRenderer2D;
	class MapRenderer3D;
} // namespace mapeditor

class MCAnimation
{
public:
	MCAnimation(long start, bool mode_3d = false) : starttime_{ start }, mode_3d_{ mode_3d } {}
	virtual ~MCAnimation() = default;

	bool mode3d() const { return mode_3d_; }

	virtual bool update(long time) { return false; }
	virtual void draw(mapeditor::MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view) {}
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
	void draw(mapeditor::MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view) override;

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
	void draw(mapeditor::MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view) override;

private:
	MapSector* sector_ = nullptr;
	Plane      plane_;
	bool       select_ = true;
	float      fade_   = 1.f;
};

class MCA3dSelection : public MCAnimation
{
public:
	MCA3dSelection(const vector<mapeditor::Item>& items, mapeditor::MapRenderer3D& renderer, bool select = true);

	bool update(long time) override;
	void draw(mapeditor::MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view) override;

private:
	mapeditor::SelectionOverlay3D overlay_;
	bool                          select_ = true;
	float                         fade_   = 1.f;
	glm::vec4                     colour_;
	bool                          additive_ = false;
};

// Fading out animation for object hilights
class MCAHilightFade : public MCAnimation
{
public:
	MCAHilightFade(long start, MapObject* object, mapeditor::MapRenderer2D* renderer, float fade_init);
	~MCAHilightFade() override = default;

	bool update(long time) override;
	void draw(gl::draw2d::Context& dc) override;

private:
	MapObject*                object_    = nullptr;
	float                     fade_      = 1.f;
	float                     init_fade_ = 1.f;
	mapeditor::MapRenderer2D* renderer_  = nullptr;
};

// Fading out animation for 3d mode wall/flat/thing hilights
class MCAHilightFade3D : public MCAnimation
{
public:
	MCAHilightFade3D(long start, const mapeditor::Item& item, float fade_init);
	~MCAHilightFade3D() override = default;

	bool update(long time) override;
	void draw(mapeditor::MapRenderer3D& renderer, const gl::Camera& camera, const gl::View& view) override;

private:
	mapeditor::Item item_;
	float           fade_      = 1.f;
	float           init_fade_ = 1.f;
};
} // namespace slade
