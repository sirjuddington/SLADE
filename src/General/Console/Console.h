
#ifndef	__CONSOLE_H__
#define	__CONSOLE_H__

class ConsoleCommand
{
public:
	ConsoleCommand(string name, void(*commandFunc)(vector<string>), int min_args, bool show_in_list = true);

	~ConsoleCommand() {}

	string	getName() { return name; }
	bool	showInList() { return show_in_list; }
	void	execute(vector<string> args);
	size_t	minArgs() { return min_args; }

	inline bool operator<(ConsoleCommand c) const { return name < c.getName(); }
	inline bool operator>(ConsoleCommand c) const { return name > c.getName(); }

private:
	string	name;
	void(*commandFunc)(vector<string>);
	size_t	min_args;
	bool	show_in_list;
};

class Console
{
public:
	Console();
	~Console();

	int numCommands() { return (int) commands.size(); }
	ConsoleCommand& command(size_t index);

	void			addCommand(ConsoleCommand& c);
	void			execute(string command);
	string			lastCommand();
	string			prevCommand(int index);
	int				numPrevCommands() { return cmd_log.size(); }

private:
	vector<ConsoleCommand>	commands;
	vector<string>			cmd_log;
};

// Define for neat console command definitions
#define CONSOLE_COMMAND(name, min_args, show_in_list) \
	void c_##name(vector<string> args); \
	ConsoleCommand name(#name, &c_##name, min_args, show_in_list); \
	void c_##name(vector<string> args)

#endif //__CONSOLE_H__
