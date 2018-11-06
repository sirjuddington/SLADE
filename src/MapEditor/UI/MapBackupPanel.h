#pragma once

class MapPreviewCanvas;
class Archive;
class ZipArchive;
class ArchiveTreeNode;
class ListView;

class MapBackupPanel : public wxPanel
{
public:
	MapBackupPanel(wxWindow* parent);
	~MapBackupPanel() {}

	Archive*	getSelectedMapData() { return archive_mapdata_; }

	bool	loadBackups(string archive_name, string map_name);
	void	updateMapPreview();

private:
	MapPreviewCanvas*			canvas_map_			= nullptr;
	ListView*					list_backups_		= nullptr;
	std::unique_ptr<ZipArchive>	archive_backups_;
	Archive*					archive_mapdata_	= nullptr;
	ArchiveTreeNode*			dir_current_		= nullptr;
};
