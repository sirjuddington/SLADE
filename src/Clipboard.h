
#ifndef __CLIPBOARD_H__
#define	__CLIPBOARD_H__

#include "Archive.h"

enum ClipboardType {
	CLIPBOARD_ENTRY_TREE,
	CLIPBOARD_COMPOSITE_TEXTURE,
	CLIPBOARD_PATCH,
	CLIPBOARD_MAP_ARCH,
	CLIPBOARD_MAP_THINGS,

	CLIPBOARD_UNKNOWN,
};

class ClipboardItem {
private:
	int			type;

public:
	ClipboardItem(int type = CLIPBOARD_UNKNOWN);
	~ClipboardItem();

	int	getType() { return type; }
};

class EntryTreeClipboardItem : public ClipboardItem {
private:
	ArchiveTreeNode*	tree;

public:
	EntryTreeClipboardItem(vector<ArchiveEntry*>& entries, vector<ArchiveTreeNode*>& dirs);
	~EntryTreeClipboardItem();

	ArchiveTreeNode*	getTree() { return tree; }
};

class CTexture;
class TextureClipboardItem : public ClipboardItem {
private:
	CTexture*				texture;
	vector<ArchiveEntry*>	patch_entries;

public:
	TextureClipboardItem(CTexture* texture, Archive* parent);
	~TextureClipboardItem();

	CTexture*		getTexture() { return texture; }
	ArchiveEntry*	getPatchEntry(string patch);
};

class MapVertex;
class MapSide;
class MapLine;
class MapSector;
class SLADEMap;
class MapArchClipboardItem : public ClipboardItem {
private:
	vector<MapVertex*>	vertices;
	vector<MapSide*>	sides;
	vector<MapLine*>	lines;
	vector<MapSector*>	sectors;

public:
	MapArchClipboardItem();
	~MapArchClipboardItem();

	void	addLines(vector<MapLine*> lines);
	string	getInfo();
	void	pasteToMap(SLADEMap* map, fpoint2_t position);
	void	getLines(vector<MapLine*>& list);
};

class MapThing;
class MapThingsClipboardItem : public ClipboardItem {
private:
	vector<MapThing*>	things;

public:
	MapThingsClipboardItem();
	~MapThingsClipboardItem();

	void	addThings(vector<MapThing*>& things);
	string	getInfo();
	void	pasteToMap(SLADEMap* map, fpoint2_t position);
	void	getThings(vector<MapThing*>& list);
};

class Clipboard {
private:
	vector<ClipboardItem*>	items;
	static Clipboard*		instance;

public:
	Clipboard();
	~Clipboard();

	// Singleton implementation
	static Clipboard*	getInstance() {
		if (!instance)
			instance = new Clipboard();

		return instance;
	}

	uint32_t		nItems() { return items.size(); }
	ClipboardItem*	getItem(uint32_t index);
	bool			addItem(ClipboardItem* item);
	bool			addItems(vector<ClipboardItem*>& items);

	void	clear();
};

#endif//__CLIPBOARD_H__

// Define for less cumbersome ClipBoard::getInstance()
#define theClipboard Clipboard::getInstance()
