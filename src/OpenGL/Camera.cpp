
#include "Main.h"
#include "Camera.h"
#include "Geometry/Geometry.h"
#include "Geometry/Rect.h"
#include "Utility/MathStuff.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

using namespace slade;
using namespace gl;

CVAR(Bool, mlook_invert_y, false, CVar::Flag::Save)
CVAR(Float, camera_3d_sensitivity_x, 1.0f, CVar::Flag::Save)
CVAR(Float, camera_3d_sensitivity_y, 1.0f, CVar::Flag::Save)


// -----------------------------------------------------------------------------
// Camera class constructor
// -----------------------------------------------------------------------------
Camera::Camera(const Vec3f& world_up) : world_up_{ world_up }
{
	updateVectors();
	updateProjection();
}

// -----------------------------------------------------------------------------
// Sets the camera's [pitch]
// -----------------------------------------------------------------------------
void Camera::setPitch(float pitch)
{
	pitch_ = pitch;

	// Update camera vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera to [position]
// -----------------------------------------------------------------------------
void Camera::setPosition(const Vec3f& position)
{
	position_ = position;
}

// -----------------------------------------------------------------------------
// Sets the camera [direction]
// -----------------------------------------------------------------------------
void Camera::setDirection(const Vec2f& direction)
{
	direction_ = direction;

	// Update camera vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Sets the camera position to [position], facing [direction]
// -----------------------------------------------------------------------------
void Camera::set(const Vec3f& position, const Vec2f& direction)
{
	// Set camera position/direction
	position_  = position;
	direction_ = direction;
	pitch_     = 0;

	// Update camera vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Sets the camera's horizontal field of view to [fov]
// -----------------------------------------------------------------------------
void Camera::setFov(float fov)
{
	fov_ = fov;
	updateProjection();
}

// -----------------------------------------------------------------------------
// Sets the camera's aspect ratio to [aspect]
// -----------------------------------------------------------------------------
void Camera::setAspectRatio(float aspect)
{
	aspect_ = aspect;
	updateProjection();
}

// -----------------------------------------------------------------------------
// Sets the camera's far clipping plane to [far]
// -----------------------------------------------------------------------------
void Camera::setFar(float far)
{
	far_ = far;
	updateProjection();
}

void Camera::setProjection(float width, float height, float near, float far, float fov_h)
{
	aspect_ = (1.6f / 1.333333f) * (width / height);
	fov_    = 2 * atan(tan(geometry::degToRad(fov_h) / 2) / aspect_);
	near_   = near;
	far_    = far;

	updateProjection();
}

// -----------------------------------------------------------------------------
// Calculates and returns a 'strafe line' from the camera position along the
// strafe vector (length 1)
// -----------------------------------------------------------------------------
Seg2f Camera::strafeLine() const
{
	return { position_.xy(), (position_ + strafe_).xy() };
}

// -----------------------------------------------------------------------------
// Moves the camera the direction it is facing by [distance].
// If [z] is false it will only be moved along x/y axes
// -----------------------------------------------------------------------------
void Camera::move(float distance, bool z)
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
void Camera::turn(float angle)
{
	// Find rotated view point
	Vec2f cp2d(position_.x, position_.y);
	Vec2f nd = geometry::rotatePoint(cp2d, cp2d + direction_, angle);

	// Update direction
	direction_.x = nd.x - position_.x;
	direction_.y = nd.y - position_.y;

	// Update vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera along the z axis by [distance]
// -----------------------------------------------------------------------------
void Camera::moveUp(float distance)
{
	position_.z += distance;

	updateView();
}

// -----------------------------------------------------------------------------
// Moves the camera along the strafe axis by [distance]
// -----------------------------------------------------------------------------
void Camera::strafe(float distance)
{
	position_.x += strafe_.x * distance;
	position_.y += strafe_.y * distance;

	updateView();
}

// -----------------------------------------------------------------------------
// Rotates the camera view around the strafe axis by [amount]
// -----------------------------------------------------------------------------
void Camera::pitch(float amount)
{
	// Pitch camera
	pitch_ += amount;

	// Clamp
	float rad90 = math::PI * 0.5f;
	pitch_      = glm::clamp(pitch_, -rad90, rad90);

	// Update vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera direction/pitch based on [xrel],[yrel]
// -----------------------------------------------------------------------------
void Camera::look(float xrel, float yrel)
{
	turn(-xrel * 0.1f * camera_3d_sensitivity_x);
	if (mlook_invert_y)
		pitch(yrel * 0.003f * camera_3d_sensitivity_y);
	else
		pitch(-yrel * 0.003f * camera_3d_sensitivity_y);
}

// -----------------------------------------------------------------------------
// Applies gravity to the camera, with the 'floor' at [floor_height], adjusting
// to be [view_hight] above the floor.
// Returns true if the camera position was changed
// -----------------------------------------------------------------------------
bool Camera::applyGravity(float floor_height, float view_height, float mult)
{
	bool changed = false;
	auto feet_z  = position_.z - view_height; // Camera 'feet' position
	if (feet_z > floor_height)
	{
		auto diff = feet_z - floor_height;
		position_.z -= diff * 0.2f * mult;
		if (position_.z - view_height < floor_height)
			position_.z = floor_height + view_height;
		changed = true;
	}

	else if (feet_z < floor_height)
	{
		auto diff = floor_height - feet_z;
		position_.z += diff * 0.5f * mult;
		if (position_.z - view_height > floor_height)
			position_.z = floor_height + view_height;
		changed = true;
	}

	updateView();

	return changed;
}

// -----------------------------------------------------------------------------
// Updates the strafe, direction and up vectors for the camera
// -----------------------------------------------------------------------------
void Camera::updateVectors()
{
	// Normalize direction
	direction_ = glm::normalize(direction_);

	// Calculate strafe vector
	strafe_ = glm::normalize(glm::cross(Vec3f(direction_, 0), world_up_));

	// Calculate 3d direction vector
	dir3d_ = glm::normalize(geometry::rotateVector3D(Vec3f(direction_, 0), strafe_, pitch_));

	// Calculate up vector
	up_ = glm::normalize(glm::cross(strafe_, dir3d_));

	updateView();
}

void Camera::updateView()
{
	view_ = glm::lookAt(position_, position_ + dir3d_, up_);
}

void Camera::updateProjection()
{
	projection_ = glm::perspective(fov_, aspect_, near_, far_);
}
