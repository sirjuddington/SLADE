
#ifndef __MAP_ENTRY_PANEL_H__
#define __MAP_ENTRY_PANEL_H__

#include "EntryPanel.h"

class MapPreviewCanvas;
class ArchiveEntry;
class MapEntryPanel : public EntryPanel
{
private:
	MapPreviewCanvas*	map_canvas;
	wxCheckBox*			cb_show_things;
	wxStaticText*		label_stats;

public:
	MapEntryPanel(wxWindow* parent);
	~MapEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	bool	createImage();
	void	toolbarButtonClick(string action_id);

	void	onCBShowThings(wxCommandEvent& e);
};

#endif//__MAP_ENTRY_PANEL_H__
