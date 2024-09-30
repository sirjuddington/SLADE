#pragma once

namespace slade
{
class ConsoleCommand
{
public:
	ConsoleCommand(
		string_view name,
		void (*command_func)(const vector<string>&),
		int  min_args,
		bool show_in_list = true);
	~ConsoleCommand() = default;

	string name() const { return name_; }
	bool   showInList() const { return show_in_list_; }
	void   execute(const vector<string>& args) const;
	size_t minArgs() const { return min_args_; }

	bool operator<(const ConsoleCommand& c) const { return name_ < c.name(); }
	bool operator>(const ConsoleCommand& c) const { return name_ > c.name(); }

private:
	string name_;
	void (*command_func_)(const vector<string>&);
	size_t min_args_;
	bool   show_in_list_;
};

class Console
{
public:
	Console()  = default;
	~Console() = default;

	int             numCommands() const { return static_cast<int>(commands_.size()); }
	ConsoleCommand& command(size_t index);

	void   addCommand(const ConsoleCommand& c);
	void   execute(string_view command);
	string lastCommand();
	string prevCommand(int index);
	int    numPrevCommands() const { return cmd_log_.size(); }

private:
	vector<ConsoleCommand> commands_;
	vector<string>         cmd_log_;
};
} // namespace slade

// Define for neat console command definitions
#define CONSOLE_COMMAND(name, min_args, show_in_list)              \
	void           c_##name(const vector<string>& args);           \
	ConsoleCommand name(#name, &c_##name, min_args, show_in_list); \
	void           c_##name(const vector<string>& args)
