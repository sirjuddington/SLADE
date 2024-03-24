#pragma once

#include "Graphics/CTexture/CTexture.h"

namespace slade
{
class ResourceManager;

// This base class is probably not really needed
class Resource
{
	friend class ResourceManager;

public:
	Resource(string_view type) : type_{ type } {}
	virtual ~Resource() = default;

	virtual int length() const { return 0; }

private:
	string type_;
};

class EntryResource : public Resource
{
	friend class ResourceManager;

public:
	EntryResource() : Resource("entry") {}
	~EntryResource() override = default;

	void add(const shared_ptr<ArchiveEntry>& entry);
	void remove(const ArchiveEntry* entry);
	void removeArchive(const Archive* archive);

	int length() const override { return entries_.size(); }

	ArchiveEntry* getEntry(const Archive* priority = nullptr, string_view nspace = "", bool ns_required = false);

private:
	vector<weak_ptr<ArchiveEntry>> entries_;
};

class TextureResource : public Resource
{
	friend class ResourceManager;

public:
	struct Texture
	{
		CTexture          tex;
		weak_ptr<Archive> parent;

		Texture(const CTexture& tex_copy, const shared_ptr<Archive>& parent) : parent(parent)
		{
			tex.copyTexture(tex_copy);
		}
	};

	TextureResource() : Resource("texture") {}
	~TextureResource() override = default;

	void add(CTexture* tex, const Archive* parent);
	void remove(const Archive* parent);

	int length() const override { return textures_.size(); }

private:
	vector<unique_ptr<Texture>> textures_;
};

typedef std::map<string, EntryResource>   EntryResourceMap;
typedef std::map<string, TextureResource> TextureResourceMap;

class ResourceManager
{
public:
	ResourceManager()  = default;
	~ResourceManager() = default;

	void addArchive(Archive* archive);
	void removeArchive(const Archive* archive);

	void addEntry(shared_ptr<ArchiveEntry>& entry);
	void removeEntry(ArchiveEntry* entry, string_view entry_name = {}, bool full_check = false);

	void listAllPatches() const;
	void putAllPatchEntries(vector<ArchiveEntry*>& list, const Archive* priority, bool fullPath = false);

	void putAllTextures(
		vector<TextureResource::Texture*>& list,
		const Archive*                     priority,
		const Archive*                     ignore = nullptr) const;
	void putAllTextureNames(vector<string>& list) const;

	void putAllFlatEntries(vector<ArchiveEntry*>& list, const Archive* priority, bool fullPath = false);
	void putAllFlatNames(vector<string>& list) const;

	ArchiveEntry* getPaletteEntry(string_view palette, const Archive* priority = nullptr);
	ArchiveEntry* getPatchEntry(string_view patch, string_view nspace = "patches", const Archive* priority = nullptr);
	ArchiveEntry* getFlatEntry(string_view flat, const Archive* priority = nullptr);
	ArchiveEntry* getTextureEntry(
		string_view    texture,
		string_view    nspace   = "textures",
		const Archive* priority = nullptr);
	ArchiveEntry* getHiresEntry(string_view texture, const Archive* priority = nullptr);
	CTexture*     getTexture(
			string_view    texture,
			string_view    type     = {},
			const Archive* priority = nullptr,
			const Archive* ignore   = nullptr);
	uint16_t getTextureHash(string_view name) const;

	// Signals
	struct Signals
	{
		sigslot::signal<> resources_updated;
	};
	Signals& signals() { return signals_; }

	static string doom64TextureName(uint16_t hash) { return doom64_hash_table_[hash]; }

private:
	EntryResourceMap   palettes_;
	EntryResourceMap   patches_;
	EntryResourceMap   patches_fp_;      // Full path
	EntryResourceMap   patches_fp_only_; // Patches that can only be used by their full path name
	EntryResourceMap   graphics_;
	EntryResourceMap   flats_;
	EntryResourceMap   flats_fp_;
	EntryResourceMap   flats_fp_only_;
	EntryResourceMap   satextures_; // Stand Alone textures (e.g., between TX_ or T_ markers)
	EntryResourceMap   satextures_fp_;
	EntryResourceMap   hires_;
	TextureResourceMap composites_; // Composite textures (defined in a TEXTUREx/TEXTURES lump)
	Signals            signals_;

	static string doom64_hash_table_[65536];

	void updateEntry(ArchiveEntry& entry, bool remove, bool add);
};
} // namespace slade
