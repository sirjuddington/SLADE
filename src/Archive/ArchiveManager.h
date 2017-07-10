#pragma once

#include "App.h"
#include "General/ListenerAnnouncer.h"
#include "Archive.h"

class ArchiveManager : public Announcer, Listener
{
public:
	ArchiveManager();
	~ArchiveManager();

	bool		init();
	bool		initBaseResource();
	bool		resArchiveOK() const { return res_archive_open_; }
	bool		addArchive(Archive* archive);
	bool		validResDir(string dir) const;
	Archive*	getArchive(int index);
	Archive*	getArchive(string filename);
	Archive*	openArchive(string filename, bool manage = true, bool silent = false);
	Archive*	openArchive(ArchiveEntry* entry, bool manage = true, bool silent = false);
	Archive*	openDirArchive(string dir, bool manage = true, bool silent = false);
	Archive*	newArchive(string format);
	bool		closeArchive(int index);
	bool		closeArchive(string filename);
	bool		closeArchive(Archive* archive);
	void		closeAll();
	int			numArchives() const { return (int)open_archives_.size(); }
	int			archiveIndex(Archive* archive);
	vector<Archive*>
				getDependentArchives(Archive* archive);
	Archive*	programResourceArchive() const { return program_resource_archive_; }
	string		getArchiveExtensionsString() const;
	bool		archiveIsResource(Archive* archive);
	void		setArchiveResource(Archive* archive, bool resource = true);

	// Base resource archive stuff
	Archive*	baseResourceArchive() const { return base_resource_archive_; }
	bool		addBaseResourcePath(string path);
	void		removeBaseResourcePath(unsigned index);
	unsigned	numBaseResourcePaths() const { return base_resource_paths_.size(); }
	string		getBaseResourcePath(unsigned index);
	bool		openBaseResource(int index);

	// Resource entry get/search
	ArchiveEntry*			getResourceEntry(string name, Archive* ignore = NULL);
	ArchiveEntry*			findResourceEntry(Archive::SearchOptions& options, Archive* ignore = NULL);
	vector<ArchiveEntry*>	findAllResourceEntries(Archive::SearchOptions& options, Archive* ignore = NULL);

	// Recent files
	string		recentFile(unsigned index);
	unsigned	numRecentFiles() const { return recent_files_.size(); }
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
	unsigned		numBookmarks() const { return bookmarks_.size(); }

	void	onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data) override;

	// For scripting
	Archive* luaOpenFile(const char* filename) { return openArchive(filename); }

private:
	struct OpenArchive
	{
		Archive*			archive;
		vector<Archive*>	open_children;	// A list of currently open archives that are within this archive
		bool				resource;
	};

	vector<OpenArchive>		open_archives_;
	Archive*				program_resource_archive_;
	Archive*				base_resource_archive_;
	bool					res_archive_open_;
	vector<string>			base_resource_paths_;
	vector<string>			recent_files_;
	vector<ArchiveEntry*>	bookmarks_;

	bool	initArchiveFormats() const;
	void	getDependentArchivesInternal(Archive* archive, vector<Archive*>& vec);
};
