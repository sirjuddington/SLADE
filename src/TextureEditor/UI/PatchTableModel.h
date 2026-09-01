#pragma once

#include "General/Sigslot.h"

namespace slade
{
class PatchTable;
}

namespace slade::texeditor
{
class PatchTableModel : public wxDataViewIndexListModel
{
public:
	enum class Column : u8
	{
		Index = 0,
		Name,
		UsedIn,
		Archive,

		Count
	};

	PatchTableModel()           = default;
	~PatchTableModel() override = default;

	void open(PatchTable& patch_table);

private:
	PatchTable*          patch_table_ = nullptr;
	ScopedConnectionList connections_;

	void refresh();

	// wxDataViewIndexListModel
	unsigned int GetColumnCount() const override { return static_cast<unsigned int>(Column::Count); }
	void         GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override;
	bool         SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override;
	int          Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending)
		const override;
};
} // namespace slade::texeditor
