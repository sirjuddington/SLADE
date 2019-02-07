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
	bool             validResDir(const wxString& dir) const;
	Archive*         getArchive(int index);
	Archive*         getArchive(const wxString& filename);
	Archive*         openArchive(const wxString& filename, bool manage = true, bool silent = false);
	Archive*         openArchive(ArchiveEntry* entry, bool manage = true, bool silent = false);
	Archive*         openDirArchive(const wxString& dir, bool manage = true, bool silent = false);
	Archive*         newArchive(const wxString& format);
	bool             closeArchive(int index);
	bool             closeArchive(const wxString& filename);
	bool             closeArchive(Archive* archive);
	void             closeAll();
	int              numArchives() const { return (int)open_archives_.size(); }
	int              archiveIndex(Archive* archive);
	vector<Archive*> getDependentArchives(Archive* archive);
	Archive*         programResourceArchive() const { return program_resource_archive_.get(); }
	wxString         getArchiveExtensionsString() const;
	bool             archiveIsResource(Archive* archive);
	void             setArchiveResource(Archive* archive, bool resource = true);

	// General access
	const vector<wxString>&      recentFiles() const { return recent_files_; }
	const vector<wxString>&      baseResourcePaths() const { return base_resource_paths_; }
	const vector<ArchiveEntry*>& bookmarks() const { return bookmarks_; }

	// Base resource archive stuff
	Archive* baseResourceArchive() const { return base_resource_archive_.get(); }
	bool     addBaseResourcePath(const wxString& path);
	void     removeBaseResourcePath(unsigned index);
	unsigned numBaseResourcePaths() const { return base_resource_paths_.size(); }
	wxString getBaseResourcePath(unsigned index);
	bool     openBaseResource(int index);

	// Resource entry get/search
	ArchiveEntry*         getResourceEntry(const wxString& name, Archive* ignore = nullptr);
	ArchiveEntry*         findResourceEntry(Archive::SearchOptions& options, Archive* ignore = nullptr);
	vector<ArchiveEntry*> findAllResourceEntries(Archive::SearchOptions& options, Archive* ignore = nullptr);

	// Recent files
	wxString recentFile(unsigned index);
	unsigned numRecentFiles() const { return recent_files_.size(); }
	void     addRecentFile(wxString path);
	void     addRecentFiles(vector<wxString> paths);
	void     removeRecentFile(const wxString& path);

	// Bookmarks
	void          addBookmark(ArchiveEntry* entry);
	bool          deleteBookmark(ArchiveEntry* entry);
	bool          deleteBookmark(unsigned index);
	bool          deleteBookmarksInArchive(Archive* archive);
	bool          deleteBookmarksInDir(ArchiveTreeNode* node);
	ArchiveEntry* getBookmark(unsigned index);
	unsigned      numBookmarks() const { return bookmarks_.size(); }

	void onAnnouncement(Announcer* announcer, const wxString& event_name, MemChunk& event_data) override;

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
	vector<wxString>      base_resource_paths_;
	vector<wxString>      recent_files_;
	vector<ArchiveEntry*> bookmarks_;

	bool initArchiveFormats() const;
	void getDependentArchivesInternal(Archive* archive, vector<Archive*>& vec);
};
