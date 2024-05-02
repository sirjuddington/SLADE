#pragma once

namespace slade
{
struct MapPreviewData;
class ListView;

class MapBackupPanel : public wxPanel
{
public:
	MapBackupPanel(wxWindow* parent);
	~MapBackupPanel() override = default;

	Archive* selectedMapData() const { return archive_mapdata_.get(); }

	bool loadBackups(wxString archive_name, const wxString& map_name);
	void updateMapPreview();

private:
	unique_ptr<MapPreviewData> map_data_;
	wxWindow*                  canvas_map_   = nullptr;
	ListView*                  list_backups_ = nullptr;
	unique_ptr<Archive>        archive_backups_;
	unique_ptr<Archive>        archive_mapdata_;
	ArchiveDir*                dir_current_ = nullptr;
};
} // namespace slade
