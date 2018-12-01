#pragma once

class SplashWindow : public wxMiniFrame
{
public:
	SplashWindow();
	~SplashWindow() {}

	float progress() const { return progress_; }
	void  setMessage(string message);
	void  setProgressMessage(string message);
	void  setProgress(float progress);

	void show(string message, bool progress = false, wxWindow* parent = nullptr);
	void hide();
	void forceRedraw();

	static void init();

	// Events
	void onPaint(wxPaintEvent& e);

private:
	string      message_;
	string      message_progress_;
	float       progress_;
	float       progress_indefinite_anim_;
	bool        show_progress_;
	wxStopWatch timer_;
};
