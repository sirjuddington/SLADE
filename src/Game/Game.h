#pragma once

enum class MapFormat;
class ParseTreeNode;

namespace Game
{
class Configuration;

// Structs
struct GameDef
{
	std::string               name;
	std::string               title;
	std::string               filename;
	std::map<MapFormat, bool> supported_formats;
	bool                      user;
	vector<std::string>       filters;

	GameDef(std::string_view def_name = "Unknown") : name{ def_name }, user{ true } {}

	bool operator>(const GameDef& right) const { return title > right.title; }
	bool operator<(const GameDef& right) const { return title < right.title; }

	bool parse(MemChunk& mc);
	bool supportsFilter(std::string_view filter) const;
};
struct PortDef
{
	std::string               name;
	std::string               title;
	std::string               filename;
	std::map<MapFormat, bool> supported_formats;
	vector<std::string>       supported_games;
	bool                      user;

	PortDef(std::string_view def_name = "Unknown") : name{ def_name }, user{ true } {}

	bool operator>(const PortDef& right) const { return title > right.title; }
	bool operator<(const PortDef& right) const { return title < right.title; }

	bool parse(MemChunk& mc);
	bool supportsGame(std::string_view game) const { return VECTOR_EXISTS(supported_games, game); }
};

// Enums
enum class TagType
{
	None = 0,
	Sector,
	Line,
	Thing,
	Back,
	SectorOrBack,
	SectorAndBack,

	// Special handling for that one
	LineId,
	LineIdHi5,

	// Some more specific types
	Thing1Sector2,                // most ZDoom teleporters work like this
	Thing1Sector3,                // Teleport_NoFog & Thing_Destroy
	Thing1Thing2,                 // TeleportOther, NoiseAlert, Thing_Move, Thing_SetGoal
	Thing1Thing4,                 // Thing_ProjectileIntercept, Thing_ProjectileAimed
	Thing1Thing2Thing3,           // TeleportGroup
	Sector1Thing2Thing3Thing5,    // TeleportInSector
	LineId1Line2,                 // Teleport_Line
	LineNegative,                 // Scroll_Texture_Both
	Thing4,                       // ThrustThing
	Thing5,                       // Radius_Quake
	Line1Sector2,                 // Sector_Attach3dMidtex
	Sector1Sector2,               // Sector_SetLink
	Sector1Sector2Sector3Sector4, // Plane_Copy
	Sector2Is3Line,               // Static_Init
	Sector1Thing2,                // PointPush_SetForce

	Patrol,
	Interpolation
};

// General
void init();

// Basic Game/Port Definitions
const std::map<std::string, GameDef>& gameDefs();
const GameDef&                        gameDef(const std::string& id);
const std::map<std::string, PortDef>& portDefs();
const PortDef&                        portDef(const std::string& id);

bool mapFormatSupported(MapFormat format, const std::string& game, const std::string& port = "");

// Full Game Configuration
Configuration& configuration();

// Tagging
TagType parseTagged(ParseTreeNode* tagged);

// Custom definitions (ZScript, DECORATE, EDF, etc.)
void updateCustomDefinitions();

} // namespace Game
