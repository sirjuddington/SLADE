#pragma once

class wxWindow;
namespace UI
{
	// Splash Window
	void	showSplash(string message, bool progress = false, wxWindow* parent = nullptr);
	void	hideSplash();
	void	updateSplash();
	float	getSplashProgress();
	void	setSplashMessage(string message);
	void	setSplashProgressMessage(string message);
	void	setSplashProgress(float progress);
}
