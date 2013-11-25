
#ifndef __EXECUTABLES_H__
#define __EXECUTABLES_H__

class Parser;
namespace Executables
{
	struct game_exe_t
	{
		string				id;
		string				name;
		string				exe_name;
		string				path;
		vector<key_value_t>	configs;
	};

	game_exe_t*		getGameExe(string id);
	game_exe_t*		getGameExe(unsigned index);
	unsigned		nGameExes();
	void			setExePath(string id, string path);
	string			writePaths();
	string			writeExecutables();

	void	init();
	void	parse(Parser* p);
	void	addGameExe(string name);
}

#endif//__EXECUTABLES_H__
