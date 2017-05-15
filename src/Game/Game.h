#pragma once

namespace Game
{
	class Configuration;

	// Structs
	struct GameDef
	{
		string			name;
		string			title;
		string			filename;
		bool			supported_formats[4];
		bool			user;
		vector<string>	filters;

		GameDef(const string& def_name = "Unknown") :
			name{ def_name },
			supported_formats{ false, false, false, false },
			user{ true } {}

		bool operator>(const GameDef& right) const { return title > right.title; }
		bool operator<(const GameDef& right) const { return title < right.title; }

		bool	parse(MemChunk& mc);
		bool	supportsFilter(const string& filter) const;
	};
	struct PortDef
	{
		string			name;
		string			title;
		string			filename;
		bool			supported_formats[4];
		vector<string>	supported_games;
		bool			user;

		PortDef(const string& def_name = "Unknown") :
			name{ def_name },
			supported_formats{ false, false, false, false },
			user{ true } {}

		bool operator>(const PortDef& right) const { return title > right.title; }
		bool operator<(const PortDef& right) const { return title < right.title; }

		bool	parse(MemChunk& mc);
		bool	supportsGame(const string& game) const { return VECTOR_EXISTS(supported_games, game); }
	};

	// General
	void	init();

	// Basic Game/Port Definitions
	const std::map<string, GameDef>&	gameDefs();
	const GameDef&						gameDef(const string& id);
	const std::map<string, PortDef>&	portDefs();
	const PortDef&						portDef(const string& id);

	bool	mapFormatSupported(int format, const string& game, const string& port = "");

	// Full Game Configuration
	Configuration&	configuration();
}
