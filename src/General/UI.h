#pragma once

namespace slade::ui
{
// General
void init(double scale = 1.);

// Splash Window
void  enableSplash(bool enable);
void  showSplash(string_view message, bool progress = false, wxWindow* parent = nullptr);
void  hideSplash();
void  updateSplash();
float getSplashProgress();
void  setSplashMessage(string_view message);
void  setSplashProgressMessage(string_view message);
void  setSplashProgress(float progress);

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

// Pixel spacing and scaling
enum class Size
{
	PadLarge,
	Pad,
	PadMinimum,
	Splitter,
	SpinCtrlWidth
};
double scaleFactor();
int    px(Size size);
int    scalePx(int px);
int    pad();      // Shortcut for UI::px(UI::Size::Pad)
int    padLarge(); // Shortcut for UI::px(UI::Size::PadLarge)

} // namespace slade::ui
