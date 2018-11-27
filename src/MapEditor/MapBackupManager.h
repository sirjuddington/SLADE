#pragma once

class ArchiveEntry;
class Archive;
class MapBackupManager
{
public:
	MapBackupManager();
	~MapBackupManager();

	bool     writeBackup(vector<ArchiveEntry*>& map_data, string archive_name, string map_name);
	Archive* openBackup(string archive_name, string map_name);
};
