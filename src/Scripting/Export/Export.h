#pragma once

struct lua_State;

namespace slade::scripting
{
// Namespaces ------------------------------------------------------------------
void registerAppNamespace(lua_State* lua);
void registerGameNamespace(lua_State* lua);
void registerArchivesNamespace(lua_State* lua);
void registerUINamespace(lua_State* lua);
void registerGraphicsNamespace(lua_State* lua);


// Types -----------------------------------------------------------------------
void registerMiscTypes(lua_State* lua);
void registerGameTypes(lua_State* lua);

// Archive
void registerArchive(lua_State* lua);
void registerArchiveEntry(lua_State* lua);
void registerEntryType(lua_State* lua);
void registerArchiveTypes(lua_State* lua);

// Graphics
void registerImageType(lua_State* lua);
void registerCTexturePatchTypes(lua_State* lua);
void registerCTextureType(lua_State* lua);
void registerPatchTableType(lua_State* lua);
void registerTextureXListType(lua_State* lua);
void registerTranslationType(lua_State* lua);
void registerGraphicsTypes(lua_State* lua);

// MapEditor
void registerMapObject(lua_State* lua);
void registerMapVertex(lua_State* lua);
void registerMapLine(lua_State* lua);
void registerMapSide(lua_State* lua);
void registerMapSector(lua_State* lua);
void registerMapThing(lua_State* lua);
void registerMapEditorTypes(lua_State* lua);
} // namespace slade::scripting
