#pragma once

namespace slade
{
class MapPreviewCanvas;
class Archive;
class ZipArchive;
class ArchiveDir;
class ListView;

class MapBackupPanel : public wxPanel
{
public:
	MapBackupPanel(wxWindow* parent);
	~MapBackupPanel() = default;

	Archive* selectedMapData() const { return archive_mapdata_.get(); }

	bool loadBackups(wxString archive_name, const wxString& map_name);
	void updateMapPreview();

private:
	MapPreviewCanvas*      canvas_map_   = nullptr;
	ListView*              list_backups_ = nullptr;
	unique_ptr<ZipArchive> archive_backups_;
	unique_ptr<Archive>    archive_mapdata_;
	ArchiveDir*            dir_current_ = nullptr;
};
} // namespace slade
