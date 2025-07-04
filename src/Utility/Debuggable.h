#pragma once

#include "Geometry/Rect.h"

namespace slade
{
// Debug helper type
#ifdef SLADE_DEBUG
#include <typeinfo>
class Debuggable
{
	string repr;

public:
	Debuggable(const wxString& v) { repr = v.utf8_string(); }
	Debuggable(const string& v) { repr = v; }
	Debuggable(const char* v) { repr = v; }
	Debuggable(const bool v) { repr = v ? "true" : "false"; }
	Debuggable(const int v) { repr = fmt::format("{}", v); }
	Debuggable(const unsigned int v) { repr = fmt::format("{}", v); }
	Debuggable(const long v) { repr = fmt::format("{}", v); }
	Debuggable(const unsigned long v) { repr = fmt::format("{}", v); }
	Debuggable(const double v) { repr = fmt::format("{}", v); }

	Debuggable(const Vec2d& v) { repr = fmt::format("({:0.6f}, {:0.6f})", v.x, v.y); }
	Debuggable(const Vec3f& v) { repr = fmt::format("({:0.6f}, {:0.6f}, {:0.6f})", v.x, v.y, v.z); }
	Debuggable(const Rectf& v)
	{
		repr = fmt::format("({:0.6f}, {:0.6f} to {:0.6f}, {:0.6f})", v.x1(), v.y1(), v.x2(), v.y2());
	}

	Debuggable(const void* v) { repr = fmt::format("{:08p}", v); }
	template<typename T> Debuggable(T* v) { repr = Debuggable(*v).repr; }

	template<typename T> Debuggable(const vector<T>& v)
	{
		repr += "{";
		for (unsigned int a = 0; a < v.size(); a++)
		{
			repr += Debuggable(v[a]).get();
			if (a < v.size() - 1)
				repr += ", ";
		}
		repr += "}";
	}

	string get() { return this->repr; }
};

inline void LOG_DEBUG(
	Debuggable a1  = "",
	Debuggable a2  = "",
	Debuggable a3  = "",
	Debuggable a4  = "",
	Debuggable a5  = "",
	Debuggable a6  = "",
	Debuggable a7  = "",
	Debuggable a8  = "",
	Debuggable a9  = "",
	Debuggable a10 = "",
	Debuggable a11 = "",
	Debuggable a12 = "")
{
	string message;
	message += a1.get() + " ";
	message += a2.get() + " ";
	message += a3.get() + " ";
	message += a4.get() + " ";
	message += a5.get() + " ";
	message += a6.get() + " ";
	message += a7.get() + " ";
	message += a8.get() + " ";
	message += a9.get() + " ";
	message += a10.get() + " ";
	message += a11.get() + " ";
	message += a12.get();
	log::message(log::MessageType::Debug, 0, message);
}

#define LOG_DEBUG_VAR(name) LOG_DEBUG(#name ": ", name)
#else // SLADE_DEBUG
struct Debuggable
{
	template<typename T> Debuggable(T _unused) {}
};
#define LOG_DEBUG(...)
#define LOG_DEBUG_VAR(name)
#endif // SLADE_DEBUG
} // namespace slade
