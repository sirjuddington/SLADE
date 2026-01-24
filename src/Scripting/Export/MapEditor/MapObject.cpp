
#include "Main.h"
#include "SLADEMap/MapObject/MapObject.h"
#include "Game/Configuration.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Scripting/Export/Export.h"
#include "Scripting/LuaBridge.h"
#include "Scripting/Scripting.h"

using namespace slade;

namespace slade::scripting
{
// -----------------------------------------------------------------------------
// Sets a boolean property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
static void objectSetBoolProperty(MapObject& self, string_view key, bool value)
{
	if (self.scriptCanModifyProp(key))
		self.setBoolProperty(key, value);
	else
		log::warning("{} boolean property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Sets an int property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
static void objectSetIntProperty(MapObject& self, string_view key, int value)
{
	if (self.scriptCanModifyProp(key))
		self.setIntProperty(key, value);
	else
		log::warning("{} integer property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Sets a float property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
static void objectSetFloatProperty(MapObject& self, string_view key, double value)
{
	if (self.scriptCanModifyProp(key))
		self.setFloatProperty(key, value);
	else
		log::warning("{} float property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Sets a string property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
static void objectSetStringProperty(MapObject& self, string_view key, string_view value)
{
	if (self.scriptCanModifyProp(key))
		self.setStringProperty(key, value);
	else
		log::warning("{} string property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Registers the MapVertex type with lua
// -----------------------------------------------------------------------------
void registerMapVertex(lua_State* lua)
{
	// Create MapVertex type, MapObject base class, no constructor
	auto lua_vertex = luabridge::getGlobalNamespace(lua).deriveClass<MapVertex, MapObject>("MapVertex");

	// Properties
	// -------------------------------------------------------------------------
	lua_vertex.addProperty("x", &MapVertex::xPos);
	lua_vertex.addProperty("y", &MapVertex::yPos);
	lua_vertex.addProperty("connectedLines", &MapVertex::connectedLines);
}

// -----------------------------------------------------------------------------
// Returns a table of needed textures for map line [self]
// -----------------------------------------------------------------------------
static luabridge::LuaRef lineVisibleTextures(MapLine& self)
{
	auto needs_tex = self.needsTexture();

	auto table           = luabridge::newTable(luaState());
	table["frontUpper"]  = (needs_tex & MapLine::Part::FrontUpper) != 0;
	table["frontMiddle"] = (needs_tex & MapLine::Part::FrontMiddle) != 0;
	table["frontLower"]  = (needs_tex & MapLine::Part::FrontLower) != 0;
	table["backUpper"]   = (needs_tex & MapLine::Part::BackUpper) != 0;
	table["backMiddle"]  = (needs_tex & MapLine::Part::BackMiddle) != 0;
	table["backLower"]   = (needs_tex & MapLine::Part::BackLower) != 0;

	return table;
}

// -----------------------------------------------------------------------------
// Returns true if the [flag] on map line [self] is set
// -----------------------------------------------------------------------------
static bool lineFlag(MapLine& self, const string& flag)
{
	if (game::configuration().lineBasicFlagSet(flag, &self, self.parentMap()->currentFormat()))
		return true;

	return game::configuration().lineFlagSet(flag, &self, self.parentMap()->currentFormat());
}

// -----------------------------------------------------------------------------
// Registers the MapLine type with lua
// -----------------------------------------------------------------------------
void registerMapLine(lua_State* lua)
{
	// Create MapLine type, MapObject base class, no constructor
	auto lua_line = luabridge::getGlobalNamespace(lua).deriveClass<MapLine, MapObject>("MapLine");

	// Properties
	// -------------------------------------------------------------------------
	lua_line.addProperty("x1", &MapLine::x1);
	lua_line.addProperty("y1", &MapLine::y1);
	lua_line.addProperty("x2", &MapLine::x2);
	lua_line.addProperty("y2", &MapLine::y2);
	lua_line.addProperty("vertex1", &MapLine::v1);
	lua_line.addProperty("vertex2", &MapLine::v2);
	lua_line.addProperty("side1", &MapLine::s1);
	lua_line.addProperty("side2", &MapLine::s2);
	lua_line.addProperty("special", &MapLine::special);
	lua_line.addProperty("length", [](MapLine& self) { return self.length(); });

	// Functions
	// -------------------------------------------------------------------------
	lua_line.addFunction("Flag", &lineFlag);
	lua_line.addFunction("Flip", &MapLine::flip, [](MapLine& self) { self.flip(true); });
	lua_line.addFunction("VisibleTextures", &lineVisibleTextures);
}

// -----------------------------------------------------------------------------
// Registers the MapSide type with lua
// -----------------------------------------------------------------------------
void registerMapSide(lua_State* lua)
{
	// Create MapSide type, MapObject base class, no constructor
	auto lua_side = luabridge::getGlobalNamespace(lua).deriveClass<MapSide, MapObject>("MapSide");

	// Properties
	// -------------------------------------------------------------------------
	lua_side.addProperty("sector", &MapSide::sector);
	lua_side.addProperty("line", &MapSide::parentLine);
	lua_side.addProperty("textureBottom", &MapSide::texLower);
	lua_side.addProperty("textureMiddle", &MapSide::texMiddle);
	lua_side.addProperty("textureTop", &MapSide::texUpper);
	lua_side.addProperty("offsetX", &MapSide::texOffsetX);
	lua_side.addProperty("offsetY", &MapSide::texOffsetY);
}

// -----------------------------------------------------------------------------
// Registers the MapSector type with lua
// -----------------------------------------------------------------------------
void registerMapSector(lua_State* lua)
{
	// Create MapSector type, MapObject base class, no constructor
	auto lua_sector = luabridge::getGlobalNamespace(lua).deriveClass<MapSector, MapObject>("MapSector");

	// Properties
	// -------------------------------------------------------------------------
	lua_sector.addProperty("textureFloor", [](MapSector& self) { return self.floor().texture; });
	lua_sector.addProperty("textureCeiling", [](MapSector& self) { return self.ceiling().texture; });
	lua_sector.addProperty("heightFloor", [](MapSector& self) { return self.floor().height; });
	lua_sector.addProperty("heightCeiling", [](MapSector& self) { return self.ceiling().height; });
	lua_sector.addProperty("lightLevel", &MapSector::lightLevel);
	lua_sector.addProperty("special", &MapSector::special);
	lua_sector.addProperty("id", &MapSector::id);
	lua_sector.addProperty("connectedSides", [](MapSector& self) { return self.connectedSides(); });
	lua_sector.addProperty(
		"colour",
		[](MapSector& self) { return self.parentMap()->mapSpecials().sectorColour(self, map::SectorPart::Interior); });
	lua_sector.addProperty("fogColour", [](MapSector& self) { return self.fogColour(); });
	lua_sector.addProperty("planeFloor", [](MapSector& self) { return self.floor().plane; });
	lua_sector.addProperty("planeCeiling", [](MapSector& self) { return self.ceiling().plane; });
	// TODO: bbox (need to export BBox struct first)

	// Functions
	// -------------------------------------------------------------------------
	lua_sector.addFunction("ContainsPoint", &MapSector::containsPoint);
}

// -----------------------------------------------------------------------------
// Returns true if [flag] is set on map thing [self]
// -----------------------------------------------------------------------------
static bool thingFlag(MapThing& self, const string& flag)
{
	if (game::configuration().thingBasicFlagSet(flag, &self, self.parentMap()->currentFormat()))
		return true;

	return game::configuration().thingFlagSet(flag, &self, self.parentMap()->currentFormat());
}

// -----------------------------------------------------------------------------
// Registers the MapThing type with lua
// -----------------------------------------------------------------------------
void registerMapThing(lua_State* lua)
{
	auto lua_thing = luabridge::getGlobalNamespace(lua).deriveClass<MapThing, MapObject>("MapThing");

	// Properties
	// -------------------------------------------------------------------------
	lua_thing.addProperty("x", &MapThing::xPos);
	lua_thing.addProperty("y", &MapThing::yPos);
	lua_thing.addProperty("type", &MapThing::type);
	lua_thing.addProperty("angle", &MapThing::angle);

	// Functions
	// -------------------------------------------------------------------------
	lua_thing.addFunction("Flag", &thingFlag);
	lua_thing.addFunction("SetAnglePoint", &MapThing::setAnglePoint);
}

// -----------------------------------------------------------------------------
// Registers the MapObject type with lua
// -----------------------------------------------------------------------------
void registerMapObject(lua_State* lua)
{
	auto lua_mapobject = luabridge::getGlobalNamespace(lua).beginClass<MapObject>("MapObject");

	// Properties
	// -------------------------------------------------------------------------
	lua_mapobject.addProperty("index", &MapObject::index);
	lua_mapobject.addProperty("type", &MapObject::objType);
	lua_mapobject.addProperty("typeName", &MapObject::typeName);
	// lua_mapobject["properties"] = sol::property(&MapObject::props); // TODO: Need to export MobjPropertyList first


	// Constants
	// -------------------------------------------------------------------------
	lua_mapobject.addStaticProperty("TYPE_OBJECT", [] { return static_cast<int>(MapObject::Type::Object); });
	lua_mapobject.addStaticProperty("TYPE_VERTEX", [] { return static_cast<int>(MapObject::Type::Vertex); });
	lua_mapobject.addStaticProperty("TYPE_LINE", [] { return static_cast<int>(MapObject::Type::Line); });
	lua_mapobject.addStaticProperty("TYPE_SIDE", [] { return static_cast<int>(MapObject::Type::Side); });
	lua_mapobject.addStaticProperty("TYPE_SECTOR", [] { return static_cast<int>(MapObject::Type::Sector); });
	lua_mapobject.addStaticProperty("TYPE_THING", [] { return static_cast<int>(MapObject::Type::Thing); });


	// Functions
	// -------------------------------------------------------------------------
	lua_mapobject.addFunction("HasProperty", &MapObject::hasProp);
	lua_mapobject.addFunction("BoolProperty", &MapObject::boolProperty);
	lua_mapobject.addFunction("IntProperty", &MapObject::intProperty);
	lua_mapobject.addFunction("FloatProperty", &MapObject::floatProperty);
	lua_mapobject.addFunction("StringProperty", &MapObject::stringProperty);
	lua_mapobject.addFunction("SetBoolProperty", &objectSetBoolProperty);
	lua_mapobject.addFunction("SetIntProperty", &objectSetIntProperty);
	lua_mapobject.addFunction("SetFloatProperty", &objectSetFloatProperty);
	lua_mapobject.addFunction("SetStringProperty", &objectSetStringProperty);
}
} // namespace slade::scripting
