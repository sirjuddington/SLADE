#pragma once

#include "EntryPanel.h"

class DataEntryPanel;

class DataEntryTable : public wxGridTableBase
{
public:
	// Column types
	enum class ColType
	{
		IntSigned,
		IntUnsigned,
		Fixed,
		String,
		Boolean,
		Float,
		CustomValue
	};

	struct Column
	{
		wxString name;
		ColType  type;
		uint16_t size;
		uint32_t row_offset;

		vector<std::pair<int, wxString>> custom_values;

		Column(const wxString& name, ColType type, uint16_t size, uint32_t row_offset) :
			name{ name },
			type{ type },
			size{ size },
			row_offset{ row_offset }
		{
		}

		void addCustomValue(int key, wxString value) { custom_values.emplace_back(key, value); }

		wxString customValue(int key)
		{
			for (auto& custom_value : custom_values)
			{
				if (key == custom_value.first)
					return custom_value.second;
			}
			return "Unknown";
		}
	};

	DataEntryTable(DataEntryPanel* parent) : parent_{ parent } {}
	virtual ~DataEntryTable() = default;

	// wxGridTableBase overrides
	int             GetNumberRows() override;
	int             GetNumberCols() override;
	wxString        GetValue(int row, int col) override;
	void            SetValue(int row, int col, const wxString& value) override;
	wxString        GetColLabelValue(int col) override;
	wxString        GetRowLabelValue(int row) override;
	bool            DeleteRows(size_t pos, size_t num) override;
	bool            InsertRows(size_t pos, size_t num) override;
	wxGridCellAttr* GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind) override;

	MemChunk& data() { return data_; }
	Column    columnInfo(int col) { return columns_[col]; }
	bool      setupDataStructure(ArchiveEntry* entry);
	void      copyRows(int row, int num, bool add = false);
	void      pasteRows(int row);

private:
	MemChunk        data_;
	vector<Column>  columns_;
	unsigned        row_stride_ = 0;
	unsigned        data_start_ = 0;
	unsigned        data_stop_  = 0;
	int             row_first_  = 0;
	wxString        row_prefix_;
	DataEntryPanel* parent_ = nullptr;
	MemChunk        data_clipboard_;
	vector<Vec2i>   cells_modified_;
	vector<int>     rows_new_;
};

class DataEntryPanel : public EntryPanel
{
public:
	DataEntryPanel(wxWindow* parent);
	~DataEntryPanel() = default;

	bool loadEntry(ArchiveEntry* entry) override;
	bool saveEntry() override;
	void setDataModified(bool modified) { EntryPanel::setModified(modified); }

	void deleteRow();
	void addRow();
	void copyRow(bool cut);
	void pasteRow();
	void changeValue() const;
	bool handleEntryPanelAction(const wxString& id) override;
	int  getColWithSelection() const;

	vector<Vec2i> selection() const;

private:
	wxGrid*         grid_data_        = nullptr;
	DataEntryTable* table_data_       = nullptr;
	wxComboBox*     combo_cell_value_ = nullptr;

	// Events
	void onKeyDown(wxKeyEvent& e);
	void onGridRightClick(wxGridEvent& e);
	void onGridCursorChanged(wxGridEvent& e);
	void onComboCellValueSet(wxCommandEvent& e);
};
