
#include "Main.h"
#include "UI.h"
#include "UI/SplashWindow.h"

void UI::showSplash(string message, bool progress, wxWindow* parent)
{
	SplashWindow::getInstance()->show(message, progress, parent);
}

void UI::hideSplash()
{
	SplashWindow::getInstance()->hide();
}

void UI::updateSplash()
{
	SplashWindow::getInstance()->forceRedraw();
}

float UI::getSplashProgress()
{
	return SplashWindow::getInstance()->getProgress();
}

void UI::setSplashMessage(string message)
{
	SplashWindow::getInstance()->setMessage(message);
}

void UI::setSplashProgressMessage(string message)
{
	SplashWindow::getInstance()->setProgressMessage(message);
}

void UI::setSplashProgress(float progress)
{
	SplashWindow::getInstance()->setProgress(progress);
}
