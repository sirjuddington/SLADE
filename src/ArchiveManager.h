
#ifndef __ARCHIVEMANAGER_H__
#define __ARCHIVEMANAGER_H__

#include "ListenerAnnouncer.h"
#include "Archive.h"

class ArchiveManager : public Announcer, Listener
{
private:
	struct archive_t
	{
		Archive*			archive;
		vector<Archive*>	open_children;	// A list of currently open archives that are within this archive
		bool				resource;
	};

	vector<archive_t>		open_archives;
	Archive*				program_resource_archive;
	Archive*				base_resource_archive;
	bool					res_archive_open;
	vector<string>			base_resource_paths;
	vector<string>			recent_files;
	vector<ArchiveEntry*>	bookmarks;
	static ArchiveManager*	instance;

	void		getDependentArchivesInternal(Archive* archive, vector<Archive*>& vec);

public:
	ArchiveManager();
	~ArchiveManager();

	static ArchiveManager*	getInstance()
	{
		if (!instance)
			instance = new ArchiveManager();

		return instance;
	}

	static void deleteInstance()
	{
		if (instance)
		{
			delete instance;
			instance = NULL;
		}
	}

	bool		init();
	bool		initBaseResource();
	bool		resArchiveOK() { return res_archive_open; }
	bool		addArchive(Archive* archive);
	bool		validResDir(string dir);
	Archive*	getArchive(int index);
	Archive*	getArchive(string filename);
	Archive*	openArchive(string filename, bool manage = true, bool silent = false);
	Archive*	openArchive(ArchiveEntry* entry, bool manage = true, bool silent = false);
	Archive*	openDirArchive(string dir, bool manage = true, bool silent = false);
	Archive*	newArchive(uint8_t type);
	bool		closeArchive(int index);
	bool		closeArchive(string filename);
	bool		closeArchive(Archive* archive);
	void		closeAll();
	int			numArchives() { return (int)open_archives.size(); }
	int			archiveIndex(Archive* archive);
	vector<Archive*>
				getDependentArchives(Archive* archive);
	Archive*	programResourceArchive() { return program_resource_archive; }
	string		getArchiveExtensionsString(bool multi_case = true);
	bool		archiveIsResource(Archive* archive);
	void		setArchiveResource(Archive* archive, bool resource = true);

	// Base resource archive stuff
	Archive*	baseResourceArchive() { return base_resource_archive; }
	bool		addBaseResourcePath(string path);
	void		removeBaseResourcePath(unsigned index);
	unsigned	numBaseResourcePaths() { return base_resource_paths.size(); }
	string		getBaseResourcePath(unsigned index);
	bool		openBaseResource(int index);

	// Resource entry get/search
	ArchiveEntry*			getResourceEntry(string name, Archive* ignore = NULL);
	ArchiveEntry*			findResourceEntry(Archive::search_options_t& options, Archive* ignore = NULL);
	vector<ArchiveEntry*>	findAllResourceEntries(Archive::search_options_t& options, Archive* ignore = NULL);

	// Recent files
	string		recentFile(unsigned index);
	unsigned	numRecentFiles() { return recent_files.size(); }
	void		addRecentFile(string path);
	void		addRecentFiles(vector<string> paths);
	void		removeRecentFile(string path);

	// Bookmarks
	void			addBookmark(ArchiveEntry* entry);
	bool			deleteBookmark(ArchiveEntry* entry);
	bool			deleteBookmark(unsigned index);
	bool			deleteBookmarksInArchive(Archive* archive);
	bool			deleteBookmarksInDir(ArchiveTreeNode* node);
	ArchiveEntry*	getBookmark(unsigned index);
	unsigned		numBookmarks() { return bookmarks.size(); }

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);
};

// Define for less cumbersome ArchiveManager::getInstance()
#define theArchiveManager ArchiveManager::getInstance()

#endif //__ARCHIVEMANAGER_H__
