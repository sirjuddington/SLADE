
#ifndef __SPLASHWINDOW_H__
#define __SPLASHWINDOW_H__

#include <wx/minifram.h>
#include <wx/timer.h>

class SplashWindow : public wxMiniFrame
{
private:
	string		message;
	string		message_progress;
	float		progress;
	float		progress_indefinite_anim;
	bool		show_progress;
	wxStopWatch	timer;

	static SplashWindow*	instance;
	static wxBitmap			bm_logo;
	static int				width;
	static int				height;

public:
	SplashWindow();
	~SplashWindow();

	// Singleton implementation
	static SplashWindow* getInstance()
	{
		if (!instance)
			instance = new SplashWindow();

		return instance;
	}
	static void deleteInstance()
	{
		if (instance)
			delete instance;
		instance = NULL;
	}

	void	setMessage(string message);
	void	setProgressMessage(string message);
	void	setProgress(float progress);

	void	init();
	void	show(string message, bool progress = false);
	void	hide();
	void	forceRedraw();

	// Events
	void	onPaint(wxPaintEvent& e);
};

// Define for less cumbersome SplashWindow::getInstance()
#define theSplashWindow SplashWindow::getInstance()

#endif//__SPLASHWINDOW_H__
