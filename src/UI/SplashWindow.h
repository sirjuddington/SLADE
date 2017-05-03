
#ifndef __SPLASHWINDOW_H__
#define __SPLASHWINDOW_H__

#include "common.h"

class SplashWindow : public wxMiniFrame
{
public:
	SplashWindow();
	~SplashWindow();

	float	getProgress() const { return progress; }
	void	setMessage(string message);
	void	setProgressMessage(string message);
	void	setProgress(float progress);

	void	show(string message, bool progress = false, wxWindow* parent = nullptr);
	void	hide();
	void	forceRedraw();

	static void	init();

	// Events
	void	onPaint(wxPaintEvent& e);

private:
	string		message;
	string		message_progress;
	float		progress;
	float		progress_indefinite_anim;
	bool		show_progress;
	wxStopWatch	timer;

	static wxBitmap	bm_logo;
	static int		width;
	static int		height;
	static bool		init_done;
};

#endif//__SPLASHWINDOW_H__
