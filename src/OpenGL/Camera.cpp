
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Camera.cpp
// Description: A Camera class for the 3D map view, handling position,
//              direction, projection, etc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Camera.h"
#include "Geometry/BBox.h"
#include "Geometry/Geometry.h"
#include "Geometry/Rect.h"
#include "Utility/MathStuff.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

using namespace slade;
using namespace gl;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, map3d_mlook_invert_y, false, CVar::Flag::Save)
CVAR(Float, map3d_mlook_sensitivity_x, 1.0, CVar::Flag::Save)
CVAR(Float, map3d_mlook_sensitivity_y, 1.0, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Camera Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Camera class constructor
// -----------------------------------------------------------------------------
Camera::Camera(const Vec3d& world_up) : world_up_{ world_up }
{
	updateVectors();
	updateProjection();
}

// -----------------------------------------------------------------------------
// Sets the camera's [pitch]
// -----------------------------------------------------------------------------
void Camera::setPitch(double pitch)
{
	pitch_ = pitch;

	// Update camera vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera to [position]
// -----------------------------------------------------------------------------
void Camera::setPosition(const Vec3d& position)
{
	position_ = position;
}

// -----------------------------------------------------------------------------
// Sets the camera [direction]
// -----------------------------------------------------------------------------
void Camera::setDirection(const Vec2d& direction)
{
	direction_ = direction;

	// Update camera vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Sets the camera position to [position], facing [direction]
// -----------------------------------------------------------------------------
void Camera::set(const Vec3d& position, const Vec2d& direction)
{
	// Set camera position/direction
	position_  = position;
	direction_ = direction;
	pitch_     = 0.0;

	// Update camera vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Sets the camera's horizontal field of view to [fov]
// -----------------------------------------------------------------------------
void Camera::setFov(double fov)
{
	fov_ = fov;
	updateProjection();
}

// -----------------------------------------------------------------------------
// Sets the camera's aspect ratio to [aspect]
// -----------------------------------------------------------------------------
void Camera::setAspectRatio(double aspect)
{
	aspect_ = aspect;
	updateProjection();
}

// -----------------------------------------------------------------------------
// Sets the camera's far clipping plane to [far]
// -----------------------------------------------------------------------------
void Camera::setFar(double far)
{
	far_ = far;
	updateProjection();
}

// -----------------------------------------------------------------------------
// Sets the camera's projection matrix
// -----------------------------------------------------------------------------
void Camera::setProjection(double width, double height, double near, double far, double fov_h)
{
	aspect_ = (1.6 / 1.333333) * (width / height);
	fov_    = 2 * atan(tan(geometry::degToRad(fov_h) / 2) / aspect_);
	near_   = near;
	far_    = far;

	updateProjection();
}

// -----------------------------------------------------------------------------
// Enables or disables reverse depth mode for the camera
// -----------------------------------------------------------------------------
void Camera::enableReverseDepth(bool enable)
{
	reverse_depth_ = enable;
	updateProjection();
}

// -----------------------------------------------------------------------------
// Calculates and returns a 'strafe line' from the camera position along the
// strafe vector (length 1)
// -----------------------------------------------------------------------------
Seg2d Camera::strafeLine() const
{
	return { position_.xy(), (position_ + strafe_).xy() };
}

// -----------------------------------------------------------------------------
// Moves the camera the direction it is facing by [distance].
// If [z] is false it will only be moved along x/y axes
// -----------------------------------------------------------------------------
void Camera::move(double distance, bool z)
{
	// Move along direction vector
	if (z)
	{
		position_.x += dir3d_.x * distance;
		position_.y += dir3d_.y * distance;
		position_.z += dir3d_.z * distance;
	}
	else
	{
		position_.x += direction_.x * distance;
		position_.y += direction_.y * distance;
	}

	updateView();
}

// -----------------------------------------------------------------------------
// Rotates the camera by [angle] around the z axis
// -----------------------------------------------------------------------------
void Camera::turn(double angle)
{
	// Find rotated view point
	Vec2d cp2d(position_.x, position_.y);
	Vec2d nd = geometry::rotatePoint(cp2d, cp2d + direction_, angle);

	// Update direction
	direction_.x = nd.x - position_.x;
	direction_.y = nd.y - position_.y;

	// Update vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera along the z axis by [distance]
// -----------------------------------------------------------------------------
void Camera::moveUp(double distance)
{
	position_.z += distance;

	updateView();
}

// -----------------------------------------------------------------------------
// Moves the camera along the strafe axis by [distance]
// -----------------------------------------------------------------------------
void Camera::strafe(double distance)
{
	position_.x += strafe_.x * distance;
	position_.y += strafe_.y * distance;

	updateView();
}

// -----------------------------------------------------------------------------
// Rotates the camera view around the strafe axis by [amount]
// -----------------------------------------------------------------------------
void Camera::pitch(double amount)
{
	// Pitch camera
	pitch_ += amount;

	// Clamp
	double rad90 = math::PI * 0.5;
	pitch_       = glm::clamp(pitch_, -rad90, rad90);

	// Update vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera direction/pitch based on [xrel],[yrel]
// -----------------------------------------------------------------------------
void Camera::look(double xrel, double yrel)
{
	turn(-xrel * 0.1 * map3d_mlook_sensitivity_x);
	if (map3d_mlook_invert_y)
		pitch(yrel * 0.003 * map3d_mlook_sensitivity_y);
	else
		pitch(-yrel * 0.003 * map3d_mlook_sensitivity_y);
}

// -----------------------------------------------------------------------------
// Applies gravity to the camera, with the 'floor' at [floor_height], adjusting
// to be [view_hight] above the floor.
// Returns true if the camera position was changed
// -----------------------------------------------------------------------------
bool Camera::applyGravity(double floor_height, double view_height, double mult)
{
	bool changed = false;
	auto feet_z  = position_.z - view_height; // Camera 'feet' position
	if (feet_z > floor_height)
	{
		auto diff = feet_z - floor_height;
		position_.z -= diff * 0.2 * mult;
		if (position_.z - view_height < floor_height)
			position_.z = floor_height + view_height;
		changed = true;
	}

	else if (feet_z < floor_height)
	{
		auto diff = floor_height - feet_z;
		position_.z += diff * 0.5 * mult;
		if (position_.z - view_height > floor_height)
			position_.z = floor_height + view_height;
		changed = true;
	}

	updateView();

	return changed;
}

// -----------------------------------------------------------------------------
// Checks if a 2D point is within the camera's frustum
// -----------------------------------------------------------------------------
bool Camera::pointInFrustum2d(const Vec2d& point) const
{
	auto z = pitch_ < 0 ? position_.z - 4096.0 : position_.z + 4096.0;

	// Check against left/right planes
	for (auto plane : { FrustumPlane::Left, FrustumPlane::Right })
		if (frustumPlane(plane).distanceTo(Vec3d(point, z)) < 0)
			return false;

	return true;
}

// -----------------------------------------------------------------------------
// Checks if a 2D line is within the camera's frustum
// -----------------------------------------------------------------------------
bool Camera::lineInFrustum2d(const Seg2d& line) const
{
	auto z = pitch_ < 0 ? position_.z - 4096.0 : position_.z + 4096.0;

	// Check against left/right planes
	for (auto plane : { FrustumPlane::Left, FrustumPlane::Right })
	{
		auto d1 = frustumPlane(plane).distanceTo(Vec3d(line.start(), z));
		auto d2 = frustumPlane(plane).distanceTo(Vec3d(line.end(), z));
		if (d1 < 0 && d2 < 0)
			return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Checks if a 2D bounding box is within the camera's frustum
// -----------------------------------------------------------------------------
bool Camera::bboxInFrustum2d(const BBox& bbox) const
{
	auto z = pitch_ < 0 ? position_.z - 4096.0 : position_.z + 4096.0;

	// Check against left/right planes
	for (auto plane : { FrustumPlane::Left, FrustumPlane::Right })
	{
		auto d1 = frustumPlane(plane).distanceTo(Vec3d(bbox.min.x, bbox.min.y, z));
		auto d2 = frustumPlane(plane).distanceTo(Vec3d(bbox.max.x, bbox.min.y, z));
		auto d3 = frustumPlane(plane).distanceTo(Vec3d(bbox.min.x, bbox.max.y, z));
		auto d4 = frustumPlane(plane).distanceTo(Vec3d(bbox.max.x, bbox.max.y, z));
		if (d1 < 0 && d2 < 0 && d3 < 0 && d4 < 0)
			return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Checks if a sphere at [center] with [radius] is within the camera's frustum
// -----------------------------------------------------------------------------
bool Camera::sphereInFrustum(const Vec3d& center, double radius) const
{
	// Check against all planes
	for (const auto& plane : frustum_planes_)
		if (plane.distanceTo(center) < -radius)
			return false;

	return true;
}

// -----------------------------------------------------------------------------
// Updates the strafe, direction and up vectors for the camera
// -----------------------------------------------------------------------------
void Camera::updateVectors()
{
	// Normalize direction
	direction_ = glm::normalize(direction_);

	// Calculate strafe vector
	strafe_ = glm::normalize(glm::cross(Vec3d(direction_, 0), world_up_));

	// Calculate 3d direction vector
	dir3d_ = glm::normalize(geometry::rotateVector3D(Vec3d(direction_, 0), strafe_, pitch_));

	// Calculate up vector
	up_ = glm::normalize(glm::cross(strafe_, dir3d_));

	updateView();
}

// -----------------------------------------------------------------------------
// Updates the camera's view matrix and frustum planes
// -----------------------------------------------------------------------------
void Camera::updateView()
{
	view_ = glm::lookAt(position_, position_ + dir3d_, up_);
	updateFrustumPlanes();
}

// -----------------------------------------------------------------------------
// Updates the camera's projection matrix and frustum planes
// -----------------------------------------------------------------------------
void Camera::updateProjection()
{
	if (reverse_depth_)
	{
		// Reverse depth projection matrix for better precision at distance
		float     focal_length = 1.0f / glm::tan(fov_ * 0.5f);
		glm::mat4 proj         = glm::mat4(0.0f);
		proj[0][0]             = focal_length / aspect_; // Scale for aspect ratio
		proj[1][1]             = focal_length;           // Scale for FOV
		proj[2][2]             = 0.0f;                   // Reverse depth: no linear component
		proj[2][3]             = -1.0f;                  // Map to NDC
		proj[3][2]             = near_;                  // Shift based on near plane

		projection_ = proj;
	}
	else
	{
		projection_ = glm::perspective(fov_, aspect_, near_, far_);
	}

	updateFrustumPlanes();
}

// -----------------------------------------------------------------------------
// Updates the camera's frustum planes based on the current view and projection
// matrices. The planes are stored in world space and point inward.
// -----------------------------------------------------------------------------
void Camera::updateFrustumPlanes()
{
	const glm::mat4 clip = projection_ * view_;

	auto set_plane = [this](FrustumPlane plane, double a, double b, double c, double d)
	{
		frustum_planes_[static_cast<u8>(plane)].set(a, b, c, -d);
		frustum_planes_[static_cast<u8>(plane)].normalize();
	};

	// Left plane
	set_plane(
		FrustumPlane::Left,
		clip[0][3] + clip[0][0],
		clip[1][3] + clip[1][0],
		clip[2][3] + clip[2][0],
		clip[3][3] + clip[3][0]);

	// Right plane
	set_plane(
		FrustumPlane::Right,
		clip[0][3] - clip[0][0],
		clip[1][3] - clip[1][0],
		clip[2][3] - clip[2][0],
		clip[3][3] - clip[3][0]);

	// Bottom plane
	set_plane(
		FrustumPlane::Bottom,
		clip[0][3] + clip[0][1],
		clip[1][3] + clip[1][1],
		clip[2][3] + clip[2][1],
		clip[3][3] + clip[3][1]);

	// Top plane
	set_plane(
		FrustumPlane::Top,
		clip[0][3] - clip[0][1],
		clip[1][3] - clip[1][1],
		clip[2][3] - clip[2][1],
		clip[3][3] - clip[3][1]);

	// Near plane
	set_plane(
		FrustumPlane::Near,
		clip[0][3] + clip[0][2],
		clip[1][3] + clip[1][2],
		clip[2][3] + clip[2][2],
		clip[3][3] + clip[3][2]);

	// Far plane
	set_plane(
		FrustumPlane::Far,
		clip[0][3] - clip[0][2],
		clip[1][3] - clip[1][2],
		clip[2][3] - clip[2][2],
		clip[3][3] - clip[3][2]);
}
