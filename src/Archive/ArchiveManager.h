#pragma once

#include "App.h"
#include "Archive.h"
#include "General/ListenerAnnouncer.h"

class ArchiveManager : public Announcer, Listener
{
public:
	ArchiveManager() = default;
	~ArchiveManager();

	bool             init();
	bool             initBaseResource();
	bool             resArchiveOK() const { return res_archive_open_; }
	bool             addArchive(Archive* archive);
	bool             validResDir(const string& dir) const;
	Archive*         getArchive(int index);
	Archive*         getArchive(const string& filename);
	Archive*         openArchive(const string& filename, bool manage = true, bool silent = false);
	Archive*         openArchive(ArchiveEntry* entry, bool manage = true, bool silent = false);
	Archive*         openDirArchive(const string& dir, bool manage = true, bool silent = false);
	Archive*         newArchive(const string& format);
	bool             closeArchive(int index);
	bool             closeArchive(const string& filename);
	bool             closeArchive(Archive* archive);
	void             closeAll();
	int              numArchives() const { return (int)open_archives_.size(); }
	int              archiveIndex(Archive* archive);
	vector<Archive*> getDependentArchives(Archive* archive);
	Archive*         programResourceArchive() const { return program_resource_archive_.get(); }
	string           getArchiveExtensionsString() const;
	bool             archiveIsResource(Archive* archive);
	void             setArchiveResource(Archive* archive, bool resource = true);

	// General access
	const vector<string>&        recentFiles() const { return recent_files_; }
	const vector<string>&        baseResourcePaths() const { return base_resource_paths_; }
	const vector<ArchiveEntry*>& bookmarks() const { return bookmarks_; }

	// Base resource archive stuff
	Archive* baseResourceArchive() const { return base_resource_archive_.get(); }
	bool     addBaseResourcePath(const string& path);
	void     removeBaseResourcePath(unsigned index);
	unsigned numBaseResourcePaths() const { return base_resource_paths_.size(); }
	string   getBaseResourcePath(unsigned index);
	bool     openBaseResource(int index);

	// Resource entry get/search
	ArchiveEntry*         getResourceEntry(const string& name, Archive* ignore = nullptr);
	ArchiveEntry*         findResourceEntry(Archive::SearchOptions& options, Archive* ignore = nullptr);
	vector<ArchiveEntry*> findAllResourceEntries(Archive::SearchOptions& options, Archive* ignore = nullptr);

	// Recent files
	string   recentFile(unsigned index);
	unsigned numRecentFiles() const { return recent_files_.size(); }
	void     addRecentFile(string path);
	void     addRecentFiles(vector<string> paths);
	void     removeRecentFile(const string& path);

	// Bookmarks
	void          addBookmark(ArchiveEntry* entry);
	bool          deleteBookmark(ArchiveEntry* entry);
	bool          deleteBookmark(unsigned index);
	bool          deleteBookmarksInArchive(Archive* archive);
	bool          deleteBookmarksInDir(ArchiveTreeNode* node);
	ArchiveEntry* getBookmark(unsigned index);
	unsigned      numBookmarks() const { return bookmarks_.size(); }

	void onAnnouncement(Announcer* announcer, const string& event_name, MemChunk& event_data) override;

private:
	struct OpenArchive
	{
		Archive*         archive;
		vector<Archive*> open_children; // A list of currently open archives that are within this archive
		bool             resource;
	};

	vector<OpenArchive>   open_archives_;
	Archive::UPtr         program_resource_archive_;
	Archive::UPtr         base_resource_archive_;
	bool                  res_archive_open_ = false;
	vector<string>        base_resource_paths_;
	vector<string>        recent_files_;
	vector<ArchiveEntry*> bookmarks_;

	bool initArchiveFormats() const;
	void getDependentArchivesInternal(Archive* archive, vector<Archive*>& vec);
};
