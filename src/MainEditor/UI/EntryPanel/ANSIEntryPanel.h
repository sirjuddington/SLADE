
#ifndef __ANSI_ENTRY_PANEL__
#define __ANSI_ENTRY_PANEL__

#include "EntryPanel.h"
#include "UI/Canvas/ANSICanvas.h"

class ANSIEntryPanel : public EntryPanel
{
private:
	ANSICanvas*		ansi_canvas;
	uint8_t*		ansi_chardata;

public:
	ANSIEntryPanel(wxWindow* parent);
	~ANSIEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	void	drawCharacter(size_t index);
};

#endif//__ANSI_ENTRY_PANEL__
