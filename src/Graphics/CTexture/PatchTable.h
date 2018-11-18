#pragma once

#include "Archive/ArchiveEntry.h"
#include "General/ListenerAnnouncer.h"

class CTexture;

class PatchTable : public Announcer
{
public:
	struct Patch
	{
		string         name;
		vector<string> used_in;

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

	PatchTable(Archive* parent = nullptr);
	~PatchTable();

	size_t   nPatches() { return patches_.size(); }
	Archive* getParent() { return parent_; }
	void     setParent(Archive* parent) { this->parent_ = parent; }

	Patch&        patch(size_t index);
	Patch&        patch(string name);
	string        patchName(size_t index);
	ArchiveEntry* patchEntry(size_t index);
	ArchiveEntry* patchEntry(string name);
	int32_t       patchIndex(string name);
	int32_t       patchIndex(ArchiveEntry* entry);
	bool          removePatch(unsigned index);
	bool          replacePatch(unsigned index, string newname);
	bool          addPatch(string name, bool allow_dup = false);

	bool loadPNAMES(ArchiveEntry* pnames, Archive* parent = nullptr);
	bool writePNAMES(ArchiveEntry* pnames);

	void clearPatchUsage();
	void updatePatchUsage(CTexture* tex);

private:
	Archive*      parent_;
	vector<Patch> patches_;
	Patch         patch_invalid_;
};
