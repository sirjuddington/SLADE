#pragma once

#include "App.h"
#include "Archive.h"
#include "General/ListenerAnnouncer.h"

class ArchiveManager : public Announcer, Listener
{
public:
	ArchiveManager() = default;
	~ArchiveManager();

	bool                        init();
	bool                        initBaseResource();
	bool                        resArchiveOK() const { return res_archive_open_; }
	bool                        addArchive(shared_ptr<Archive> archive);
	bool                        validResDir(string_view dir) const;
	shared_ptr<Archive>         getArchive(int index);
	shared_ptr<Archive>         getArchive(string_view filename);
	shared_ptr<Archive>         openArchive(string_view filename, bool manage = true, bool silent = false);
	shared_ptr<Archive>         openArchive(ArchiveEntry* entry, bool manage = true, bool silent = false);
	shared_ptr<Archive>         openDirArchive(string_view dir, bool manage = true, bool silent = false);
	shared_ptr<Archive>         newArchive(string_view format);
	bool                        closeArchive(int index);
	bool                        closeArchive(string_view filename);
	bool                        closeArchive(Archive* archive);
	void                        closeAll();
	int                         numArchives() const { return (int)open_archives_.size(); }
	int                         archiveIndex(Archive* archive);
	vector<shared_ptr<Archive>> getDependentArchives(Archive* archive);
	Archive*                    programResourceArchive() const { return program_resource_archive_.get(); }
	string                      getArchiveExtensionsString() const;
	bool                        archiveIsResource(Archive* archive);
	void                        setArchiveResource(Archive* archive, bool resource = true);
	vector<shared_ptr<Archive>> allArchives(bool resource_only = false);
	shared_ptr<Archive>         shareArchive(Archive* archive);

	// General access
	const vector<string>&                 recentFiles() const { return recent_files_; }
	const vector<string>&                 baseResourcePaths() const { return base_resource_paths_; }
	const vector<weak_ptr<ArchiveEntry>>& bookmarks() const { return bookmarks_; }

	// Base resource archive stuff
	Archive* baseResourceArchive() const { return base_resource_archive_.get(); }
	bool     addBaseResourcePath(string_view path);
	void     removeBaseResourcePath(unsigned index);
	unsigned numBaseResourcePaths() const { return base_resource_paths_.size(); }
	string   getBaseResourcePath(unsigned index);
	bool     openBaseResource(int index);

	// Resource entry get/search
	ArchiveEntry*         getResourceEntry(string_view name, Archive* ignore = nullptr);
	ArchiveEntry*         findResourceEntry(Archive::SearchOptions& options, Archive* ignore = nullptr);
	vector<ArchiveEntry*> findAllResourceEntries(Archive::SearchOptions& options, Archive* ignore = nullptr);

	// Recent files
	string   recentFile(unsigned index);
	unsigned numRecentFiles() const { return recent_files_.size(); }
	void     addRecentFile(string_view path);
	void     addRecentFiles(const vector<string>& paths);
	void     removeRecentFile(string_view path);

	// Bookmarks
	void          addBookmark(const shared_ptr<ArchiveEntry>& entry);
	bool          deleteBookmark(ArchiveEntry* entry);
	bool          deleteBookmark(unsigned index);
	bool          deleteBookmarksInArchive(Archive* archive);
	bool          deleteBookmarksInDir(ArchiveTreeNode* node);
	ArchiveEntry* getBookmark(unsigned index);
	unsigned      numBookmarks() const { return bookmarks_.size(); }

	void onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data) override;

private:
	struct OpenArchive
	{
		shared_ptr<Archive>       archive;
		vector<weak_ptr<Archive>> open_children; // A list of currently open archives that are within this archive
		bool                      resource;
	};

	vector<OpenArchive>            open_archives_;
	unique_ptr<Archive>            program_resource_archive_;
	unique_ptr<Archive>            base_resource_archive_;
	bool                           res_archive_open_ = false;
	vector<string>                 base_resource_paths_;
	vector<string>                 recent_files_;
	vector<weak_ptr<ArchiveEntry>> bookmarks_;

	bool initArchiveFormats() const;
	void getDependentArchivesInternal(Archive* archive, vector<shared_ptr<Archive>>& vec);
};
