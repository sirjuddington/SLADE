#pragma once

#include "EntryPanel.h"

class HexEditorPanel;

class HexEntryPanel : public EntryPanel
{
public:
	HexEntryPanel(wxWindow* parent);
	~HexEntryPanel() {}

	bool	loadEntry(ArchiveEntry* entry) override;
	bool	saveEntry() override;

private:
	HexEditorPanel*	hex_editor_ = nullptr;
};
