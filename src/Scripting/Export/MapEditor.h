

void objectSetBoolProperty(MapObject& self, const string& key, bool value)
{
	if (self.scriptCanModifyProp(key))
		self.setBoolProperty(key, value);
	else
		Log::warning(1, S_FMT(
			"%s boolean property \"%s\" can not be modified via script",
			CHR(self.getTypeName()),
			key
		));
}

void objectSetIntProperty(MapObject& self, const string& key, int value)
{
	if (self.scriptCanModifyProp(key))
		self.setIntProperty(key, value);
	else
		Log::warning(1, S_FMT(
			"%s integer property \"%s\" can not be modified via script",
			CHR(self.getTypeName()),
			key
		));
}

void objectSetFloatProperty(MapObject& self, const string& key, double value)
{
	if (self.scriptCanModifyProp(key))
		self.setFloatProperty(key, value);
	else
		Log::warning(1, S_FMT(
			"%s float property \"%s\" can not be modified via script",
			CHR(self.getTypeName()),
			key
		));
}

void objectSetStringProperty(MapObject& self, const string& key, const string& value)
{
	if (self.scriptCanModifyProp(key))
		self.setStringProperty(key, value);
	else
		Log::warning(1, S_FMT(
			"%s string property \"%s\" can not be modified via script",
			CHR(self.getTypeName()),
			key
		));
}

void registerSLADEMap(sol::state& lua)
{
	lua.new_usertype<SLADEMap>(
		"Map",

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"name",				sol::property(&SLADEMap::mapName),
		"udmfNamespace",	sol::property(&SLADEMap::udmfNamespace),
		"vertices",			sol::property(&SLADEMap::vertices),
		"linedefs",			sol::property(&SLADEMap::lines),
		"sidedefs",			sol::property(&SLADEMap::sides),
		"sectors",			sol::property(&SLADEMap::sectors),
		"things",			sol::property(&SLADEMap::things)
	);
}

void selectMapObject(MapEditContext& self, MapObject* object, bool select)
{
	if (object)
		self.selection().select(
			{ (int)object->getIndex(), MapEditor::itemTypeFromObject(object) },
			select
		);
}

void setEditMode(
	MapEditContext& self,
	MapEditor::Mode mode,
	MapEditor::SectorMode sector_mode = MapEditor::SectorMode::Both)
{
	self.setEditMode(mode);
	if (mode == MapEditor::Mode::Sectors)
		self.setSectorEditMode(sector_mode);
}

void registerMapEditor(sol::state& lua)
{
	lua.new_usertype<MapEditContext>(
		"MapEditor",

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"editMode",			sol::property(&MapEditContext::editMode),
		"sectorEditMode",	sol::property(&MapEditContext::sectorEditMode),
		"gridSize",			sol::property(&MapEditContext::gridSize),
		"map",				sol::property(&MapEditContext::map),

		// Functions
		"selectedVertices", sol::overload(
			[](MapEditContext& self, bool try_hilight)
			{ return self.selection().selectedVertices(try_hilight); },
			[](MapEditContext& self)
			{ return self.selection().selectedVertices(false); }
		),

		"selectedLines", sol::overload(
			[](MapEditContext& self, bool try_hilight)
			{ return self.selection().selectedLines(try_hilight); },
			[](MapEditContext& self)
			{ return self.selection().selectedLines(false); }
		),

		"selectedSectors", sol::overload(
			[](MapEditContext& self, bool try_hilight)
			{ return self.selection().selectedSectors(try_hilight); },
			[](MapEditContext& self)
			{ return self.selection().selectedSectors(false); }
		),

		"selectedThings", sol::overload(
			[](MapEditContext& self, bool try_hilight)
			{ return self.selection().selectedThings(try_hilight); },
			[](MapEditContext& self)
			{ return self.selection().selectedThings(false); }
		),

		"clearSelection", [](MapEditContext& self) { self.selection().clear(); },

		"select", sol::overload(
			&selectMapObject,
			[](MapEditContext& self, MapObject* object) { selectMapObject(self, object, true); }
		),

		"setEditMode", sol::overload(
			[](MapEditContext& self, MapEditor::Mode mode)
			{ setEditMode(self, mode); },
			[](MapEditContext& self, MapEditor::Mode mode, MapEditor::SectorMode sector_mode)
			{ setEditMode(self, mode, sector_mode); }
		)
	);

	// MapEditor enums
	sol::table mapeditor = lua["MapEditor"];
	mapeditor.new_enum(
		"Mode",
		"Vertices",	MapEditor::Mode::Vertices,
		"Lines",	MapEditor::Mode::Lines,
		"Sectors",	MapEditor::Mode::Sectors,
		"Things",	MapEditor::Mode::Things,
		"Visual",	MapEditor::Mode::Visual
	);
	mapeditor.new_enum(
		"SectorMode",
		"Both",		MapEditor::SectorMode::Both,
		"Floor",	MapEditor::SectorMode::Floor,
		"Ceiling",	MapEditor::SectorMode::Ceiling
	);
}

void registerMapVertex(sol::state& lua)
{
	lua.new_usertype<MapVertex>(
		"MapVertex",
		sol::base_classes, sol::bases<MapObject>(),

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"x",				&MapVertex::xPos,
		"y",				&MapVertex::yPos,
		"connectedLines",	&MapVertex::connectedLines
	);
}

sol::table lineVisibleTextures(MapLine& self)
{
	auto needs_tex = self.needsTexture();
	return Lua::state().create_table_with(
		"frontUpper", (needs_tex & TEX_FRONT_UPPER) != 0,
		"frontMiddle", (needs_tex & TEX_FRONT_MIDDLE) != 0,
		"frontLower", (needs_tex & TEX_FRONT_LOWER) != 0,
		"backUpper", (needs_tex & TEX_BACK_UPPER) != 0,
		"backMiddle", (needs_tex & TEX_BACK_MIDDLE) != 0,
		"backLower", (needs_tex & TEX_BACK_LOWER) != 0
	);
}

bool lineFlag(MapLine& self, const string& flag)
{
	if (Game::configuration().lineBasicFlagSet(flag, &self, self.getParentMap()->currentFormat()))
		return true;

	return Game::configuration().lineFlagSet(flag, &self, self.getParentMap()->currentFormat());
}

void registerMapLine(sol::state& lua)
{
	lua.new_usertype<MapLine>(
		"MapLine",
		sol::base_classes, sol::bases<MapObject>(),

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"x1",				sol::property(&MapLine::x1),
		"y1",				sol::property(&MapLine::y1),
		"x2",				sol::property(&MapLine::x2),
		"y2",				sol::property(&MapLine::y2),
		"vertex1",			sol::property(&MapLine::v1),
		"vertex2",			sol::property(&MapLine::v2),
		"side1",			sol::property(&MapLine::s1),
		"side2",			sol::property(&MapLine::s2),
		"special",			sol::property(&MapLine::getSpecial),
		"length",			sol::property(&MapLine::getLength),
		"visibleTextures",	sol::property(&lineVisibleTextures),

		// Functions
		"flag",	&lineFlag,
		"flip", sol::overload(
			&MapLine::flip,
			[](MapLine& self) { self.flip(true); }
		)
	);
}

void registerMapSide(sol::state& lua)
{
	lua.new_usertype<MapSide>(
		"MapSide",
		sol::base_classes, sol::bases<MapObject>(),

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"sector",			sol::property(&MapSide::getSector),
		"line",				sol::property(&MapSide::getParentLine),
		"textureBottom",	sol::property(&MapSide::getTexLower),
		"textureMiddle",	sol::property(&MapSide::getTexMiddle),
		"textureTop",		sol::property(&MapSide::getTexUpper),
		"offsetX",			sol::property(&MapSide::getOffsetX),
		"offsetY",			sol::property(&MapSide::getOffsetY)
	);
}

void registerMapSector(sol::state& lua)
{
	lua.new_usertype<MapSector>(
		"MapSector",
		sol::base_classes, sol::bases<MapObject>(),

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"textureFloor",		sol::property(&MapSector::getFloorTex),
		"textureCeiling",	sol::property(&MapSector::getCeilingTex),
		"heightFloor",		sol::property(&MapSector::getFloorHeight),
		"heightCeiling",	sol::property(&MapSector::getCeilingHeight),
		"lightLevel",		sol::property(&MapSector::getLightLevel),
		"special",			sol::property(&MapSector::getSpecial),
		"id",				sol::property(&MapSector::getTag),
		"connectedSides",	sol::property(&MapSector::connectedSides),
		"colour",			sol::property(&MapSector::getColour),
		"fogColour",		sol::property(&MapSector::getFogColour),
		"planeFloor",		sol::property(&MapSector::getFloorPlane),
		"planeCeiling",		sol::property(&MapSector::getCeilingPlane),
		// bbox (need to export bbox_t first)

		// Functions
		"containsPoint", &MapSector::isWithin
	);
}

bool thingFlag(MapThing& self, const string& flag)
{
	if (Game::configuration().thingBasicFlagSet(flag, &self, self.getParentMap()->currentFormat()))
		return true;

	return Game::configuration().thingFlagSet(flag, &self, self.getParentMap()->currentFormat());
}

void registerMapThing(sol::state& lua)
{
	lua.new_usertype<MapThing>(
		"MapThing",
		sol::base_classes, sol::bases<MapObject>(),

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"x",		sol::property(&MapThing::xPos),
		"y",		sol::property(&MapThing::yPos),
		"type",		sol::property(&MapThing::getType),
		"angle",	sol::property(&MapThing::getAngle),

		// Functions
		"flag",				&thingFlag,
		"setAnglePoint",	&MapThing::setAnglePoint
	);
}

void registerMapObject(sol::state& lua)
{
	lua.new_usertype<MapObject>(
		"MapObject",

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"index",	sol::property(&MapObject::getIndex),
		"typeName",	sol::property(&MapObject::getTypeName),
		//"properties", sol::property(&MapObject::props), // Need to export MobjPropertyList first

		// Functions
		"hasProperty",			&MapObject::hasProp,
		"boolProperty",			&MapObject::boolProperty,
		"intProperty",			&MapObject::intProperty,
		"floatProperty",		&MapObject::floatProperty,
		"stringProperty",		&MapObject::stringProperty,
		"setBoolProperty",		&objectSetBoolProperty,
		"setIntProperty",		&objectSetIntProperty,
		"setFloatProperty",		&objectSetFloatProperty,
		"setStringProperty",	&objectSetStringProperty
	);
}

void registerMapEditorTypes(sol::state& lua)
{
	registerMapEditor(lua);
	registerSLADEMap(lua);
	registerMapObject(lua);
	registerMapVertex(lua);
	registerMapLine(lua);
	registerMapSide(lua);
	registerMapSector(lua);
	registerMapThing(lua);
}
