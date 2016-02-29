
#ifndef __MAP_BACKUP_MANAGER_H__
#define __MAP_BACKUP_MANAGER_H__

class ArchiveEntry;
class Archive;
class MapBackupManager
{
private:

public:
	MapBackupManager();
	~MapBackupManager();

	bool		writeBackup(vector<ArchiveEntry*>& map_data, string archive_name, string map_name);
	Archive*	openBackup(string archive_name, string map_name);
};

#endif//__MAP_BACKUP_MANAGER_H__
