
#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "ArchiveEntry.h"
#include "Utility/Tree.h"
#include "General/ListenerAnnouncer.h"

class ArchiveTreeNode : public STreeNode
{
	friend class Archive;
private:
	Archive*					archive;
	ArchiveEntry::SPtr			dir_entry;
	vector<ArchiveEntry::SPtr>	entries;

protected:
	STreeNode* createChild(string name)
	{
		ArchiveTreeNode* node = new ArchiveTreeNode();
		node->dir_entry->name = name;
		node->archive = archive;
		return node;
	}

public:
	ArchiveTreeNode(ArchiveTreeNode* parent = NULL);
	~ArchiveTreeNode();

	void 		addChild(STreeNode* child);

	Archive*			getArchive();
	string				getName();
	ArchiveEntry*		getDirEntry() { return dir_entry.get(); }
	ArchiveEntry*		getEntry(unsigned index);
	ArchiveEntry::SPtr	getEntryShared(unsigned index);
	ArchiveEntry*		getEntry(string name, bool cut_ext = false);
	ArchiveEntry::SPtr	getEntryShared(string name, bool cut_ext = false);
	ArchiveEntry::SPtr	getEntryShared(ArchiveEntry* entry);
	unsigned			numEntries(bool inc_subdirs = false);
	int					entryIndex(ArchiveEntry* entry, size_t startfrom = 0);

	void	setName(string name) { dir_entry->name = name; }

	void	linkEntries(ArchiveEntry* first, ArchiveEntry* second);
	bool	addEntry(ArchiveEntry* entry, unsigned index = 0xFFFFFFFF);
	bool	addEntry(ArchiveEntry::SPtr& entry, unsigned index = 0xFFFFFFFF);
	bool	removeEntry(unsigned index);
	bool	swapEntries(unsigned index1, unsigned index2);

	ArchiveTreeNode*	clone();
	bool				merge(ArchiveTreeNode* node, unsigned position = 0xFFFFFFFF, int state = 2);

	bool	exportTo(string path);
};

// Define archive types
enum ArchiveTypes
{
	ARCHIVE_INVALID = 0,
	ARCHIVE_WAD,
	ARCHIVE_ZIP,
	ARCHIVE_LIB,
	ARCHIVE_DAT,
	ARCHIVE_RES,
	ARCHIVE_WAD2,
	ARCHIVE_WAD3,
	ARCHIVE_PAK,
	ARCHIVE_BSP,
	ARCHIVE_GRP,
	ARCHIVE_RFF,
	ARCHIVE_WOLF,
	ARCHIVE_GOB,
	ARCHIVE_LFD,
	ARCHIVE_HOG,
	ARCHIVE_HOG2,
	ARCHIVE_PIG,
	ARCHIVE_MVL,
	ARCHIVE_ADAT,
	ARCHIVE_TAR,
	ARCHIVE_GZIP,
	ARCHIVE_BZ2,
	ARCHIVE_7Z,
	ARCHIVE_DISK,
	ARCHIVE_FOLDER,
	ARCHIVE_POD,
	ARCHIVE_CHASM_BIN,
};

struct archive_desc_t
{
	uint8_t	type;				// See ArchiveTypes enum
	bool	supports_dirs;
	bool	names_extensions;
	int		max_name_length;

	archive_desc_t()
	{
		supports_dirs = false;
		names_extensions = true;
		max_name_length = -1;
	}
};

class Archive : public Announcer
{
private:
	bool				modified;
	ArchiveTreeNode*	dir_root;

protected:
	archive_desc_t		desc;
	string			filename;
	ArchiveEntry*	parent;
	bool			on_disk;	// Specifies whether the archive exists on disk (as opposed to being newly created)
	bool			read_only;	// If true, the archive cannot be modified

public:
	struct mapdesc_t
	{
		string			name;
		ArchiveEntry*	head;
		ArchiveEntry*	end;
		uint8_t			format;		// See MapTypes enum
		bool			archive;	// True if head is an archive (for maps in zips)

		vector<ArchiveEntry*> unk;	// Unknown map lumps (must be preserved for UDMF compliance)

		mapdesc_t()
		{
			head = end = NULL;
			archive = false;
			format = MAP_UNKNOWN;
		}
	};

	static bool	save_backup;

	Archive(uint8_t type = ARCHIVE_INVALID);
	virtual ~Archive();

	archive_desc_t		getDesc() { return desc; }
	uint8_t				getType() { return desc.type; }
	string				getFilename(bool full = true);
	ArchiveEntry*		getParent() { return parent; }
	Archive*			getParentArchive() { return (parent ? parent->getParent() : NULL); }
	ArchiveTreeNode*	getRoot() { return dir_root; }
	bool				isModified() { return modified; }
	bool				isOnDisk() { return on_disk; }
	bool				isReadOnly() { return read_only; }
	virtual bool		isWritable() { return true; }

	void	setModified(bool modified);
	void	setFilename(string filename) { this->filename = filename; }

	// Entry retrieval/info
	bool						checkEntry(ArchiveEntry* entry);
	virtual ArchiveEntry*		getEntry(string name, bool cut_ext = false, ArchiveTreeNode* dir = NULL);
	virtual ArchiveEntry*		getEntry(unsigned index, ArchiveTreeNode* dir = NULL);
	virtual int					entryIndex(ArchiveEntry* entry, ArchiveTreeNode* dir = NULL);
	virtual ArchiveEntry*		entryAtPath(string path);
	virtual ArchiveEntry::SPtr	entryAtPathShared(string path);

	// Archive type info
	virtual string	getFileExtensionString() = 0;
	virtual string	getFormat() = 0;
	virtual bool	isTreeless() { return false; }

	// Opening
	virtual bool	open(string filename);			// Open from File
	virtual bool	open(ArchiveEntry* entry);		// Open from ArchiveEntry
	virtual bool	open(MemChunk& mc) = 0;			// Open from MemChunk

	// Writing/Saving
	virtual bool	write(MemChunk& mc, bool update = true) = 0;	// Write to MemChunk
	virtual bool	write(string filename, bool update = true);		// Write to File
	virtual bool	save(string filename = "");						// Save archive

	// Misc
	virtual bool		loadEntryData(ArchiveEntry* entry) = 0;
	virtual unsigned	numEntries();
	virtual void		close();
	void				entryStateChanged(ArchiveEntry* entry);
	void				getEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveTreeNode* start = NULL);
	void				getEntryTreeAsList(vector<ArchiveEntry::SPtr>& list, ArchiveTreeNode* start = NULL);
	bool				canSave() { return parent || on_disk; }
	virtual bool		paste(ArchiveTreeNode* tree, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* base = NULL);
	virtual bool		importDir(string directory);
	virtual bool		hasFlatHack() { return false; }

	// Directory stuff
	virtual ArchiveTreeNode*	getDir(string path, ArchiveTreeNode* base = NULL);
	virtual ArchiveTreeNode*	createDir(string path, ArchiveTreeNode* base = NULL);
	virtual bool				removeDir(string path, ArchiveTreeNode* base = NULL);
	virtual bool				renameDir(ArchiveTreeNode* dir, string new_name);

	// Entry addition/removal
	virtual ArchiveEntry*	addEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL, bool copy = false);
	virtual ArchiveEntry*	addEntry(ArchiveEntry* entry, string add_namespace, bool copy = false) { return addEntry(entry, 0xFFFFFFFF, NULL, false); } // By default, add to the 'global' namespace (ie root dir)
	virtual ArchiveEntry*	addNewEntry(string name = "", unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL);
	virtual ArchiveEntry*	addNewEntry(string name, string add_namespace);
	virtual bool			removeEntry(ArchiveEntry* entry);

	// Entry moving
	virtual bool	swapEntries(unsigned index1, unsigned index2, ArchiveTreeNode* dir = NULL);
	virtual bool	swapEntries(ArchiveEntry* entry1, ArchiveEntry* entry2);
	virtual bool	moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL);

	// Entry modification
	virtual bool	renameEntry(ArchiveEntry* entry, string name);
	virtual bool	revertEntry(ArchiveEntry* entry);

	// Detection
	virtual mapdesc_t			getMapInfo(ArchiveEntry* maphead) { return mapdesc_t(); }
	virtual vector<mapdesc_t>	detectMaps() = 0;
	virtual string				detectNamespace(ArchiveEntry* entry);
	virtual string				detectNamespace(size_t index, ArchiveTreeNode* dir = NULL);

	// Search
	struct search_options_t
	{
		string				match_name;			// Ignore if empty
		EntryType*			match_type;			// Ignore if NULL
		string				match_namespace;	// Ignore if empty
		ArchiveTreeNode*	dir;				// Root if NULL
		bool				ignore_ext;			// Defaults true
		bool				search_subdirs;		// Defaults false

		search_options_t()
		{
			match_name = "";
			match_type = NULL;
			match_namespace = "";
			dir = NULL;
			ignore_ext = true;
			search_subdirs = false;
		}
	};
	virtual ArchiveEntry*			findFirst(search_options_t& options);
	virtual ArchiveEntry*			findLast(search_options_t& options);
	virtual vector<ArchiveEntry*>	findAll(search_options_t& options);
	virtual vector<ArchiveEntry*>	findModifiedEntries(ArchiveTreeNode* dir = NULL);
};

// Base class for list-based archive formats
class TreelessArchive : public Archive
{
public:
	TreelessArchive(uint8_t type = ARCHIVE_INVALID) :Archive(type) { desc.supports_dirs = false; }
	virtual ~TreelessArchive() {}

	// Entry retrieval/info
	virtual ArchiveEntry*		getEntry(string name, bool cut_ext = false, ArchiveTreeNode* dir = NULL) { return Archive::getEntry(name); }
	virtual ArchiveEntry*		getEntry(unsigned index, ArchiveTreeNode* dir = NULL) { return Archive::getEntry(index, NULL); }
	virtual int					entryIndex(ArchiveEntry* entry, ArchiveTreeNode* dir = NULL) { return Archive::entryIndex(entry, NULL); }

	// Misc
	virtual unsigned		numEntries() { return getRoot()->numEntries(); }
	void					getEntryTreeAsList(vector<ArchiveEntry*>& list, ArchiveTreeNode* start = NULL) { return Archive::getEntryTreeAsList(list, NULL); }
	bool					paste(ArchiveTreeNode* tree, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* base = NULL);
	bool					isTreeless() { return true; }

	// Directory stuff
	virtual ArchiveTreeNode*	getDir(string path, ArchiveTreeNode* base = NULL) { return getRoot(); }
	virtual ArchiveTreeNode*	createDir(string path, ArchiveTreeNode* base = NULL) { return getRoot(); }
	virtual bool				removeDir(string path, ArchiveTreeNode* base = NULL) { return false; }
	virtual bool				renameDir(ArchiveTreeNode* dir, string new_name) { return false; }

	// Entry addition/removal
	virtual ArchiveEntry*	addEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL, bool copy = false) { return Archive::addEntry(entry, position, NULL, copy); }
	virtual ArchiveEntry*	addNewEntry(string name = "", unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL) { return Archive::addNewEntry(name, position, NULL); }

	// Entry moving
	virtual bool	moveEntry(ArchiveEntry* entry, unsigned position = 0xFFFFFFFF, ArchiveTreeNode* dir = NULL) { return Archive::moveEntry(entry, position, NULL); }

	// Detection
	virtual string				detectNamespace(ArchiveEntry* entry) { return "global"; }
	virtual string				detectNamespace(size_t index, ArchiveTreeNode* dir = NULL) { return "global"; }

};

#endif//__ARCHIVE_H__
