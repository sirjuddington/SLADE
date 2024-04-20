#pragma once

#include "EntryPanel.h"

namespace slade
{
class AnsiCanvas;

class ANSIEntryPanel : public EntryPanel
{
public:
	ANSIEntryPanel(wxWindow* parent);
	~ANSIEntryPanel() override = default;

	static constexpr int DATASIZE = 4000;

protected:
	bool loadEntry(ArchiveEntry* entry) override;
	bool writeEntry(ArchiveEntry& entry) override;

private:
	AnsiCanvas*     ansi_canvas_ = nullptr;
	vector<uint8_t> ansi_chardata_;
};
} // namespace slade
