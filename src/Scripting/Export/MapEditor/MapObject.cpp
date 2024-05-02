
#include "Main.h"
#include "SLADEMap/MapObject/MapObject.h"
#include "Game/Configuration.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/SLADEMap.h"
#include "Scripting/Lua.h"
#include "Utility/ColRGBA.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;

namespace slade::lua
{
// -----------------------------------------------------------------------------
// Sets a boolean property [key] on the MapObject [self] to [value].
// Also checks if the property [key] is allowed to be modified by scripts
// -----------------------------------------------------------------------------
void objectSetBoolProperty(MapObject& self, string_view key, bool value)
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
void objectSetIntProperty(MapObject& self, string_view key, int value)
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
void objectSetFloatProperty(MapObject& self, string_view key, double value)
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
void objectSetStringProperty(MapObject& self, string_view key, string_view value)
{
	if (self.scriptCanModifyProp(key))
		self.setStringProperty(key, value);
	else
		log::warning("{} string property \"{}\" can not be modified via script", self.typeName(), key);
}

// -----------------------------------------------------------------------------
// Registers the MapVertex type with lua
// -----------------------------------------------------------------------------
void registerMapVertex(sol::state& lua)
{
	// Create MapVertex type, MapObject base class, no constructor
	auto lua_vertex = lua.new_usertype<MapVertex>(
		"MapVertex", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_vertex.set("x", &MapVertex::xPos);
	lua_vertex.set("y", &MapVertex::yPos);
	lua_vertex.set("connectedLines", &MapVertex::connectedLines);
}

// -----------------------------------------------------------------------------
// Returns a table of needed textures for map line [self]
// -----------------------------------------------------------------------------
sol::table lineVisibleTextures(MapLine& self)
{
	auto needs_tex = self.needsTexture();
	return lua::state().create_table_with(
		"frontUpper",
		(needs_tex & MapLine::Part::FrontUpper) != 0,
		"frontMiddle",
		(needs_tex & MapLine::Part::FrontMiddle) != 0,
		"frontLower",
		(needs_tex & MapLine::Part::FrontLower) != 0,
		"backUpper",
		(needs_tex & MapLine::Part::BackUpper) != 0,
		"backMiddle",
		(needs_tex & MapLine::Part::BackMiddle) != 0,
		"backLower",
		(needs_tex & MapLine::Part::BackLower) != 0);
}

// -----------------------------------------------------------------------------
// Returns true if the [flag] on map line [self] is set
// -----------------------------------------------------------------------------
bool lineFlag(MapLine& self, const string& flag)
{
	if (game::configuration().lineBasicFlagSet(flag, &self, self.parentMap()->currentFormat()))
		return true;

	return game::configuration().lineFlagSet(flag, &self, self.parentMap()->currentFormat());
}

// -----------------------------------------------------------------------------
// Registers the MapLine type with lua
// -----------------------------------------------------------------------------
void registerMapLine(sol::state& lua)
{
	// Create MapLine type, MapObject base class, no constructor
	auto lua_line = lua.new_usertype<MapLine>(
		"MapLine", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_line.set("x1", sol::property(&MapLine::x1));
	lua_line.set("y1", sol::property(&MapLine::y1));
	lua_line.set("x2", sol::property(&MapLine::x2));
	lua_line.set("y2", sol::property(&MapLine::y2));
	lua_line.set("vertex1", sol::property(&MapLine::v1));
	lua_line.set("vertex2", sol::property(&MapLine::v2));
	lua_line.set("side1", sol::property(&MapLine::s1));
	lua_line.set("side2", sol::property(&MapLine::s2));
	lua_line.set("special", sol::property(&MapLine::special));
	lua_line.set("length", sol::property(&MapLine::length));

	// Functions
	// -------------------------------------------------------------------------
	lua_line.set_function("Flag", &lineFlag);
	lua_line.set_function("Flip", sol::overload(&MapLine::flip, [](MapLine& self) { self.flip(true); }));
	lua_line.set_function("VisibleTextures", &lineVisibleTextures);
}

// -----------------------------------------------------------------------------
// Registers the MapSide type with lua
// -----------------------------------------------------------------------------
void registerMapSide(sol::state& lua)
{
	// Create MapSide type, MapObject base class, no constructor
	auto lua_side = lua.new_usertype<MapSide>(
		"MapSide", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_side.set("sector", sol::property(&MapSide::sector));
	lua_side.set("line", sol::property(&MapSide::parentLine));
	lua_side.set("textureBottom", sol::property(&MapSide::texLower));
	lua_side.set("textureMiddle", sol::property(&MapSide::texMiddle));
	lua_side.set("textureTop", sol::property(&MapSide::texUpper));
	lua_side.set("offsetX", sol::property(&MapSide::texOffsetX));
	lua_side.set("offsetY", sol::property(&MapSide::texOffsetY));
}

// -----------------------------------------------------------------------------
// Registers the MapSector type with lua
// -----------------------------------------------------------------------------
void registerMapSector(sol::state& lua)
{
	// Create MapSector type, MapObject base class, no constructor
	auto lua_sector = lua.new_usertype<MapSector>(
		"MapSector", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_sector.set("textureFloor", sol::property([](MapSector& self) { return self.floor().texture; }));
	lua_sector.set("textureCeiling", sol::property([](MapSector& self) { return self.ceiling().texture; }));
	lua_sector.set("heightFloor", sol::property([](MapSector& self) { return self.floor().height; }));
	lua_sector.set("heightCeiling", sol::property([](MapSector& self) { return self.ceiling().height; }));
	lua_sector.set("lightLevel", sol::property(&MapSector::lightLevel));
	lua_sector.set("special", sol::property(&MapSector::special));
	lua_sector.set("id", sol::property(&MapSector::tag));
	lua_sector.set("connectedSides", sol::property([](MapSector& self) { return self.connectedSides(); }));
	lua_sector.set("colour", sol::property(&MapSector::colourAt));
	lua_sector.set("fogColour", sol::property(&MapSector::fogColour));
	lua_sector.set("planeFloor", sol::property([](MapSector& self) { return self.floor().plane; }));
	lua_sector.set("planeCeiling", sol::property([](MapSector& self) { return self.ceiling().plane; }));
	// TODO: bbox (need to export BBox struct first)

	// Functions
	// -------------------------------------------------------------------------
	lua_sector.set_function("ContainsPoint", &MapSector::containsPoint);
}

// -----------------------------------------------------------------------------
// Returns true if [flag] is set on map thing [self]
// -----------------------------------------------------------------------------
bool thingFlag(MapThing& self, const string& flag)
{
	if (game::configuration().thingBasicFlagSet(flag, &self, self.parentMap()->currentFormat()))
		return true;

	return game::configuration().thingFlagSet(flag, &self, self.parentMap()->currentFormat());
}

// -----------------------------------------------------------------------------
// Registers the MapThing type with lua
// -----------------------------------------------------------------------------
void registerMapThing(sol::state& lua)
{
	auto lua_thing = lua.new_usertype<MapThing>(
		"MapThing", sol::base_classes, sol::bases<MapObject>(), "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_thing.set("x", sol::property(&MapThing::xPos));
	lua_thing.set("y", sol::property(&MapThing::yPos));
	lua_thing.set("type", sol::property(&MapThing::type));
	lua_thing.set("angle", sol::property(&MapThing::angle));

	// Functions
	// -------------------------------------------------------------------------
	lua_thing.set_function("Flag", &thingFlag);
	lua_thing.set_function("SetAnglePoint", &MapThing::setAnglePoint);
}

// -----------------------------------------------------------------------------
// Registers the MapObject type with lua
// -----------------------------------------------------------------------------
void registerMapObject(sol::state& lua)
{
	auto lua_mapobject = lua.new_usertype<MapObject>("MapObject", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_mapobject.set("index", sol::property(&MapObject::index));
	lua_mapobject.set("type", sol::property(&MapObject::objType));
	lua_mapobject.set("typeName", sol::property(&MapObject::typeName));
	// lua_mapobject["properties"] = sol::property(&MapObject::props); // TODO: Need to export MobjPropertyList first


	// Constants
	// -------------------------------------------------------------------------
	lua_mapobject.set("TYPE_OBJECT", sol::property([]() { return MapObject::Type::Object; }));
	lua_mapobject.set("TYPE_VERTEX", sol::property([]() { return MapObject::Type::Vertex; }));
	lua_mapobject.set("TYPE_LINE", sol::property([]() { return MapObject::Type::Line; }));
	lua_mapobject.set("TYPE_SIDE", sol::property([]() { return MapObject::Type::Side; }));
	lua_mapobject.set("TYPE_SECTOR", sol::property([]() { return MapObject::Type::Sector; }));
	lua_mapobject.set("TYPE_THING", sol::property([]() { return MapObject::Type::Thing; }));


	// Functions
	// -------------------------------------------------------------------------
	lua_mapobject.set_function("HasProperty", &MapObject::hasProp);
	lua_mapobject.set_function("BoolProperty", &MapObject::boolProperty);
	lua_mapobject.set_function("IntProperty", &MapObject::intProperty);
	lua_mapobject.set_function("FloatProperty", &MapObject::floatProperty);
	lua_mapobject.set_function("StringProperty", &MapObject::stringProperty);
	lua_mapobject.set_function("SetBoolProperty", &objectSetBoolProperty);
	lua_mapobject.set_function("SetIntProperty", &objectSetIntProperty);
	lua_mapobject.set_function("SetFloatProperty", &objectSetFloatProperty);
	lua_mapobject.set_function("SetStringProperty", &objectSetStringProperty);
}
} // namespace slade::lua
