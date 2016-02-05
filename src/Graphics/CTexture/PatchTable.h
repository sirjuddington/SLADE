
#ifndef __PATCH_TABLE_H__
#define __PATCH_TABLE_H__

#include "Archive/ArchiveEntry.h"
#include "General/ListenerAnnouncer.h"

struct patch_t
{
	string			name;
	vector<string>	used_in;

	void removeTextureUsage(string texture)
	{
		for (unsigned a = 0; a < used_in.size(); a++)
		{
			if (S_CMP(texture, used_in[a]))
			{
				used_in.erase(used_in.begin() + a);
				a--;
			}
		}
	}
};

class CTexture;

class PatchTable : public Announcer
{
private:
	Archive*		parent;
	vector<patch_t>	patches;
	patch_t			patch_invalid;

public:
	PatchTable(Archive* parent = NULL);
	~PatchTable();

	size_t		nPatches() { return patches.size(); }
	Archive*	getParent() { return parent; }
	void		setParent(Archive* parent) { this->parent = parent; }

	patch_t&		patch(size_t index);
	patch_t&		patch(string name);
	string			patchName(size_t index);
	ArchiveEntry*	patchEntry(size_t index);
	ArchiveEntry*	patchEntry(string name);
	int32_t			patchIndex(string name);
	int32_t			patchIndex(ArchiveEntry* entry);
	bool			removePatch(unsigned index);
	bool			replacePatch(unsigned index, string newname);
	bool			addPatch(string name, bool allow_dup = false);

	bool	loadPNAMES(ArchiveEntry* pnames, Archive* parent = NULL);
	bool	writePNAMES(ArchiveEntry* pnames);

	void	clearPatchUsage();
	void	updatePatchUsage(CTexture* tex);
};

#endif//__PATCH_TABLE_T__
