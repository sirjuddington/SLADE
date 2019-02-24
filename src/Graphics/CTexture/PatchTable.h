#pragma once

#include "Archive/ArchiveEntry.h"
#include "General/ListenerAnnouncer.h"

class CTexture;

class PatchTable : public Announcer
{
public:
	struct Patch
	{
		std::string         name;
		vector<std::string> used_in;

		Patch(std::string_view name) : name{ name } {}

		void removeTextureUsage(std::string_view texture)
		{
			for (unsigned a = 0; a < used_in.size(); a++)
			{
				if (texture == used_in[a])
				{
					used_in.erase(used_in.begin() + a);
					a--;
				}
			}
		}
	};

	PatchTable(Archive* parent = nullptr) : parent_{ parent } {}
	~PatchTable() = default;

	size_t   nPatches() const { return patches_.size(); }
	Archive* parent() const { return parent_; }
	void     setParent(Archive* parent) { this->parent_ = parent; }

	Patch&             patch(size_t index);
	Patch&             patch(std::string_view name);
	const std::string& patchName(size_t index);
	ArchiveEntry*      patchEntry(size_t index);
	ArchiveEntry*      patchEntry(std::string_view name);
	int32_t            patchIndex(std::string_view name);
	int32_t            patchIndex(ArchiveEntry* entry);
	bool               removePatch(unsigned index);
	bool               replacePatch(unsigned index, std::string_view newname);
	bool               addPatch(std::string_view name, bool allow_dup = false);

	bool loadPNAMES(ArchiveEntry* pnames, Archive* parent = nullptr);
	bool writePNAMES(ArchiveEntry* pnames);

	void clearPatchUsage();
	void updatePatchUsage(CTexture* tex);

private:
	Archive*      parent_ = nullptr;
	vector<Patch> patches_;
	Patch         patch_invalid_{ "INVALID_PATCH" };
};
