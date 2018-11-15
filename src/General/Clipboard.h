#pragma once

#include "Archive/Archive.h"
#include "Utility/Structs.h"

enum ClipboardType
{
	CLIPBOARD_ENTRY_TREE,
	CLIPBOARD_COMPOSITE_TEXTURE,
	CLIPBOARD_PATCH,
	CLIPBOARD_MAP_ARCH,
	CLIPBOARD_MAP_THINGS,

	CLIPBOARD_UNKNOWN,
};

class ClipboardItem
{
public:
	ClipboardItem(int type = CLIPBOARD_UNKNOWN);
	~ClipboardItem();

	int getType() { return type_; }

private:
	int type_;
};

class EntryTreeClipboardItem : public ClipboardItem
{
public:
	EntryTreeClipboardItem(vector<ArchiveEntry*>& entries, vector<ArchiveTreeNode*>& dirs);
	~EntryTreeClipboardItem();

	ArchiveTreeNode* getTree() { return tree_; }

private:
	ArchiveTreeNode* tree_;
};

class CTexture;
class TextureClipboardItem : public ClipboardItem
{
public:
	TextureClipboardItem(CTexture* texture, Archive* parent);
	~TextureClipboardItem();

	CTexture*     getTexture() { return texture_; }
	ArchiveEntry* getPatchEntry(string patch);

private:
	CTexture*             texture_;
	vector<ArchiveEntry*> patch_entries_;
};

class MapVertex;
class MapSide;
class MapLine;
class MapSector;
class SLADEMap;
class MapArchClipboardItem : public ClipboardItem
{
public:
	MapArchClipboardItem();
	~MapArchClipboardItem();

	void               addLines(vector<MapLine*> lines);
	string             getInfo();
	vector<MapVertex*> pasteToMap(SLADEMap* map, fpoint2_t position);
	void               getLines(vector<MapLine*>& list);
	fpoint2_t          getMidpoint() { return midpoint_; }

private:
	vector<MapVertex*> vertices_;
	vector<MapSide*>   sides_;
	vector<MapLine*>   lines_;
	vector<MapSector*> sectors_;
	fpoint2_t          midpoint_;
};

class MapThing;
class MapThingsClipboardItem : public ClipboardItem
{
public:
	MapThingsClipboardItem();
	~MapThingsClipboardItem();

	void      addThings(vector<MapThing*>& things);
	string    getInfo();
	void      pasteToMap(SLADEMap* map, fpoint2_t position);
	void      getThings(vector<MapThing*>& list);
	fpoint2_t getMidpoint() { return midpoint_; }

private:
	vector<MapThing*> things_;
	fpoint2_t         midpoint_;
};

class Clipboard
{
public:
	Clipboard();
	~Clipboard();

	// Singleton implementation
	static Clipboard* getInstance()
	{
		if (!instance)
			instance = new Clipboard();

		return instance;
	}

	uint32_t       nItems() { return items_.size(); }
	ClipboardItem* getItem(uint32_t index);
	bool           addItem(ClipboardItem* item);
	bool           addItems(vector<ClipboardItem*>& items);

	void clear();

private:
	vector<ClipboardItem*> items_;
	static Clipboard*      instance;
};

// Define for less cumbersome ClipBoard::getInstance()
#define theClipboard Clipboard::getInstance()
