#pragma once

namespace slade
{
struct Plane;
}

namespace slade::math
{
constexpr double PI      = 3.1415926535897932384626433832795;
constexpr float  EPSILON = 0.00001f;

double clamp(double val, double min, double max);
int    floor(double val);
int    ceil(double val);
int    round(double val);

// Checks if floats [a] and [b] are equal within [epsilon]
inline bool fEqual(const float a, const float b, const float epsilon = EPSILON)
{
	return fabs(a - b) < epsilon;
}

// Checks if float [a] is less than or equal to [b], within [epsilon]
inline bool fLessOrEqual(const float a, const float b, const float epsilon = EPSILON)
{
	return a < b || fEqual(a, b, epsilon);
}

// Checks if float [a] is less than [b], within [epsilon]
inline bool fLess(const float a, const float b, const float epsilon = EPSILON)
{
	return a < b && !fEqual(a, b, epsilon);
}

// Checks if float [a] is greater than or equal to [b], within [epsilon]
inline bool fGreaterOrEqual(const float a, const float b, const float epsilon = EPSILON)
{
	return a > b || fEqual(a, b, epsilon);
}

// Checks if float [a] is greater than [b], within [epsilon]
inline bool fGreater(const float a, const float b, const float epsilon = EPSILON)
{
	return a > b && !fEqual(a, b, epsilon);
}


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
