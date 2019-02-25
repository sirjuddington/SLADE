#pragma once

class ArchiveEntry;
class Archive;
class MapBackupManager
{
public:
	MapBackupManager()  = default;
	~MapBackupManager() = default;

	bool writeBackup(
		vector<std::unique_ptr<ArchiveEntry>>& map_data,
		std::string_view                       archive_name,
		std::string_view                       map_name) const;
	Archive* openBackup(std::string_view archive_name, std::string_view map_name) const;
};
