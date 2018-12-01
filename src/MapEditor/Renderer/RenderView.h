#pragma once

namespace MapEditor
{
class RenderView
{
public:
	RenderView();

	const Vec2f& offset(bool inter = false) const { return inter ? offset_inter_ : offset_; }
	double           scale(bool inter = false) const { return inter ? scale_inter_ : scale_; }
	const Vec2i&  size() const { return size_; }
	const Rectf&   mapBounds() const { return map_bounds_; }

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
	void zoomToward(double amount, const Vec2f point);
	void fitTo(BBox bbox);
	bool interpolate(double mult, const Vec2f* towards = nullptr);

	// Map <-> Screen Coordinate Translation
	double    mapX(double screen_x, bool inter = false) const;
	double    mapY(double screen_y, bool inter = false) const;
	Vec2f mapPos(const Vec2f& screen_pos, bool inter = false) const;
	int       screenX(double map_x) const;
	int       screenY(double map_y) const;

	void apply() const;
	void setOverlayCoords(bool set) const;

private:
	Vec2f offset_;
	Vec2f offset_inter_;
	double    scale_;
	double    scale_inter_;
	double    min_scale_;
	double    max_scale_;
	Vec2i  size_;
	Rectf   map_bounds_;

	void updateMapBounds();
};
} // namespace MapEditor
