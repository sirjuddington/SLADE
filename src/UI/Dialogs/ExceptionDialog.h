#pragma once

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
};
} // namespace slade::ui
