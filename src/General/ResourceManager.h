#pragma once

#include "Archive/Archive.h"
#include "General/ListenerAnnouncer.h"
#include "Graphics/CTexture/CTexture.h"

class ResourceManager;

// This base class is probably not really needed
class Resource
{
	friend class ResourceManager;

public:
	Resource(std::string_view type) : type_{ type } {}
	virtual ~Resource() = default;

	virtual int length() { return 0; }

private:
	std::string type_;
};

class EntryResource : public Resource
{
	friend class ResourceManager;

public:
	EntryResource() : Resource("entry") {}
	virtual ~EntryResource() = default;

	void add(ArchiveEntry::SPtr& entry);
	void remove(ArchiveEntry::SPtr& entry);
	void removeArchive(Archive* archive);

	int length() override { return entries_.size(); }

	ArchiveEntry* getEntry(Archive* priority = nullptr, std::string_view nspace = "", bool ns_required = false);

private:
	vector<std::weak_ptr<ArchiveEntry>> entries_;
};

class TextureResource : public Resource
{
	friend class ResourceManager;

public:
	struct Texture
	{
		CTexture tex;
		Archive* parent;

		Texture(const CTexture& tex_copy, Archive* parent) : parent(parent) { tex.copyTexture(tex_copy); }
	};

	TextureResource() : Resource("texture") {}
	virtual ~TextureResource() = default;

	void add(CTexture* tex, Archive* parent);
	void remove(Archive* parent);

	int length() override { return textures_.size(); }

private:
	vector<std::unique_ptr<Texture>> textures_;
};

typedef std::map<std::string, EntryResource>   EntryResourceMap;
typedef std::map<std::string, TextureResource> TextureResourceMap;

class ResourceManager : public Listener, public Announcer
{
public:
	ResourceManager()  = default;
	~ResourceManager() = default;

	void addArchive(Archive* archive);
	void removeArchive(Archive* archive);

	void addEntry(ArchiveEntry::SPtr& entry, bool log = false);
	void removeEntry(ArchiveEntry::SPtr& entry, bool log = false, bool full_check = false);

	void listAllPatches();
	void putAllPatchEntries(vector<ArchiveEntry*>& list, Archive* priority, bool fullPath = false);

	void putAllTextures(vector<TextureResource::Texture*>& list, Archive* priority, Archive* ignore = nullptr);
	void putAllTextureNames(vector<std::string>& list);

	void putAllFlatEntries(vector<ArchiveEntry*>& list, Archive* priority, bool fullPath = false);
	void putAllFlatNames(vector<std::string>& list);

	ArchiveEntry* getPaletteEntry(std::string_view palette, Archive* priority = nullptr);
	ArchiveEntry* getPatchEntry(std::string_view patch, std::string_view nspace = "patches", Archive* priority = nullptr);
	ArchiveEntry* getFlatEntry(std::string_view flat, Archive* priority = nullptr);
	ArchiveEntry* getTextureEntry(
		std::string_view texture,
		std::string_view nspace   = "textures",
		Archive*        priority = nullptr);
	CTexture* getTexture(std::string_view texture, Archive* priority = nullptr, Archive* ignore = nullptr);
	uint16_t  getTextureHash(std::string_view name) const;

	void onAnnouncement(Announcer* announcer, std::string_view event_name, MemChunk& event_data) override;

	static std::string doom64TextureName(uint16_t hash) { return doom64_hash_table_[hash]; }

private:
	EntryResourceMap palettes_;
	EntryResourceMap patches_;
	EntryResourceMap patches_fp_;      // Full path
	EntryResourceMap patches_fp_only_; // Patches that can only be used by their full path name
	EntryResourceMap graphics_;
	EntryResourceMap flats_;
	EntryResourceMap flats_fp_;
	EntryResourceMap flats_fp_only_;
	EntryResourceMap satextures_; // Stand Alone textures (e.g., between TX_ or T_ markers)
	EntryResourceMap satextures_fp_;
	// EntryResourceMap	satextures_fp_only_; // Probably not needed
	TextureResourceMap textures_; // Composite textures (defined in a TEXTUREx/TEXTURES lump)

	static std::string doom64_hash_table_[65536];
};
