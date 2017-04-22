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
	void	setCursor(wxWindow* window, MouseCursor cursor);
}
