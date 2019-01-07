#pragma once

class ArchiveEntry;
class Archive;
class MapBackupManager
{
public:
	MapBackupManager()  = default;
	~MapBackupManager() = default;

	bool     writeBackup(vector<std::unique_ptr<ArchiveEntry>>& map_data, string archive_name, string map_name) const;
	Archive* openBackup(string archive_name, string map_name) const;
};
