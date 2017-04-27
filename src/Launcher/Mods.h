#pragma once

namespace Launcher
{
	class Mod
	{
	public:
		Mod(const string& path);
		~Mod();

		bool scanFile();

	private:
		string		path_;
		string		title_;
		uint32_t	crc_;
	};

	class ModLibrary
	{
	public:
		ModLibrary();
		~ModLibrary();

		const vector<Mod>&	mods() const { return mods_; }

		// Testing
		void scanDir(const string& path);

	private:
		vector<Mod>	mods_;
	};
}
