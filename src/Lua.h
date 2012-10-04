
#ifndef __LUA_H__
#define __LUA_H__

namespace Lua {
	bool	init();
	void	close();
	bool	run(string program);
}

#endif//__LUA_H__
