
#ifndef	__CONSOLE_H__
#define	__CONSOLE_H__

#include "ListenerAnnouncer.h"

class ConsoleCommand {
private:
	string name;
	void (*commandFunc)(vector<string>);
	size_t min_args;

public:
	ConsoleCommand(string name, void(*commandFunc)(vector<string>), int min_args);

	~ConsoleCommand() {}

	string getName() { return name; }
	void execute(vector<string> args);

	inline bool operator<(ConsoleCommand c) const { return name < c.getName(); }
	inline bool operator>(ConsoleCommand c) const { return name > c.getName(); }
};

class Console : public Announcer {
private:
	vector<ConsoleCommand> commands;

	vector<string> log;
	vector<string> cmd_log;

	static Console* instance;

public:
	Console();
	~Console();

	static Console* getInstance() {
		if (!instance)
			instance = new Console();

		return instance;
	}

	static void deleteInstance() {
		if (instance) delete instance;
	}

	int numCommands() { return (int) commands.size(); }
	ConsoleCommand& command(size_t index);

	void	addCommand(ConsoleCommand &c);
	void	execute(string command);
	void	logMessage(string message);
	string	lastLogLine();
	string	lastCommand();
	string	dumpLog();
	string	prevCommand(int index);
	int		numPrevCommands() { return cmd_log.size(); }
};

// Define for less cumbersome Console::getInstance()
#define theConsole Console::getInstance()

// Define for neat console command definitions
#define CONSOLE_COMMAND(name, min_args) \
	void c_##name(vector<string> args); \
	ConsoleCommand name(#name, &c_##name, min_args); \
	void c_##name(vector<string> args)

#endif //__CONSOLE_H__
