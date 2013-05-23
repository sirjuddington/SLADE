
#ifndef __PATCH_TABLE_H__
#define __PATCH_TABLE_H__

#include "ArchiveEntry.h"
#include "ListenerAnnouncer.h"

struct patch_t
{
	string			name;
	vector<string>	used_in;

	void removeTextureUsage(string texture)
	{
		vector<string>::iterator i = used_in.begin();
		while (i != used_in.end())
		{
			if (S_CMP(texture, *i))
				used_in.erase(i);
			else
				i++;
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
