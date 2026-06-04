#pragma once

namespace slade
{
class MapBackupManager
{
public:
	MapBackupManager()  = default;
	~MapBackupManager() = default;

	bool writeBackup(const vector<unique_ptr<ArchiveEntry>>& map_data, string_view archive_name, string_view map_name)
		const;
	unique_ptr<Archive> openBackup(string_view archive_name, string_view map_name) const;
};
} // namespace slade
