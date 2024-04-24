#pragma once

#include "Geometry/Rect.h"

namespace slade
{
struct BBox;
}

namespace slade::gl
{
class Shader;

class View
{
public:
	View() = default;
	View(bool centered, bool y_flipped, bool interpolated = true) :
		y_flipped_{ y_flipped },
		centered_{ centered },
		interpolated_{ interpolated }
	{
	}

	const Vec2d& offset(bool inter = false) const { return inter ? offset_inter_ : offset_; }
	const Vec2d& scale(bool inter = false) const { return inter ? scale_inter_ : scale_; }
	const Vec2i& size() const { return size_; }
	const Rectd& visibleRegion() const { return visible_region_; }
	bool         centered() const { return centered_; }
	bool         yFlipped() const { return y_flipped_; }
	bool         interpolated() const { return interpolated_; }
	glm::mat4    projectionMatrix() const { return projection_matrix_; }
	glm::mat4    projectionMatrix(bool y_flipped) const;
	glm::mat4    viewMatrix() const { return view_matrix_; }
	glm::mat4    mvpMatrix(const glm::mat4& model) const { return projection_matrix_ * view_matrix_ * model; }

	void flipY(bool flip)
	{
		y_flipped_ = flip;
		updateMatrices();
	}
	void setInterpolated(bool interpolated)
	{
		interpolated_ = interpolated;
		if (!interpolated)
			resetInter();
	}
	void setCentered(bool centered)
	{
		centered_ = centered;
		updateVisibleRegion();
		updateMatrices();
	}
	void setOffset(double x, double y)
	{
		offset_ = { x, y };

		if (!interpolated_)
			offset_inter_ = offset_;

		updateVisibleRegion();
		updateMatrices();
	}
	void setScale(const Vec2d& scale)
	{
		scale_ = scale;

		if (!interpolated_)
			scale_inter_ = scale_;

		updateVisibleRegion();
		updateMatrices();
	}
	void setScale(const Vec2d& scale, const Vec2i& focus_point);
	void setScale(double scale) { setScale({ scale, scale }); }
	void setScale(double scale, const Vec2i& focus_point) { setScale({ scale, scale }, focus_point); }
	void setSize(int width, int height)
	{
		size_ = { width, height };
		updateVisibleRegion();
		updateMatrices();
	}

	void resetInter(bool x = true, bool y = true, bool scale = true);
	void pan(double x, double y);
	void zoom(double amount);
	void zoomToward(double amount, const Vec2i& point);
	void fitTo(const BBox& bbox, double scale_inc = 1.25);
	bool interpolate(double mult, const Vec2d* towards = nullptr);

	// Canvas <-> Screen Coordinate Translation
	double canvasX(int screen_x) const;
	double canvasY(int screen_y) const;
	Vec2d  canvasPos(const Vec2i& screen_pos) const { return { canvasX(screen_pos.x), canvasY(screen_pos.y) }; }
	int    screenX(double canvas_x) const;
	int    screenY(double canvas_y) const;
	Vec2i  screenPos(double canvas_x, double canvas_y) const { return { screenX(canvas_x), screenY(canvas_y) }; }

	void setupShader(const Shader& shader, const glm::mat4& model = glm::mat4(1.0f)) const;

private:
	bool   y_flipped_  = false; // If true, the Y-axis is flipped (ie. bottom is 0)
	bool   centered_   = false; // If true, 0,0 is at the center of the 'screen'
	double min_scale_  = 0.005;
	double max_scale_  = 100.;
	bool interpolated_ = false; // If true, pan/zoom interpolation is enabled (in which case, interpolate() needs to be
								// called regularly to correctly update the interpolated scale/zoom values)

	Vec2i size_;
	Vec2d offset_;
	Vec2d offset_inter_;
	Vec2d scale_       = { 1.0, 1.0 };
	Vec2d scale_inter_ = { 1.0, 1.0 };

	Rectd     visible_region_;
	glm::mat4 projection_matrix_ = glm::mat4(1.0f);
	glm::mat4 view_matrix_       = glm::mat4(1.0f);

	void updateVisibleRegion();
	void updateMatrices();

	// Canvas <-> Screen Coordinate Translation (uninterpolated)
	double canvasXUninterpolated(int screen_x) const;
	double canvasYUninterpolated(int screen_y) const;
	Vec2d  canvasPosUninterpolated(const Vec2i& screen_pos) const;
};
} // namespace slade::gl
