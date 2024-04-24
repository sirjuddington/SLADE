#pragma once

#include "Geometry/RectFwd.h"

namespace slade
{
class Camera
{
public:
	Camera() = default;

	double       pitch() const { return pitch_; }
	const Vec3d& position() const { return position_; }
	const Vec2d& direction() const { return direction_; }
	const Vec3d& strafeVector() const { return strafe_; }
	const Vec3d& directionVector() const { return dir3d_; }

	void setPitch(double pitch);
	void setPosition(const Vec3d& position);
	void setDirection(const Vec2d& direction);
	void set(const Vec3d& position, const Vec2d& direction);

	Vec3d upVector() const;
	Seg2d strafeLine() const;

	void move(double distance, bool z = true);
	void turn(double angle);
	void moveUp(double distance);
	void strafe(double distance);
	void pitch(double amount);
	void look(double xrel, double yrel);
	void applyGravity(double floor_height, double mult);

private:
	Vec3d  position_;
	Vec2d  direction_;
	double pitch_ = 0.;
	Vec3d  dir3d_;
	Vec3d  strafe_;

	void updateVectors();
};
} // namespace slade
