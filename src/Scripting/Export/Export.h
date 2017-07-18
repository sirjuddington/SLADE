#pragma once

#include "Scripting/SolUtil.h"

namespace sol { class state; }

namespace Lua
{
	void	registerSLADENamespace(sol::state& lua);
	void	registerArchivesNamespace(sol::state& lua);
	void	registerGameNamespace(sol::state& lua);
	void	registerMapEditorNamespace(sol::state& lua);

	void	registerArchiveTypes(sol::state& lua);
	void	registerGameTypes(sol::state& lua);
	void	registerMapEditorTypes(sol::state& lua);
}
