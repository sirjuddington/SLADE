#pragma once

#include "Archive/ArchiveEntry.h"
#include "General/ListenerAnnouncer.h"

class CTexture;

class PatchTable : public Announcer
{
public:
	struct Patch
	{
		wxString         name;
		vector<wxString> used_in;

		Patch(const wxString& name) : name{ name } {}

		void removeTextureUsage(const wxString& texture)
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

	PatchTable(Archive* parent = nullptr) : parent_{ parent } {}
	~PatchTable() = default;

	size_t   nPatches() const { return patches_.size(); }
	Archive* parent() const { return parent_; }
	void     setParent(Archive* parent) { this->parent_ = parent; }

	Patch&        patch(size_t index);
	Patch&        patch(const wxString& name);
	wxString      patchName(size_t index);
	ArchiveEntry* patchEntry(size_t index);
	ArchiveEntry* patchEntry(const wxString& name);
	int32_t       patchIndex(const wxString& name);
	int32_t       patchIndex(ArchiveEntry* entry);
	bool          removePatch(unsigned index);
	bool          replacePatch(unsigned index, const wxString& newname);
	bool          addPatch(const wxString& name, bool allow_dup = false);

	bool loadPNAMES(ArchiveEntry* pnames, Archive* parent = nullptr);
	bool writePNAMES(ArchiveEntry* pnames);

	void clearPatchUsage();
	void updatePatchUsage(CTexture* tex);

private:
	Archive*      parent_ = nullptr;
	vector<Patch> patches_;
	Patch         patch_invalid_{ "INVALID_PATCH" };
};
