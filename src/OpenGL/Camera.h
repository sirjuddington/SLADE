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

	Camera(const Vec3f& world_up = { 0.0f, 1.0f, 0.0f });

	float            pitch() const { return pitch_; }
	const Vec3f&     position() const { return position_; }
	const Vec2f&     direction() const { return direction_; }
	float            fov() const { return fov_; }
	float            aspect() const { return aspect_; }
	float            near() const { return near_; }
	float            far() const { return far_; }
	const Vec3f&     strafeVector() const { return strafe_; }
	const Vec3f&     directionVector() const { return dir3d_; }
	const Vec3f&     upVector() const { return up_; }
	const glm::mat4& viewMatrix() const { return view_; }
	const glm::mat4& projectionMatrix() const { return projection_; }
	const Plane&     frustumPlane(FrustumPlane plane) const { return frustum_planes_[static_cast<u8>(plane)]; }

	void setPitch(float pitch);
	void setPosition(const Vec3f& position);
	void setDirection(const Vec2f& direction);
	void set(const Vec3f& position, const Vec2f& direction);
	void setFov(float fov);
	void setAspectRatio(float aspect);
	void setFar(float far);
	void setProjection(float width, float height, float near, float far, float fov_h);
	void enableReverseDepth(bool enable);

	Seg2f strafeLine() const;

	void move(float distance, bool z = true);
	void turn(float angle);
	void moveUp(float distance);
	void strafe(float distance);
	void pitch(float amount);
	void look(float xrel, float yrel);
	bool applyGravity(float floor_height, float view_height, float mult);

	bool pointInFrustum2d(const Vec2f& point) const;
	bool lineInFrustum2d(const Seg2d& line) const;
	bool bboxInFrustum2d(const BBox& bbox) const;

private:
	// View
	Vec3f position_  = { 0.0f, 0.0f, 0.0f };
	Vec2f direction_ = { 0.0f, 1.0f };
	float pitch_     = 0.;

	// Projection
	float fov_           = 0.9817477f; // Vertical fov (is 90 degrees horizontal)
	float aspect_        = 1.6f;
	float near_          = 2.0f;
	float far_           = 40000.0f;
	bool  reverse_depth_ = true; // Use reverse depth for better precision at distance

	// Vectors & Matrices
	Vec3f                dir3d_;
	Vec3f                strafe_;
	Vec3f                up_;
	Vec3f                world_up_;
	glm::mat4            view_       = glm::mat4(1.0f);
	glm::mat4            projection_ = glm::mat4(1.0f);
	std::array<Plane, 6> frustum_planes_;

	void updateVectors();
	void updateView();
	void updateProjection();
	void updateFrustumPlanes();
};
} // namespace slade::gl
