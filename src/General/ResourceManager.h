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
	Resource(const wxString& type) : type_{ type } {}
	virtual ~Resource() = default;

	virtual int length() { return 0; }

private:
	wxString type_;
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

	ArchiveEntry* getEntry(Archive* priority = nullptr, const wxString& nspace = "", bool ns_required = false);

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

typedef std::map<wxString, EntryResource>   EntryResourceMap;
typedef std::map<wxString, TextureResource> TextureResourceMap;

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
	void putAllTextureNames(vector<wxString>& list);

	void putAllFlatEntries(vector<ArchiveEntry*>& list, Archive* priority, bool fullPath = false);
	void putAllFlatNames(vector<wxString>& list);

	ArchiveEntry* getPaletteEntry(const wxString& palette, Archive* priority = nullptr);
	ArchiveEntry* getPatchEntry(const wxString& patch, const wxString& nspace = "patches", Archive* priority = nullptr);
	ArchiveEntry* getFlatEntry(const wxString& flat, Archive* priority = nullptr);
	ArchiveEntry* getTextureEntry(
		const wxString& texture,
		const wxString& nspace   = "textures",
		Archive*        priority = nullptr);
	CTexture* getTexture(const wxString& texture, Archive* priority = nullptr, Archive* ignore = nullptr);
	uint16_t  getTextureHash(const wxString& name) const;

	void onAnnouncement(Announcer* announcer, const wxString& event_name, MemChunk& event_data) override;

	static wxString doom64TextureName(uint16_t hash) { return doom64_hash_table_[hash]; }

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

	static wxString doom64_hash_table_[65536];
};
