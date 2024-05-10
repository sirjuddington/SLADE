#pragma once

namespace slade::log
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

	Message(string_view message, MessageType type, const std::tm& timestamp) :
		message{ message.data(), message.size() },
		type{ type },
		timestamp{ timestamp }
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

inline void info(int level, const wxString& text)
{
	message(MessageType::Info, level, text.ToStdString());
}
inline void info(const wxString& text)
{
	message(MessageType::Info, text.ToStdString());
}

inline void warning(int level, const wxString& text)
{
	message(MessageType::Warning, level, text.ToStdString());
}
inline void warning(const wxString& text)
{
	message(MessageType::Warning, text.ToStdString());
}

inline void error(int level, const wxString& text)
{
	message(MessageType::Error, level, text.ToStdString());
}
inline void error(const wxString& text)
{
	message(MessageType::Error, text.ToStdString());
}

// These can't be inline, need access to Global::debug
void debug(int level, const wxString& text);
void debug(const wxString& text);
void debug(int level, string_view text, fmt::format_args args);
void debug(string_view text, fmt::format_args args);

inline void console(const wxString& text)
{
	message(MessageType::Console, text.ToStdString());
}


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

template<typename... Args> void console(int level, string_view text, const Args&... args)
{
	message(MessageType::Console, level, text, fmt::make_format_args(args...));
}
template<typename... Args> void console(string_view text, const Args&... args)
{
	message(MessageType::Console, text, fmt::make_format_args(args...));
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
} // namespace slade::log
