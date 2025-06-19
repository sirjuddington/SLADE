#pragma once

// Forward declarations
namespace slade
{
class Archive;
namespace database
{
	class Context;
} // namespace database
} // namespace slade
namespace SQLite
{
class Database;
}

namespace slade::database
{
// General database functions
bool   fileExists();
string programDatabasePath();
bool   init();
void   close();
void   migrateConfigs();

// archive_file table functions
i64            archiveFileId(Context& db, const string& path, i64 parent_id = -1);
i64            archiveFileId(Context& db, const Archive& archive);
time_t         archiveFileLastOpened(Context& db, i64 id);
void           setArchiveFileLastOpened(Context& db, int64_t archive_id, time_t last_opened);
i64            writeArchiveFile(Context& db, const Archive& archive);
vector<string> recentFiles(Context& db, unsigned count = 0);

// Signals
struct Signals
{
	// Table updates
	sigslot::signal<> archive_file_updated;
};
Signals& signals();

} // namespace slade::database
