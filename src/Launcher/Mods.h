#pragma once

namespace Launcher
{
	class Mod
	{
	public:
		Mod();
		~Mod();

	private:
		string	path;
		string	title;
	};

	class ModLibrary
	{
	public:
		ModLibrary();
		~ModLibrary();

	private:
		vector<Mod>	mods;
	};
}
