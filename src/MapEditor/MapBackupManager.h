#pragma once

class ArchiveEntry;
class Archive;
class MapBackupManager
{
public:
	MapBackupManager()  = default;
	~MapBackupManager() = default;

	bool writeBackup(vector<std::unique_ptr<ArchiveEntry>>& map_data, wxString archive_name, wxString map_name) const;
	Archive* openBackup(wxString archive_name, wxString map_name) const;
};
