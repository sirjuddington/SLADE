#pragma once

class wxWebRequestEvent;

namespace slade::ui
{
class ExceptionDialog : public wxDialog
{
public:
	ExceptionDialog(
		wxWindow*     parent,
		const string& message,
		const string& stacktrace_simple,
		const string& stacktrace_full);

private:
	wxTextCtrl* text_stacktrace_;
	wxButton*   btn_send_continue_;
	wxButton*   btn_continue_;
	string      message_;
	string      stacktrace_simple_;
	string      stacktrace_full_;
	int         send_report_request_id_ = 0;

	void copyStackTrace() const;
	void sendReport();
	void onWebRequestUpdate(wxWebRequestEvent& e);
};
} // namespace slade::ui
