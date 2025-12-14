#pragma once

#include "General/JsonFwd.h"

class wxWebRequestEvent;

namespace slade::ui
{
class CrashReportDialog : public wxDialog
{
public:
	CrashReportDialog(wxWindow* parent);
	~CrashReportDialog() override = default;

	void loadFromCpptrace(const cpptrace::stacktrace& trace);

private:
	wxTextCtrl*      text_stack_;
	wxButton*        btn_copy_trace_;
	wxButton*        btn_exit_;
	wxButton*        btn_send_exit_;
	wxButton*        btn_github_issue_;
	string           trace_;
	string           top_level_;
	unique_ptr<Json> j_info_;
	int              send_report_request_id_ = 0;

	void onBtnCopyTrace(wxCommandEvent& e);
	void onBtnPostReport(wxCommandEvent& e);
	void onBtnSendAndExit(wxCommandEvent& e);
	void onBtnExit(wxCommandEvent& e) { EndModal(wxID_OK); }
	void onWebRequestUpdate(wxWebRequestEvent& e);
};
} // namespace slade::ui
