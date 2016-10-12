
#ifndef __DATA_ENTRY_PANEL_H__
#define __DATA_ENTRY_PANEL_H__

#include "common.h"
#include "EntryPanel.h"
#include "General/SAction.h"

struct int_string_t
{
	int		key;
	string	value;
	int_string_t(int key, string value)
	{
		this->key = key;
		this->value = value;
	}
};

struct dep_column_t
{
	string					name;
	uint8_t					type;
	uint16_t				size;
	uint32_t				row_offset;
	vector<int_string_t>	custom_values;
	dep_column_t(string name, uint8_t type, uint16_t size, uint32_t row_offset)
	{
		this->name = name;
		this->type = type;
		this->size = size;
		this->row_offset = row_offset;
	}

	void addCustomValue(int key, string value)
	{
		custom_values.push_back(int_string_t(key, value));
	}

	string getCustomValue(int key)
	{
		for (unsigned a = 0; a < custom_values.size(); a++)
		{
			if (key == custom_values[a].key)
				return custom_values[a].value;
		}
		return "Unknown";
	}
};

class DataEntryPanel;
class DataEntryTable : public wxGridTableBase
{
private:
	MemChunk				data;
	vector<dep_column_t>	columns;
	unsigned				row_stride;
	unsigned				data_start;
	unsigned				data_stop;
	int						row_first;
	string					row_prefix;
	DataEntryPanel*			parent;
	MemChunk				data_clipboard;
	vector<point2_t>		cells_modified;
	vector<int>				rows_new;

public:
	DataEntryTable(DataEntryPanel* parent);
	virtual ~DataEntryTable();

	// Column types
	enum
	{
		COL_INT_SIGNED,
		COL_INT_UNSIGNED,
		COL_FIXED,
		COL_STRING,
		COL_BOOLEAN,
		COL_FLOAT,
		COL_CUSTOM_VALUE
	};

	// wxGridTableBase overrides
	int				GetNumberRows();
	int				GetNumberCols();
	string			GetValue(int row, int col);
	void			SetValue(int row, int col, const string& value);
	string			GetColLabelValue(int col);
	string			GetRowLabelValue(int row);
	bool			DeleteRows(size_t pos, size_t num);
	bool			InsertRows(size_t pos, size_t num);
	wxGridCellAttr*	GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind);

	MemChunk&		getData() { return data; }
	dep_column_t	getColumnInfo(int col) { return columns[col]; }
	bool			setupDataStructure(ArchiveEntry* entry);
	void			copyRows(int row, int num, bool add = false);
	void			pasteRows(int row);
};

class DataEntryPanel : public EntryPanel, SActionHandler
{
private:
	wxGrid*			grid_data;
	DataEntryTable*	table_data;
	wxComboBox*		combo_cell_value;

public:
	DataEntryPanel(wxWindow* parent);
	~DataEntryPanel();

	bool	loadEntry(ArchiveEntry* entry);
	bool	saveEntry();
	void	setModified(bool modified) { EntryPanel::setModified(modified); }

	void	deleteRow();
	void	addRow();
	void	copyRow(bool cut);
	void	pasteRow();
	void	changeValue();
	bool	handleAction(string id);
	int		getColWithSelection();

	vector<point2_t>	getSelection();

	void	onKeyDown(wxKeyEvent& e);
	void	onGridRightClick(wxGridEvent& e);
	void	onGridCursorChanged(wxGridEvent& e);
	void	onComboCellValueSet(wxCommandEvent& e);
};

#endif//__DATA_ENTRY_PANEL_H__
