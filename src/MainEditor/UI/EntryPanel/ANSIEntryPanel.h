#pragma once

#include "EntryPanel.h"

namespace slade
{
class ANSICanvas;

class ANSIEntryPanel : public EntryPanel
{
public:
	ANSIEntryPanel(wxWindow* parent, bool frame = true);
	~ANSIEntryPanel() = default;

	bool saveEntry() override;

	static const int DATASIZE = 4000;

protected:
	bool loadEntry(ArchiveEntry* entry) override;

private:
	ANSICanvas*     ansi_canvas_ = nullptr;
	vector<uint8_t> ansi_chardata_;
};
} // namespace slade
