#pragma once

#include "EntryPanel.h"

class ANSICanvas;

class ANSIEntryPanel : public EntryPanel
{
public:
	ANSIEntryPanel(wxWindow* parent);
	~ANSIEntryPanel() = default;

	bool loadEntry(ArchiveEntry* entry) override;
	bool saveEntry() override;

	static const int DATASIZE = 4000;

private:
	ANSICanvas*     ansi_canvas_ = nullptr;
	vector<uint8_t> ansi_chardata_;
};
