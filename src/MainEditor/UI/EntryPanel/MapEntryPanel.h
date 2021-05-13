#pragma once

#include "EntryPanel.h"

namespace slade
{
class MapPreviewCanvas;
class ArchiveEntry;

class MapEntryPanel : public EntryPanel
{
public:
	MapEntryPanel(wxWindow* parent);
	~MapEntryPanel() override = default;

	bool createImage();
	void toolbarButtonClick(const wxString& action_id) override;

protected:
	bool loadEntry(ArchiveEntry* entry) override;

private:
	MapPreviewCanvas* map_canvas_     = nullptr;
	wxCheckBox*       cb_show_things_ = nullptr;
	wxStaticText*     label_stats_    = nullptr;

	void onCBShowThings(wxCommandEvent& e);
};
} // namespace slade
