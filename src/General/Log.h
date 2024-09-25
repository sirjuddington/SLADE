#pragma once

namespace slade
{
namespace log
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
		std::tm     timestamp;

		Message(string_view message, MessageType type, std::tm timestamp) :
			message{ message.data(), message.size() }, type{ type }, timestamp{ timestamp }
		{
		}

		string formattedMessageLine() const;
	};

	const vector<Message>& history();
	int                    verbosity();
	void                   setVerbosity(int verbosity);
	void                   init();
	void                   message(MessageType type, int level, string_view text);
	void                   message(MessageType type, string_view text);
	void                   message(MessageType type, int level, string_view text, fmt::format_args args);
	void                   message(MessageType type, string_view text, fmt::format_args args);
	vector<Message*>       since(time_t time, MessageType type = MessageType::Any);


	// Message shortcuts by type
	// -----------------------------------------------------------------------------

	inline void info(int level, const wxString& text) { message(MessageType::Info, level, text.ToStdString()); }
	inline void info(const wxString& text) { message(MessageType::Info, text.ToStdString()); }

	inline void warning(int level, const wxString& text) { message(MessageType::Warning, level, text.ToStdString()); }
	inline void warning(const wxString& text) { message(MessageType::Warning, text.ToStdString()); }

	inline void error(int level, const wxString& text) { message(MessageType::Error, level, text.ToStdString()); }
	inline void error(const wxString& text) { message(MessageType::Error, text.ToStdString()); }

	// These can't be inline, need access to Global::debug
	void debug(int level, const wxString& text);
	void debug(const wxString& text);
	void debug(int level, string_view text, fmt::format_args args);
	void debug(string_view text, fmt::format_args args);

	inline void console(const wxString& text) { message(MessageType::Console, text.ToStdString()); }


	// Message shortcuts by type with args for fmt::format
	// -----------------------------------------------------------------------------

	template<typename... Args> void info(int level, string_view text, const Args&... args)
	{
		message(MessageType::Info, level, text, fmt::make_format_args(args...));
	}
	template<typename... Args> void info(string_view text, const Args&... args)
	{
		message(MessageType::Info, text, fmt::make_format_args(args...));
	}

	template<typename... Args> void warning(int level, string_view text, const Args&... args)
	{
		message(MessageType::Warning, level, text, fmt::make_format_args(args...));
	}
	template<typename... Args> void warning(string_view text, const Args&... args)
	{
		message(MessageType::Warning, text, fmt::make_format_args(args...));
	}

	template<typename... Args> void error(int level, string_view text, const Args&... args)
	{
		message(MessageType::Error, level, text, fmt::make_format_args(args...));
	}
	template<typename... Args> void error(string_view text, const Args&... args)
	{
		message(MessageType::Error, text, fmt::make_format_args(args...));
	}

	template<typename... Args> void debug(int level, string_view text, const Args&... args)
	{
		if (global::debug)
			message(MessageType::Debug, level, text, fmt::make_format_args(args...));
	}
	template<typename... Args> void debug(string_view text, const Args&... args)
	{
		if (global::debug)
			message(MessageType::Debug, text, fmt::make_format_args(args...));
	}
} // namespace log


// Debug helper type
#ifdef SLADE_DEBUG
#include <typeinfo>
class Debuggable
{
	wxString repr;

public:
	Debuggable(const wxString v) { repr = v; }
	Debuggable(const string v) { repr = v; }
	Debuggable(const char* v) { repr = v; }
	Debuggable(const bool v) { repr = v ? "true" : "false"; }
	Debuggable(const int v) { repr = wxString::Format("%d", v); }
	Debuggable(const unsigned int v) { repr = wxString::Format("%u", v); }
	Debuggable(const long v) { repr = wxString::Format("%ld", v); }
	Debuggable(const unsigned long v) { repr = wxString::Format("%lu", v); }
	Debuggable(const double v) { repr = wxString::Format("%g", v); }

	Debuggable(const Vec2d v) { repr = wxString::Format("(%0.6f, %0.6f)", v.x, v.y); }
	Debuggable(const Vec3f v) { repr = wxString::Format("(%0.6f, %0.6f, %0.6f)", v.x, v.y, v.z); }
	Debuggable(const Rectf v) { repr = wxString::Format("(%0.6f, %0.6f to %0.6f, %0.6f)", v.x1(), v.y1(), v.x2(), v.y2()); }

	Debuggable(const void* v) { repr = wxString::Format("%08p", v); }
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

	wxString get() { return this->repr; }
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
	wxString message;
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
	log::message(log::MessageType::Debug, 0, message.ToStdString());
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
