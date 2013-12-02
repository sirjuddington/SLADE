
#ifndef __MAP_ENTRY_PANEL_H__
#define __MAP_ENTRY_PANEL_H__

#include "EntryPanel.h"
#include "MapPreviewCanvas.h"

class MapEntryPanel : public EntryPanel
{
private:
	MapPreviewCanvas*	map_canvas;

public:
	MapEntryPanel(wxWindow* parent);
	~MapEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	bool	createImage();
	void	toolbarButtonClick(string action_id);
};

#endif//__MAP_ENTRY_PANEL_H__
