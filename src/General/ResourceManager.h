
#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#include "General/ListenerAnnouncer.h"
#include "Archive/Archive.h"
#include <map>

class ResourceManager;
class CTexture;

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
	vector<ArchiveEntry*>	entries;

public:
	EntryResource(ArchiveEntry* entry = NULL);
	~EntryResource();

	void	add(ArchiveEntry* entry);
	void	remove(ArchiveEntry* entry);

	int		length();
};

class TextureResource : public Resource
{
	friend class ResourceManager;
public:
	struct tex_res_t
	{
		CTexture*	tex;
		Archive*	parent;
	};

	TextureResource();
	~TextureResource();

	void	add(CTexture* tex, Archive* parent);
	void	remove(Archive* parent);

	int		length();

private:
	vector<tex_res_t>	textures;
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

	void	addEntry(ArchiveEntry* entry);
	void	removeEntry(ArchiveEntry* entry);

	void	listAllPatches();
	void	getAllPatchEntries(vector<ArchiveEntry*>& list, Archive* priority);

	void	getAllTextures(vector<TextureResource::tex_res_t>& list, Archive* priority, Archive* ignore = NULL);
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
