
#include "Main.h"
#include "General/Console/Console.h"
#include "UI.h"
#include "UI/SplashWindow.h"

namespace UI
{
	std::unique_ptr<SplashWindow> splash_window;
}


void UI::showSplash(string message, bool progress, wxWindow* parent)
{
	if (!splash_window)
	{
		SplashWindow::init();
		splash_window = std::make_unique<SplashWindow>();
	}

	splash_window->show(message, progress, parent);
}

void UI::hideSplash()
{
	if (splash_window)
	{
		splash_window->hide();
		splash_window.reset();
	}
}

void UI::updateSplash()
{
	if (splash_window)
		splash_window->forceRedraw();
}

float UI::getSplashProgress()
{
	return splash_window ? splash_window->getProgress() : 0.0f;
}

void UI::setSplashMessage(string message)
{
	if (splash_window)
		splash_window->setMessage(message);
}

void UI::setSplashProgressMessage(string message)
{
	if (splash_window)
		splash_window->setProgressMessage(message);
}

void UI::setSplashProgress(float progress)
{
	if (splash_window)
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
