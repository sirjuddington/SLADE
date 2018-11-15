#pragma once

// No #includes, they're a headache
class Archive;
class ArchivePanel;
class GfxEntryPanel;

// Console helpers
namespace CH
{
Archive*       getCurrentArchive();
ArchivePanel*  getCurrentArchivePanel();
GfxEntryPanel* getCurrentGfxPanel();
} // namespace CH
