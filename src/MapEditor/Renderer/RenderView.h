#pragma once

namespace slade::mapeditor
{
class RenderView
{
public:
	RenderView() = default;

	const Vec2d& offset(bool inter = false) const { return inter ? offset_inter_ : offset_; }
	double       scale(bool inter = false) const { return inter ? scale_inter_ : scale_; }
	const Vec2i& size() const { return size_; }
	const Rectd& mapBounds() const { return map_bounds_; }

	void setOffset(double x, double y)
	{
		offset_ = { x, y };
		updateMapBounds();
	}
	void setSize(int width, int height)
	{
		size_ = { width, height };
		updateMapBounds();
	}

	void resetInter(bool x, bool y, bool scale);
	void zoom(double amount);
	void zoomToward(double amount, const Vec2d point);
	void fitTo(BBox bbox);
	bool interpolate(double mult, const Vec2d* towards = nullptr);

	// Map <-> Screen Coordinate Translation
	double mapX(int screen_x, bool inter = false) const;
	double mapY(int screen_y, bool inter = false) const;
	Vec2d  mapPos(const Vec2i& screen_pos, bool inter = false) const;
	int    screenX(double map_x) const;
	int    screenY(double map_y) const;

	void apply() const;
	void setOverlayCoords(bool set) const;

private:
	Vec2d  offset_;
	Vec2d  offset_inter_;
	double scale_       = 0.;
	double scale_inter_ = 0.;
	double min_scale_   = 0.005;
	double max_scale_   = 10.;
	Vec2i  size_;
	Rectd  map_bounds_;

	void updateMapBounds();
};
} // namespace slade::mapeditor
