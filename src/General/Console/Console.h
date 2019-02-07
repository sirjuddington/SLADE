#pragma once

class ConsoleCommand
{
public:
	ConsoleCommand(
		const wxString& name,
		void (*command_func)(const vector<wxString>&),
		int  min_args,
		bool show_in_list = true);
	~ConsoleCommand() = default;

	wxString name() const { return name_; }
	bool     showInList() const { return show_in_list_; }
	void     execute(const vector<wxString>& args) const;
	size_t   minArgs() const { return min_args_; }

	bool operator<(ConsoleCommand c) const { return name_ < c.name(); }
	bool operator>(ConsoleCommand c) const { return name_ > c.name(); }

private:
	wxString name_;
	void (*command_func_)(const vector<wxString>&);
	size_t min_args_;
	bool   show_in_list_;
};

class Console
{
public:
	Console()  = default;
	~Console() = default;

	int             numCommands() const { return (int)commands_.size(); }
	ConsoleCommand& command(size_t index);

	void     addCommand(ConsoleCommand& c);
	void     execute(const wxString& command);
	wxString lastCommand();
	wxString prevCommand(int index);
	int      numPrevCommands() const { return cmd_log_.size(); }

private:
	vector<ConsoleCommand> commands_;
	vector<wxString>       cmd_log_;
};

// Define for neat console command definitions
#define CONSOLE_COMMAND(name, min_args, show_in_list)              \
	void           c_##name(const vector<wxString>& args);         \
	ConsoleCommand name(#name, &c_##name, min_args, show_in_list); \
	void           c_##name(const vector<wxString>& args)
