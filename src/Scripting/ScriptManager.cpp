
#include "Main.h"
#include "ScriptManager.h"
#include "UI/ScriptManagerWindow.h"

namespace ScriptManager
{
	ScriptManagerWindow* window = nullptr;
}

void ScriptManager::open()
{
	if (!window)
		window = new ScriptManagerWindow();

	window->Show();
}
