#pragma once

#include "Archive/Archive.h"

namespace slade::archiveoperations
{
bool removeUnusedPatches(Archive* archive);
bool checkDuplicateEntryNames(Archive* archive);
bool checkDuplicateEntryContent(Archive* archive);
void removeUnusedTextures(Archive* archive);
void removeUnusedFlats(Archive* archive);
bool removeUnusedZDoomTextures(Archive* archive);
void removeEntriesUnchangedFromIWAD(Archive* archive);

// Search and replace in maps
size_t replaceThings(Archive* archive, int oldtype, int newtype);
size_t replaceTextures(
	Archive*        archive,
	const wxString& oldname,
	const wxString& newname,
	bool            floor   = false,
	bool            ceiling = false,
	bool            lower   = false,
	bool            middle  = false,
	bool            upper   = false);
size_t replaceSpecials(
	Archive* archive,
	int      oldtype,
	int      newtype,
	bool     lines   = true,
	bool     things  = true,
	bool     arg0    = false,
	int      oldarg0 = 0,
	int      newarg0 = 0,
	bool     arg1    = false,
	int      oldarg1 = 0,
	int      newarg1 = 0,
	bool     arg2    = false,
	int      oldarg2 = 0,
	int      newarg2 = 0,
	bool     arg3    = false,
	int      oldarg3 = 0,
	int      newarg3 = 0,
	bool     arg4    = false,
	int      oldarg4 = 0,
	int      newarg4 = 0);
}; // namespace slade::archiveoperations
