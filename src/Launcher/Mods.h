#pragma once

namespace Launcher
{
	class Mod
	{
	public:
		Mod();
		~Mod();

	private:
		string	path_;
		string	title_;
	};

	class ModLibrary
	{
	public:
		ModLibrary();
		~ModLibrary();

	private:
		vector<Mod>	mods_;
	};
}
