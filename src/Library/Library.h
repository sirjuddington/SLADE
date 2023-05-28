#pragma once

namespace slade
{
class Archive;
class ArchiveEntry;
class Tokenizer;

namespace library
{
	struct Signals
	{
		sigslot::signal<int64_t> archive_file_updated;
		sigslot::signal<int64_t> archive_file_inserted;
		sigslot::signal<int64_t> archive_file_deleted;
	};

	// General
	void     init();
	Signals& signals();

	// Archives
	int64_t        readArchiveInfo(const Archive& archive);
	void           setArchiveLastOpenedTime(int64_t archive_id, time_t last_opened);
	int64_t        writeArchiveInfo(const Archive& archive);
	void           writeArchiveEntryInfo(const Archive& archive);
	void           writeArchiveMapInfo(const Archive& archive);
	void           removeMissingArchives();
	vector<string> recentFiles(unsigned count = 20);

	// Archive Dir Scan
	void scanArchivesInDir(string_view path, const vector<string>& ignore_ext, bool rebuild = false);
	void stopArchiveDirScan();
	bool archiveDirScanRunning();

	// Bookmarks
	vector<int64_t> bookmarkedEntries(int64_t archive_id);
	void            addBookmark(int64_t archive_id, int64_t entry_id);
	void            removeBookmark(int64_t archive_id, int64_t entry_id);
	void            removeArchiveBookmarks(int64_t archive_id);
	void            writeArchiveBookmarks(int64_t archive_id, const vector<int64_t>& entry_ids);

	// Entries
	string findEntryTypeId(const slade::ArchiveEntry& entry);

	// Recent Files (for pre-3.3.0 compatibility, remove in 3.4.0)
	void readPre330RecentFiles(Tokenizer& tz);
} // namespace library
} // namespace slade
