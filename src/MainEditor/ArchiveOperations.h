#pragma once

namespace slade::archiveoperations
{
bool save(Archive& archive);
bool saveAs(Archive& archive);

bool buildArchive(string_view path);

bool removeUnusedPatches(Archive* archive);
bool checkDuplicateEntryNames(Archive* archive);
bool checkDuplicateEntryContent(const Archive* archive);
void removeUnusedTextures(Archive* archive);
void removeUnusedFlats(Archive* archive);
void removeUnusedZDoomTextures(Archive* archive);
bool checkDuplicateZDoomTextures(Archive* archive);
bool checkDuplicateZDoomPatches(Archive* archive);
void removeEntriesUnchangedFromIWAD(Archive* archive);
bool checkOverriddenEntriesInIWAD(Archive* archive);
bool checkZDoomOverriddenEntriesInIWAD(Archive* archive);

// Search and replace in maps
size_t replaceThings(Archive* archive, int oldtype, int newtype);
size_t replaceTextures(
	Archive*      archive,
	const string& oldtex,
	const string& newtex,
	bool          floor   = false,
	bool          ceiling = false,
	bool          lower   = false,
	bool          middle  = false,
	bool          upper   = false);
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
