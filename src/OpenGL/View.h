#pragma once

#include "Geometry/Rect.h"

namespace slade
{
struct BBox;
}

namespace slade::gl
{
class View
{
public:
	View() = default;
	View(bool y_flipped) : y_flipped_{ y_flipped } {}

	const Vec2d& offset(bool inter = false) const { return inter ? offset_inter_ : offset_; }
	double       scale(bool inter = false) const { return inter ? scale_inter_ : scale_; }
	const Vec2i& size() const { return size_; }
	const Rectd& visibleRegion() const { return visible_region_; }
	bool         yFlipped() const { return y_flipped_; }

	void setOffset(double x, double y)
	{
		offset_ = { x, y };
		updateVisibleRegion();
	}
	void setScale(double scale)
	{
		scale_ = scale;
		updateVisibleRegion();
	}
	void setSize(int width, int height)
	{
		size_ = { width, height };
		updateVisibleRegion();
	}
	void flipY(bool flip) { y_flipped_ = flip; }

	void resetInter(bool x, bool y, bool scale);
	void pan(double x, double y);
	void zoom(double amount);
	void zoomToward(double amount, const Vec2i& point);
	void fitTo(const BBox& bbox);
	bool interpolate(double mult, const Vec2d* towards = nullptr);

	// Canvas <-> Screen Coordinate Translation
	double canvasX(int screen_x, bool inter = false) const;
	double canvasY(int screen_y, bool inter = false) const;
	Vec2d  canvasPos(const Vec2i& screen_pos, bool inter = false) const;
	int    screenX(double canvas_x) const;
	int    screenY(double canvas_y) const;

	void apply(bool init = true) const;
	void setOverlayCoords(bool set) const;

private:
	Vec2d  offset_         = {};
	Vec2d  offset_inter_   = {};
	bool   y_flipped_      = false; // If true, the Y-axis is flipped (ie. bottom is 0)
	double scale_          = 0.;
	double scale_inter_    = 0.;
	double min_scale_      = 0.005;
	double max_scale_      = 10.;
	Vec2i  size_           = {};
	Rectd  visible_region_ = {};

	void updateVisibleRegion();
};
} // namespace slade::gl
