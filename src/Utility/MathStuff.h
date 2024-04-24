#pragma once

namespace slade
{
struct Plane;
}

namespace slade::math
{
constexpr double PI = 3.1415926535897932384626433832795;

double clamp(double val, double min, double max);
int    floor(double val);
int    ceil(double val);
int    round(double val);

template<typename T> T scale(T value, double scale)
{
	return static_cast<T>(static_cast<double>(value) * scale);
}

template<typename T> T scaleInverse(T value, double scale)
{
	if (scale != 0.0)
		return static_cast<T>(static_cast<double>(value) / scale);

	return 0.0;
}

} // namespace slade::math
