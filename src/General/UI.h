#pragma once

namespace UI
{
// General
void init(double scale = 1.);

// Splash Window
void  enableSplash(bool enable);
void  showSplash(const string& message, bool progress = false, wxWindow* parent = nullptr);
void  hideSplash();
void  updateSplash();
float getSplashProgress();
void  setSplashMessage(const string& message);
void  setSplashProgressMessage(const string& message);
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

} // namespace UI
