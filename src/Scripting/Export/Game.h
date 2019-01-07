
void registerThingType(sol::state& lua)
{
	lua.new_usertype<Game::ThingType>(
		"ThingType",

		// No constructor
		"new",
		sol::no_constructor,


		// Properties
		// ---------------------------------------------------------------------------
		"name",
		sol::property(&Game::ThingType::name),

		"group",
		sol::property(&Game::ThingType::group),

		"radius",
		sol::property(&Game::ThingType::radius),

		"height",
		sol::property(&Game::ThingType::height),

		"scaleY",
		sol::property(&Game::ThingType::scaleY),

		"scaleX",
		sol::property(&Game::ThingType::scaleX),

		"angled",
		sol::property(&Game::ThingType::angled),

		"hanging",
		sol::property(&Game::ThingType::hanging),

		"fullbright",
		sol::property(&Game::ThingType::fullbright),

		"decoration",
		sol::property(&Game::ThingType::decoration),

		"solid",
		sol::property(&Game::ThingType::solid),

		//"tagged",
		// sol::property(&Game::ThingType::needsTag),

		"sprite",
		sol::property(&Game::ThingType::sprite),

		"icon",
		sol::property(&Game::ThingType::icon),

		"translation",
		sol::property(&Game::ThingType::translation),

		"palette",
		sol::property(&Game::ThingType::palette));
}

void registerGameNamespace(sol::state& lua)
{
	auto game = lua.create_table("Game");
	game.set_function("thingType", [](int type) { return Game::configuration().thingType(type); });
}

void registerGameTypes(sol::state& lua)
{
	registerThingType(lua);
}
