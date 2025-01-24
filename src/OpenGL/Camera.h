#pragma once

#include "Geometry/RectFwd.h"

// These unnecessary macros are defined in windows headers somewhere...
#undef near
#undef far

namespace slade::gl
{
class Camera
{
public:
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

	void setPitch(float pitch);
	void setPosition(const Vec3f& position);
	void setDirection(const Vec2f& direction);
	void set(const Vec3f& position, const Vec2f& direction);
	void setFov(float fov);
	void setAspectRatio(float aspect);
	void setFar(float far);
	void setProjection(float width, float height, float near, float far, float fov_h);

	Seg2f strafeLine() const;

	void move(float distance, bool z = true);
	void turn(float angle);
	void moveUp(float distance);
	void strafe(float distance);
	void pitch(float amount);
	void look(float xrel, float yrel);
	void applyGravity(float floor_height, float mult);

private:
	// View
	Vec3f position_  = { 0.0f, 0.0f, 0.0f };
	Vec2f direction_ = { 0.0f, 1.0f };
	float pitch_     = 0.;

	// Projection
	float fov_    = 0.9817477f; // Vertical fov (is 90 degrees horizontal)
	float aspect_ = 1.6f;
	float near_   = 0.5f;
	float far_    = 20000.0f;

	// Vectors & Matrices
	Vec3f     dir3d_;
	Vec3f     strafe_;
	Vec3f     up_;
	Vec3f     world_up_;
	glm::mat4 view_;
	glm::mat4 projection_;

	void updateVectors();
	void updateView();
	void updateProjection();
};
} // namespace slade::gl
