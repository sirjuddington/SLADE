#pragma once

namespace slade
{
class SplashWindow : public wxMiniFrame
{
public:
	SplashWindow(wxWindow* parent = nullptr);
	~SplashWindow() = default;

	float progress() const { return progress_; }
	void  setMessage(const wxString& message);
	void  setProgressMessage(const wxString& message);
	void  setProgress(float progress);

	void show(const wxString& message, bool progress = false);
	void hide();
	void forceRedraw();

	static void init();

	// Events
	void onPaint(wxPaintEvent& e);

private:
	wxString    message_;
	wxString    message_progress_;
	float       progress_                 = 0.f;
	float       progress_indefinite_anim_ = 0.f;
	bool        show_progress_            = false;
	wxStopWatch timer_;
};
} // namespace slade
