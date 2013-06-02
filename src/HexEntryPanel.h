
#ifndef __HEX_ENTRY_PANEL__
#define __HEX_ENTRY_PANEL__

#include "EntryPanel.h"
#include "HexEditorPanel.h"

class HexEntryPanel : public EntryPanel
{
private:
	HexEditorPanel*	hex_editor;

public:
	HexEntryPanel(wxWindow* parent);
	~HexEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
};

#endif//__HEX_ENTRY_PANEL__
