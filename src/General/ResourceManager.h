
#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#include "common.h"
#include "Archive/Archive.h"
#include "General/ListenerAnnouncer.h"
#include "Graphics/CTexture/CTexture.h"

class ResourceManager;

// This base class is probably not really needed
class Resource
{
	friend class ResourceManager;
private:
	string	type;

public:
	Resource(string type) { this->type = type; }
	~Resource() {}

	virtual int	length() { return 0; }
};

class EntryResource : public Resource
{
	friend class ResourceManager;
private:
	vector<std::weak_ptr<ArchiveEntry>>	entries;

public:
	EntryResource();
	~EntryResource();

	void	add(ArchiveEntry::SPtr& entry);
	void	remove(ArchiveEntry::SPtr& entry);

	int		length();

	ArchiveEntry*	getEntry(Archive* priority = nullptr, string nspace = "", bool ns_required = false);
};

class TextureResource : public Resource
{
	friend class ResourceManager;
public:
	struct Texture
	{
		CTexture	tex;
		Archive*	parent;

		Texture(CTexture* tex_copy, Archive* parent) : parent(parent)
		{
			if (tex_copy)
				tex.copyTexture(tex_copy);
		}
	};

	TextureResource();
	~TextureResource();

	void	add(CTexture* tex, Archive* parent);
	void	remove(Archive* parent);

	int		length();

private:
	vector<std::unique_ptr<Texture>>	textures;
};

typedef std::map<string, EntryResource> EntryResourceMap;
typedef std::map<string, TextureResource> TextureResourceMap;

class ResourceManager : public Listener, public Announcer
{
private:
	EntryResourceMap	palettes;
	EntryResourceMap	patches;
	EntryResourceMap	graphics;
	EntryResourceMap	flats;
	EntryResourceMap	satextures;	// Stand Alone textures (e.g., between TX_ or T_ markers)
	TextureResourceMap	textures;	// Composite textures (defined in a TEXTUREx/TEXTURES lump)

	static ResourceManager*	instance;
	static string Doom64HashTable[65536];

public:
	ResourceManager();
	~ResourceManager();

	static ResourceManager*	getInstance()
	{
		if (!instance)
			instance = new ResourceManager();

		return instance;
	}

	void	addArchive(Archive* archive);
	void	removeArchive(Archive* archive);

	void	addEntry(ArchiveEntry::SPtr& entry);
	void	removeEntry(ArchiveEntry::SPtr& entry);

	void	listAllPatches();
	void	getAllPatchEntries(vector<ArchiveEntry*>& list, Archive* priority);

	void	getAllTextures(vector<TextureResource::Texture*>& list, Archive* priority, Archive* ignore = NULL);
	void	getAllTextureNames(vector<string>& list);

	void	getAllFlatEntries(vector<ArchiveEntry*>& list, Archive* priority);
	void	getAllFlatNames(vector<string>& list);

	ArchiveEntry*	getPaletteEntry(string palette, Archive* priority = NULL);
	ArchiveEntry*	getPatchEntry(string patch, string nspace = "patches", Archive* priority = NULL);
	ArchiveEntry*	getFlatEntry(string flat, Archive* priority = NULL);
	ArchiveEntry*	getTextureEntry(string texture, string nspace = "textures", Archive* priority = NULL);
	CTexture*		getTexture(string texture, Archive* priority = NULL, Archive* ignore = NULL);
	string			getTextureName(uint16_t hash) { return Doom64HashTable[hash]; }
	uint16_t		getTextureHash(string name);

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

// Define for less cumbersome ResourceManager::getInstance()
#define theResourceManager ResourceManager::getInstance()

#endif//__RESOURCE_MANAGER_H__
