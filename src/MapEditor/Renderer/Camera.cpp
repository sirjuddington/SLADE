
#include "Main.h"
#include "Camera.h"
#include "Geometry/Geometry.h"
#include "Geometry/Rect.h"
#include "Utility/MathStuff.h"

using namespace slade;

CVAR(Bool, mlook_invert_y, false, CVar::Flag::Save)
CVAR(Float, camera_3d_sensitivity_x, 1.0f, CVar::Flag::Save)
CVAR(Float, camera_3d_sensitivity_y, 1.0f, CVar::Flag::Save)


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
	pitch_     = 0;

	// Update camera vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Calculates and returns the camera's up vector
// -----------------------------------------------------------------------------
Vec3d Camera::upVector() const
{
	return glm::normalize(glm::cross(strafe_, dir3d_));
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
}

// -----------------------------------------------------------------------------
// Moves the camera along the strafe axis by [distance]
// -----------------------------------------------------------------------------
void Camera::strafe(double distance)
{
	position_.x += strafe_.x * distance;
	position_.y += strafe_.y * distance;
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
	if (pitch_ > rad90)
		pitch_ = rad90;
	if (pitch_ < -rad90)
		pitch_ = -rad90;

	// Update vectors
	updateVectors();
}

// -----------------------------------------------------------------------------
// Moves the camera direction/pitch based on [xrel],[yrel]
// -----------------------------------------------------------------------------
void Camera::look(double xrel, double yrel)
{
	turn(-xrel * 0.1 * camera_3d_sensitivity_x);
	if (mlook_invert_y)
		pitch(yrel * 0.003 * camera_3d_sensitivity_y);
	else
		pitch(-yrel * 0.003 * camera_3d_sensitivity_y);
}

// -----------------------------------------------------------------------------
// Applies gravity to the camera, with the 'floor' at [floor_height]
// -----------------------------------------------------------------------------
void Camera::applyGravity(double floor_height, double mult)
{
	if (position_.z > floor_height)
	{
		double diff = position_.z - floor_height;
		position_.z -= (diff * 0.3 * mult);
		if (position_.z < floor_height)
			position_.z = floor_height;
	}

	else if (position_.z < floor_height)
	{
		double diff = floor_height - position_.z;
		position_.z += (diff * 0.5 * mult);
		if (position_.z > floor_height)
			position_.z = floor_height;
	}
}

// -----------------------------------------------------------------------------
// Updates the strafe and direction vectors for the camera
// -----------------------------------------------------------------------------
void Camera::updateVectors()
{
	// Normalize direction
	direction_ = glm::normalize(direction_);

	// Calculate strafe vector
	strafe_ = glm::normalize(glm::cross(Vec3d(direction_, 0), Vec3d(0, 0, 1)));

	// Calculate 3d direction vector
	dir3d_ = glm::normalize(geometry::rotateVector3D(Vec3d(direction_, 0), strafe_, pitch_));
}
