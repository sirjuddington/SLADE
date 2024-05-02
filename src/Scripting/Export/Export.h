#pragma once

#include "thirdparty/sol/forward.hpp"

namespace slade::lua
{
// Namespaces ------------------------------------------------------------------
void registerAppNamespace(sol::state& lua);
void registerGameNamespace(sol::state& lua);
void registerArchivesNamespace(sol::state& lua);
void registerUINamespace(sol::state& lua);
void registerGraphicsNamespace(sol::state& lua);


// Types -----------------------------------------------------------------------
void registerMiscTypes(sol::state& lua);
void registerGameTypes(sol::state& lua);

// Archive
void registerArchive(sol::state& lua);
void registerArchiveEntry(sol::state& lua);
void registerEntryType(sol::state& lua);
void registerArchiveTypes(sol::state& lua);

// Graphics
void registerImageConvertOptionsType(sol::state& lua);
void registerImageFormatType(sol::state& lua);
void registerImageDrawOptionsType(sol::state& lua);
void registerImageType(sol::state& lua);
void registerCTexturePatchTypes(sol::state& lua);
void registerCTextureType(sol::state& lua);
void registerPatchTableType(sol::state& lua);
void registerTextureXListType(sol::state& lua);
void registerTranslationType(sol::state& lua);
void registerGraphicsTypes(sol::state& lua);

// MapEditor
void registerMapObject(sol::state& lua);
void registerMapVertex(sol::state& lua);
void registerMapLine(sol::state& lua);
void registerMapSide(sol::state& lua);
void registerMapSector(sol::state& lua);
void registerMapThing(sol::state& lua);
void registerMapEditorTypes(sol::state& lua);
} // namespace slade::lua
