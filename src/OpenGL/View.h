#pragma once

namespace slade::gl
{
class Shader;

class View
{
public:
	View() = default;
	View(bool centered, bool y_flipped, bool interpolated = true) :
		y_flipped_{ y_flipped }, centered_{ centered }, interpolated_{ interpolated }
	{
	}

	const Vec2d& offset(bool inter = false) const { return inter ? offset_inter_ : offset_; }
	double       scale(bool inter = false) const { return inter ? scale_inter_ : scale_; }
	const Vec2i& size() const { return size_; }
	const Rectd& visibleRegion() const { return visible_region_; }
	bool         yFlipped() const { return y_flipped_; }
	bool         interpolated() const { return interpolated_; }

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
	void setOffset(double x, double y)
	{
		offset_ = { x, y };

		if (!interpolated_)
			offset_inter_ = offset_;

		updateVisibleRegion();
		updateMatrices();
	}
	void setScale(double scale)
	{
		scale_ = scale;

		if (!interpolated_)
			scale_inter_ = scale_;

		updateVisibleRegion();
		updateMatrices();
	}
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
	Vec2d  canvasPos(const Vec2i& screen_pos) const;
	int    screenX(double canvas_x) const;
	int    screenY(double canvas_y) const;

	void apply(bool init = true) const;
	void setOverlayCoords(bool set) const;

	void      setupShader(const Shader& shader) const;
	glm::mat4 modelViewMatrix() const { return model_matrix_; }

private:
	bool   y_flipped_    = false; // If true, the Y-axis is flipped (ie. bottom is 0)
	bool   centered_     = false; // If true, 0,0 is at the center of the 'screen'
	double min_scale_    = 0.005;
	double max_scale_    = 10.;
	bool   interpolated_ = true; // If true, pan/zoom interpolation is enabled (in which case, interpolate() needs to be
								 // called regularly to correctly update the interpolated scale/zoom values)

	Vec2i  size_;
	Vec2d  offset_;
	Vec2d  offset_inter_;
	double scale_       = 1.;
	double scale_inter_ = 1.;

	Rectd     visible_region_;
	glm::mat4 projection_matrix_ = glm::mat4(1.0f);
	glm::mat4 model_matrix_      = glm::mat4(1.0f);

	void updateVisibleRegion();
	void updateMatrices();

	// Canvas <-> Screen Coordinate Translation (uninterpolated)
	double canvasXUninterpolated(int screen_x) const;
	double canvasYUninterpolated(int screen_y) const;
	Vec2d  canvasPosUninterpolated(const Vec2i& screen_pos) const;
};
} // namespace slade::gl
