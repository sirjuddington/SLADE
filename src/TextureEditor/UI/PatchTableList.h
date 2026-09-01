#pragma once

#include "UI/Lists/SDataViewCtrl.h"

namespace slade
{
class PatchTable;
}

namespace slade::texeditor
{
class PatchTableList : public ui::SDataViewCtrl
{
public:
	PatchTableList(wxWindow* parent, PatchTable* patch_table);

	int         selectedPatchIndex() const;
	vector<int> selectedPatchIndices() const;
	void        selectPatch(int index, bool select = true);

private:
	PatchTable* patch_table_ = nullptr;

	wxDataViewColumn* col_index_   = nullptr;
	wxDataViewColumn* col_name_    = nullptr;
	wxDataViewColumn* col_used_in_ = nullptr;
	wxDataViewColumn* col_archive_ = nullptr;

	const Archive* stateArchive() const override;

	void setupColumns();
};
} // namespace slade::texeditor
