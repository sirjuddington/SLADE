
#include "Main.h"
#include "Game.h"
#include "Configuration.h"

namespace Game
{
	Configuration	config_current;
}

Configuration& Game::configuration()
{
	return config_current;
}
