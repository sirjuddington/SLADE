#pragma once

#include "thirdparty/sol/forward.hpp"

namespace slade::lua
{
// Namespaces
void registerAppNamespace(sol::state& lua);
void registerGameNamespace(sol::state& lua);
void registerArchivesNamespace(sol::state& lua);
void registerUINamespace(sol::state& lua);

// Types
void registerMiscTypes(sol::state& lua);
void registerArchiveTypes(sol::state& lua);
void registerMapEditorTypes(sol::state& lua);
void registerGameTypes(sol::state& lua);
void registerGraphicsTypes(sol::state& lua);
} // namespace slade::lua
