#pragma once

namespace Log
{
enum class MessageType
{
	Info,
	Warning,
	Error,
	Debug,
	Console, // Only displayed in the console
	Script,  // Script output
	Any
};

struct Message
{
	string      message;
	MessageType type;
	time_t      timestamp;

	string formattedMessageLine() const;
};

const vector<Message>& history();
int                    verbosity();
void                   setVerbosity(int verbosity);
void                   init();
vector<Message*>       since(time_t time, MessageType type = MessageType::Any);

void message(MessageType type, int level, const char* text);
void message(MessageType type, int level, const wxString& text);
void message(MessageType type, const char* text);
void message(MessageType type, const wxString& text);

// clang-format off
inline void	info(int level, const char* text) { message(MessageType::Info, level, text); }
inline void	info(int level, const wxString& text) { message(MessageType::Info, level, text); }
inline void	info(const char* text) { message(MessageType::Info, text); }
inline void	info(const wxString& text) { message(MessageType::Info, text); }

inline void	warning(int level, const char* text) { message(MessageType::Warning, level, text); }
inline void	warning(int level, const wxString& text) { message(MessageType::Warning, level, text); }
inline void	warning(const char* text) { message(MessageType::Warning, text); }
inline void	warning(const wxString& text) { message(MessageType::Warning, text); }

inline void	error(int level, const char* text) { message(MessageType::Error, level, text); }
inline void	error(int level, const wxString& text) { message(MessageType::Error, level, text); }
inline void	error(const char* text) { message(MessageType::Error, text); }
inline void	error(const wxString& text) { message(MessageType::Error, text); }

void debug(int level, const char* text);
void debug(int level, const wxString& text);
void debug(const char* text);
void debug(const wxString& text);

inline void	console(const char* text) { message(MessageType::Console, text); }
inline void	console(const wxString& text) { message(MessageType::Console, text); }
// clang-format on
} // namespace Log

// Try to avoid using these and use Log::message/error/warning with S_FMT instead
#define LOG_MESSAGE(level, ...) Log::message(Log::MessageType::Info, level, wxString::Format(__VA_ARGS__))
#define LOG_WARNING(level, ...) Log::message(Log::MessageType::Warning, level, wxString::Format(__VA_ARGS__))
#define LOG_ERROR(level, ...) Log::message(Log::MessageType::Error, level, wxString::Format(__VA_ARGS__))

// Debug helper type
// Note: NDEBUG is a standard C macro indicating that assert()s should be
// disabled, but it's also the only macro cmake defines differently between
// debug and release builds, so we co-opt it just for this.
#ifndef NDEBUG
#include <typeinfo>
class Debuggable
{
	string repr;

public:
	Debuggable(string v) { repr = v; }
	Debuggable(const char* v) { repr = v; }
	Debuggable(bool v) { repr = v ? "true" : "false"; }
	Debuggable(int v) { repr = S_FMT("%d", v); }
	Debuggable(unsigned int v) { repr = S_FMT("%u", v); }
	Debuggable(long v) { repr = S_FMT("%ld", v); }
	Debuggable(unsigned long v) { repr = S_FMT("%lu", v); }
	Debuggable(double v) { repr = S_FMT("%g", v); }

	Debuggable(Vec2f v) { repr = S_FMT("(%0.6f, %0.6f)", v.x, v.y); }
	Debuggable(Vec3f v) { repr = S_FMT("(%0.6f, %0.6f, %0.6f)", v.x, v.y, v.z); }
	Debuggable(Rectf v) { repr = S_FMT("(%0.6f, %0.6f to %0.6f, %0.6f)", v.x1(), v.y1(), v.x2(), v.y2()); }

	template<typename T> Debuggable(T* v) { repr = Debuggable(*v).repr; }

	template<typename T> Debuggable(vector<T> v)
	{
		repr << "{";
		for (unsigned int a = 0; a < v.size(); a++)
		{
			repr << Debuggable(v[a]).get();
			if (a < v.size() - 1)
				repr << ", ";
		}
		repr << "}";
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
	message << a1.get() << " ";
	message << a2.get() << " ";
	message << a3.get() << " ";
	message << a4.get() << " ";
	message << a5.get() << " ";
	message << a6.get() << " ";
	message << a7.get() << " ";
	message << a8.get() << " ";
	message << a9.get() << " ";
	message << a10.get() << " ";
	message << a11.get() << " ";
	message << a12.get();
	message.Trim();
	Log::debug(0, S_FMT("%s", message));
}

#define LOG_DEBUG_VAR(name) LOG_DEBUG(#name ": ", name)
#else // not NDEBUG
struct Debuggable
{
	template<typename T> Debuggable(T _unused) {}
};
#define LOG_DEBUG(...)
#define LOG_DEBUG_VAR(name)
#endif // DEBUG
