#pragma once

// Forward declarations
namespace slade
{
class Archive;
namespace database
{
	class Context;
	class Statement;
} // namespace database
} // namespace slade

namespace slade::database
{
// Database model for a row in the archive_file table
struct ArchiveFile
{
	i64           id = -1;
	string        path;
	unsigned      size = 0;
	string        hash;
	string        format_id;
	time_t        last_opened   = 0;
	time_t        last_modified = 0;
	optional<i64> parent_id;

	void read(Statement& ps);
	void write();

	i64  insert();
	void update() const;
	void remove();
};

i64            archiveFileId(const string& path, optional<i64> parent_id = {});
i64            archiveFileId(const Archive& archive);
time_t         archiveFileLastOpened(i64 id);
void           setArchiveFileLastOpened(int64_t archive_id, time_t last_opened);
i64            writeArchiveFile(const Archive& archive);
vector<string> recentFiles(unsigned count = 0);

} // namespace slade::database
