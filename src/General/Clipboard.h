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

	int type() { return type_; }

private:
	int type_;
};

class EntryTreeClipboardItem : public ClipboardItem
{
public:
	EntryTreeClipboardItem(vector<ArchiveEntry*>& entries, vector<ArchiveTreeNode*>& dirs);
	~EntryTreeClipboardItem();

	ArchiveTreeNode* tree() { return tree_; }

private:
	ArchiveTreeNode* tree_;
};

class CTexture;
class TextureClipboardItem : public ClipboardItem
{
public:
	TextureClipboardItem(CTexture* texture, Archive* parent);
	~TextureClipboardItem();

	CTexture*     texture() { return texture_; }
	ArchiveEntry* patchEntry(string patch);

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
	string             info();
	vector<MapVertex*> pasteToMap(SLADEMap* map, fpoint2_t position);
	void               putLines(vector<MapLine*>& list);
	fpoint2_t          midpoint() { return midpoint_; }

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
	string    info();
	void      pasteToMap(SLADEMap* map, fpoint2_t position);
	void      putThings(vector<MapThing*>& list);
	fpoint2_t midpoint() { return midpoint_; }

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
		if (!instance_)
			instance_ = new Clipboard();

		return instance_;
	}

	uint32_t       nItems() { return items_.size(); }
	ClipboardItem* item(uint32_t index);
	bool           addItem(ClipboardItem* item);
	bool           addItems(vector<ClipboardItem*>& items);

	void clear();

private:
	vector<ClipboardItem*> items_;
	static Clipboard*      instance_;
};

// Define for less cumbersome ClipBoard::getInstance()
#define theClipboard Clipboard::getInstance()
