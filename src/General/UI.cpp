
#include "Main.h"
#include "UI.h"
#include "App.h"
#include "General/Console/Console.h"
#include "UI/SplashWindow.h"





namespace UI
{
std::unique_ptr<SplashWindow> splash_window;
bool                          splash_enabled = true;

// Pixel sizes/scale
double scale = 1.;
int    px_pad_small;
int    px_pad;
int    px_pad_min;
int    px_splitter;
int    px_spin_width;

bool isMainThread()
{
	return App::mainThreadId() == std::this_thread::get_id();
}
} // namespace UI


void UI::init(double scale)
{
	UI::scale     = scale;
	px_pad_small  = 8 * scale;
	px_pad        = 12 * scale;
	px_pad_min    = 3 * scale;
	px_splitter   = 10 * scale;
	if (App::platform() == App::Platform::Linux)
		px_spin_width = -1;
	else
		px_spin_width = 64 * scale;
}

void UI::enableSplash(bool enable)
{
	splash_enabled = enable;
}

void UI::showSplash(string message, bool progress, wxWindow* parent)
{
	if (!splash_enabled || !isMainThread())
		return;

	if (!splash_window)
	{
		SplashWindow::init();
		splash_window = std::make_unique<SplashWindow>();
	}

	splash_window->show(message, progress, parent);
}

void UI::hideSplash()
{
	if (splash_window && isMainThread())
	{
		splash_window->hide();
		splash_window.reset();
	}
}

void UI::updateSplash()
{
	if (splash_window && isMainThread())
		splash_window->forceRedraw();
}

float UI::getSplashProgress()
{
	return splash_window ? splash_window->getProgress() : 0.0f;
}

void UI::setSplashMessage(string message)
{
	if (splash_window && isMainThread())
		splash_window->setMessage(message);
}

void UI::setSplashProgressMessage(string message)
{
	if (splash_window && isMainThread())
		splash_window->setProgressMessage(message);
}

void UI::setSplashProgress(float progress)
{
	if (splash_window && isMainThread())
		splash_window->setProgress(progress);
}

void UI::setCursor(wxWindow* window, MouseCursor cursor)
{
	switch (cursor)
	{
	case MouseCursor::Hand: window->SetCursor(wxCursor(wxCURSOR_HAND)); break;
	case MouseCursor::Move: window->SetCursor(wxCursor(wxCURSOR_SIZING)); break;
	case MouseCursor::Cross: window->SetCursor(wxCursor(wxCURSOR_CROSS)); break;
	case MouseCursor::SizeNS: window->SetCursor(wxCursor(wxCURSOR_SIZENS)); break;
	case MouseCursor::SizeWE: window->SetCursor(wxCursor(wxCURSOR_SIZEWE)); break;
	case MouseCursor::SizeNESW: window->SetCursor(wxCursor(wxCURSOR_SIZENESW)); break;
	case MouseCursor::SizeNWSE: window->SetCursor(wxCursor(wxCURSOR_SIZENWSE)); break;
	default: window->SetCursor(wxNullCursor);
	}
}

double UI::scaleFactor()
{
	return scale;
}

int UI::px(Size size)
{
	switch (size)
	{
	case Size::PadLarge: return px_pad;
	case Size::Pad: return px_pad_small;
	case Size::PadMinimum: return px_pad_min;
	case Size::Splitter: return px_splitter;
	case Size::SpinCtrlWidth: return px_spin_width;
	default: return 0;
	}
}

int UI::scalePx(int px)
{
	return px * scale;
}

int UI::pad()
{
	return px_pad_small;
}

int UI::padLarge()
{
	return px_pad;
}


/* Console Command - "splash"
 * Shows the splash screen with the given message, or hides it if
 * no message is given
 *******************************************************************/
CONSOLE_COMMAND(splash, 0, false)
{
	if (args.size() == 0)
		UI::hideSplash();
	else if (args.size() == 1)
		UI::showSplash(args[0]);
	else
	{
		UI::showSplash(args[0], true);
		float prog = atof(CHR(args[1]));
		UI::setSplashProgress(prog);
		UI::setSplashProgressMessage(S_FMT("Progress %s", args[1]));
	}
}
