#pragma once

#include "EntryPanel.h"
#include "General/SAction.h"

class DataEntryPanel;

class DataEntryTable : public wxGridTableBase
{
public:
	struct Column
	{
		string   name;
		uint8_t  type;
		uint16_t size;
		uint32_t row_offset;

		vector<std::pair<int, string>> custom_values;

		Column(string name, uint8_t type, uint16_t size, uint32_t row_offset)
		{
			this->name       = name;
			this->type       = type;
			this->size       = size;
			this->row_offset = row_offset;
		}

		void addCustomValue(int key, string value) { custom_values.push_back({ key, value }); }

		string getCustomValue(int key)
		{
			for (unsigned a = 0; a < custom_values.size(); a++)
			{
				if (key == custom_values[a].first)
					return custom_values[a].second;
			}
			return "Unknown";
		}
	};

	DataEntryTable(DataEntryPanel* parent) : parent_{ parent } {}
	virtual ~DataEntryTable() {}

	// Column types
	enum ColType
	{
		IntSigned,
		IntUnsigned,
		Fixed,
		String,
		Boolean,
		Float,
		CustomValue
	};

	// wxGridTableBase overrides
	int             GetNumberRows() override;
	int             GetNumberCols() override;
	string          GetValue(int row, int col) override;
	void            SetValue(int row, int col, const string& value) override;
	string          GetColLabelValue(int col) override;
	string          GetRowLabelValue(int row) override;
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
	string          row_prefix_;
	DataEntryPanel* parent_ = nullptr;
	MemChunk        data_clipboard_;
	vector<Vec2i>   cells_modified_;
	vector<int>     rows_new_;
};

class DataEntryPanel : public EntryPanel, SActionHandler
{
public:
	DataEntryPanel(wxWindow* parent);
	~DataEntryPanel() {}

	bool loadEntry(ArchiveEntry* entry) override;
	bool saveEntry() override;
	void setDataModified(bool modified) { EntryPanel::setModified(modified); }

	void deleteRow();
	void addRow();
	void copyRow(bool cut);
	void pasteRow();
	void changeValue();
	bool handleAction(string id) override;
	int  getColWithSelection();

	vector<Vec2i> selection();

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
