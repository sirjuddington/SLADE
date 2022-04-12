#pragma once

namespace slade
{
class ConsolePanel : public wxPanel
{
public:
	ConsolePanel(wxWindow* parent, int id);
	~ConsolePanel() = default;

	void initLayout();
	void setupTextArea() const;
	void update();

private:
	wxStyledTextCtrl* text_log_      = nullptr;
	wxTextCtrl*       text_command_  = nullptr;
	int               cmd_log_index_ = 0;
	wxTimer           timer_update_;
	unsigned          next_message_index_ = 0;

	// Events
	void onCommandEnter(wxCommandEvent& e);
	void onCommandKeyDown(wxKeyEvent& e);
};
} // namespace slade
