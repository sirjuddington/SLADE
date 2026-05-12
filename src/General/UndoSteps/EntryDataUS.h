#pragma once

#include "General/UndoRedo.h"

namespace slade
{
class EntryDataUS : public UndoStep
{
public:
	EntryDataUS(ArchiveEntry* entry);

	bool swapData();
	bool doUndo() override { return swapData(); }
	bool doRedo() override { return swapData(); }

private:
	MemChunk data_;
	string   path_;
	int      index_   = -1;
	Archive* archive_ = nullptr;
};
} // namespace slade
