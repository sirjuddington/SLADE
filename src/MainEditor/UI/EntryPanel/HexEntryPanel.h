#pragma once

#include "EntryPanel.h"

namespace slade
{
class HexEditorPanel;

class HexEntryPanel : public EntryPanel
{
public:
	HexEntryPanel(wxWindow* parent);
	~HexEntryPanel() = default;

	bool saveEntry() override;

protected:
	bool loadEntry(ArchiveEntry* entry) override;

private:
	HexEditorPanel* hex_editor_ = nullptr;
};
} // namespace slade
