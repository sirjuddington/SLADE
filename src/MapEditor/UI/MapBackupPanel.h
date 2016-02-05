
#ifndef __MAP_BACKUP_PANEL_H__
#define __MAP_BACKUP_PANEL_H__

#include "UI/Lists/ListView.h"

class MapPreviewCanvas;
class Archive;
class ZipArchive;
class ArchiveTreeNode;
class MapBackupPanel : public wxPanel
{
private:
	MapPreviewCanvas*	canvas_map;
	ListView*			list_backups;
	ZipArchive*			archive_backups;
	Archive*			archive_mapdata;
	ArchiveTreeNode*	dir_current;

public:
	MapBackupPanel(wxWindow* parent);
	~MapBackupPanel();

	Archive*	getSelectedMapData() { return archive_mapdata; }

	bool	loadBackups(string archive_name, string map_name);
	void	updateMapPreview();

	void	onBackupListChanged(wxListEvent& e);
};

#endif//__MAP_BACKUP_PANEL_H__
