#pragma once

#include "Archive/ArchiveEntry.h"

namespace slade
{
class CTexture;

class PatchTable
{
public:
	struct Patch
	{
		string         name;
		vector<string> used_in;

		Patch(string_view name) : name{ name } {}

		void removeTextureUsage(string_view texture)
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

	size_t               nPatches() const { return patches_.size(); }
	Archive*             parent() const { return parent_; }
	void                 setParent(Archive* parent) { parent_ = parent; }
	const vector<Patch>& patches() const { return patches_; }

	Patch&        patch(size_t index);
	Patch&        patch(string_view name);
	const string& patchName(size_t index) const;
	ArchiveEntry* patchEntry(size_t index);
	ArchiveEntry* patchEntry(string_view name);
	int32_t       patchIndex(string_view name) const;
	int32_t       patchIndex(ArchiveEntry* entry) const;
	bool          removePatch(unsigned index);
	bool          replacePatch(unsigned index, string_view newname);
	bool          addPatch(string_view name, bool allow_dup = false);

	bool loadPNAMES(ArchiveEntry* pnames, Archive* parent = nullptr);
	bool writePNAMES(ArchiveEntry* pnames);

	void clearPatchUsage();
	void updatePatchUsage(CTexture* tex);

	// Signals
	struct Signals
	{
		sigslot::signal<> modified;
	};
	Signals& signals() { return signals_; }

private:
	Archive*      parent_ = nullptr;
	vector<Patch> patches_;
	Patch         patch_invalid_{ "INVALID_PATCH" };
	Signals       signals_;
};
} // namespace slade
