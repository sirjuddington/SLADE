#pragma once

class ConsoleCommand
{
public:
	ConsoleCommand(string name, void (*command_func)(vector<string>), int min_args, bool show_in_list = true);
	~ConsoleCommand() {}

	string getName() { return name_; }
	bool   showInList() { return show_in_list_; }
	void   execute(vector<string> args);
	size_t minArgs() { return min_args_; }

	inline bool operator<(ConsoleCommand c) const { return name_ < c.getName(); }
	inline bool operator>(ConsoleCommand c) const { return name_ > c.getName(); }

private:
	string name_;
	void (*command_func_)(vector<string>);
	size_t min_args_;
	bool   show_in_list_;
};

class Console
{
public:
	Console();
	~Console();

	int             numCommands() { return (int)commands_.size(); }
	ConsoleCommand& command(size_t index);

	void   addCommand(ConsoleCommand& c);
	void   execute(string command);
	string lastCommand();
	string prevCommand(int index);
	int    numPrevCommands() { return cmd_log_.size(); }

private:
	vector<ConsoleCommand> commands_;
	vector<string>         cmd_log_;
};

// Define for neat console command definitions
#define CONSOLE_COMMAND(name, min_args, show_in_list)              \
	void           c_##name(vector<string> args);                  \
	ConsoleCommand name(#name, &c_##name, min_args, show_in_list); \
	void           c_##name(vector<string> args)
