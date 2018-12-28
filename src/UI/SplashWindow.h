#pragma once

class SplashWindow : public wxMiniFrame
{
public:
	SplashWindow();
	~SplashWindow() = default;

	float progress() const { return progress_; }
	void  setMessage(const string& message);
	void  setProgressMessage(const string& message);
	void  setProgress(float progress);

	void show(const string& message, bool progress = false, wxWindow* parent = nullptr);
	void hide();
	void forceRedraw();

	static void init();

	// Events
	void onPaint(wxPaintEvent& e);

private:
	string      message_;
	string      message_progress_;
	float       progress_                 = 0.f;
	float       progress_indefinite_anim_ = 0.f;
	bool        show_progress_            = false;
	wxStopWatch timer_;
};
