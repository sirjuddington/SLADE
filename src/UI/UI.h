#pragma once

namespace slade::ui
{
// General
void init();

// Splash Window
void  enableSplash(bool enable);
void  showSplash(string_view message, bool progress = false, wxWindow* parent = nullptr);
void  hideSplash();
void  updateSplash();
float getSplashProgress();
void  setSplashMessage(string_view message);
void  setSplashProgressMessage(string_view message);
void  setSplashProgress(float progress);
void  setSplashProgress(unsigned current, unsigned max);

// Mouse Cursor
enum class MouseCursor
{
	Normal,
	Hand,
	Move,
	Cross,
	SizeNS,
	SizeWE,
	SizeNESW,
	SizeNWSE
};
void setCursor(wxWindow* window, MouseCursor cursor);

// Standard sizes and padding (in DIP, "DPI Independent Pixels")
enum class Size
{
	PadSmall,
	Pad,
	PadLarge,
	PadXLarge,
	Splitter,
	SpinCtrlWidth
};
int sizePx(Size size, const wxWindow* window = nullptr);
int pad(const wxWindow* window = nullptr);
int padLarge(const wxWindow* window = nullptr);
int padXLarge(const wxWindow* window = nullptr);
int padSmall(const wxWindow* window = nullptr);

// Message Boxes
enum class MessageBoxType : u8
{
	Info,
	Warning,
	Error,
	Question
};
enum class MessageBoxButtons : u8
{
	Ok,
	OkCancel,
	YesNo,
	YesNoCancel
};
enum class MessageBoxResult : u8
{
	Ok,
	Cancel,
	Yes,
	No
};
MessageBoxResult messageBox(
	MessageBoxType    type,
	string_view       title,
	string_view       message,
	MessageBoxButtons buttons = MessageBoxButtons::Ok,
	wxWindow*         parent  = nullptr);

} // namespace slade::ui
