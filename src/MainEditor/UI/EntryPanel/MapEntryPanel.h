#pragma once

#include "EntryPanel.h"

class MapPreviewCanvas;
class ArchiveEntry;

class MapEntryPanel : public EntryPanel
{
public:
	MapEntryPanel(wxWindow* parent);
	~MapEntryPanel() {}

	bool	loadEntry(ArchiveEntry* entry) override;
	bool	saveEntry() override;
	bool	createImage();
	void	toolbarButtonClick(string action_id) override;

private:
	MapPreviewCanvas*	map_canvas_		= nullptr;
	wxCheckBox*			cb_show_things_	= nullptr;
	wxStaticText*		label_stats_	= nullptr;

	void	onCBShowThings(wxCommandEvent& e);
};
