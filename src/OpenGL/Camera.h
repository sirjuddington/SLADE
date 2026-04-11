#pragma once

#include "Geometry/Plane.h"
#include "Geometry/RectFwd.h"
#include <array>

// These unnecessary macros are defined in windows headers somewhere...
#undef near
#undef far

namespace slade
{
struct BBox;
}
namespace slade::gl
{
class Camera
{
public:
	// Frustum plane indices
	enum class FrustumPlane : u8
	{
		Left = 0,
		Right,
		Bottom,
		Top,
		Near,
		Far
	};

	Camera(const Vec3d& world_up = { 0.0f, 1.0f, 0.0f });

	double           pitch() const { return pitch_; }
	const Vec3d&     position() const { return position_; }
	const Vec2d&     direction() const { return direction_; }
	double           fov() const { return fov_; }
	double           aspect() const { return aspect_; }
	double           near() const { return near_; }
	double           far() const { return far_; }
	const Vec3d&     strafeVector() const { return strafe_; }
	const Vec3d&     directionVector() const { return dir3d_; }
	const Vec3d&     upVector() const { return up_; }
	const glm::mat4& viewMatrix() const { return view_; }
	const glm::mat4& projectionMatrix() const { return projection_; }
	const Plane&     frustumPlane(FrustumPlane plane) const { return frustum_planes_[static_cast<u8>(plane)]; }

	void setPitch(double pitch);
	void setPosition(const Vec3d& position);
	void setDirection(const Vec2d& direction);
	void set(const Vec3d& position, const Vec2d& direction);
	void setFov(double fov);
	void setAspectRatio(double aspect);
	void setFar(double far);
	void setProjection(double width, double height, double near, double far, double fov_h);
	void enableReverseDepth(bool enable);

	Seg2d strafeLine() const;

	void move(double distance, bool z = true);
	void turn(double angle);
	void moveUp(double distance);
	void strafe(double distance);
	void pitch(double amount);
	void look(double xrel, double yrel);
	bool applyGravity(double floor_height, double view_height, double mult);

	bool pointInFrustum2d(const Vec2d& point) const;
	bool lineInFrustum2d(const Seg2d& line) const;
	bool bboxInFrustum2d(const BBox& bbox) const;
	bool sphereInFrustum(const Vec3d& center, double radius) const;

private:
	// View
	Vec3d  position_  = { 0.0, 0.0, 0.0 };
	Vec2d  direction_ = { 0.0, 1.0 };
	double pitch_     = 0.0;

	// Projection
	double fov_           = 0.9817477; // Vertical fov (is 90 degrees horizontal)
	double aspect_        = 1.6;
	double near_          = 2.0;
	double far_           = 40000.0;
	bool   reverse_depth_ = true; // Use reverse depth for better precision at distance

	// Vectors & Matrices
	Vec3d                dir3d_;
	Vec3d                strafe_;
	Vec3d                up_;
	Vec3d                world_up_;
	glm::mat4            view_       = glm::mat4(1.0f);
	glm::mat4            projection_ = glm::mat4(1.0f);
	std::array<Plane, 6> frustum_planes_;

	void updateVectors();
	void updateView();
	void updateProjection();
	void updateFrustumPlanes();
};
} // namespace slade::gl
