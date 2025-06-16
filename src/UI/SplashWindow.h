#pragma once

namespace slade
{
class SplashWindow : public wxMiniFrame
{
public:
	SplashWindow(wxWindow* parent = nullptr);
	~SplashWindow() = default;

	float progress() const { return progress_; }
	void  setMessage(string_view message);
	void  setProgressMessage(string_view message);
	void  setProgress(float progress);

	void show(string_view message, bool progress = false);
	void hide();
	void forceRedraw(bool yield_for_ui = true);

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
} // namespace slade
