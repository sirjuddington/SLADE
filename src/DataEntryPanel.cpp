
#include "Main.h"
#include "WxStuff.h"
#include "DataEntryPanel.h"
#include "ColourConfiguration.h"
#include "BinaryControlLump.h"


DataEntryTable::DataEntryTable(DataEntryPanel* parent)
{
	row_stride = 0;
	data_start = 0;
	this->parent = parent;
}

DataEntryTable::~DataEntryTable()
{
}

int DataEntryTable::GetNumberRows()
{
	if (row_stride == 0)
		return 0;
	else
	{
		LOG_MESSAGE(2, "size %d, %d rows", data.getSize(), data.getSize() / row_stride);
		return data.getSize() / row_stride;
	}
}

int DataEntryTable::GetNumberCols()
{
	return columns.size();
}

string DataEntryTable::GetValue(int row, int col)
{
	if (!data.seek((row * row_stride) + columns[col].row_offset, 0))
		return "INVALID";

	// Signed integer column
	if (columns[col].type == COL_INT_SIGNED)
	{
		if (columns[col].size == 1)
		{
			int8_t val;
			data.read(&val, 1);
			return S_FMT("%d", val);
		}
		else if (columns[col].size == 2)
		{
			int16_t val;
			data.read(&val, 2);
			return S_FMT("%d", val);
		}
		else if (columns[col].size == 4)
		{
			int32_t val;
			data.read(&val, 4);
			return S_FMT("%d", val);
		}
		else if (columns[col].size == 8)
		{
			int64_t val;
			data.read(&val, 8);
			return S_FMT("%d", val);
		}
		return "INVALID SIZE";
	}
	
	// Unsigned integer column
	else if (columns[col].type == COL_INT_UNSIGNED)
	{
		if (columns[col].size == 1)
		{
			uint8_t val;
			data.read(&val, 1);
			return S_FMT("%d", val);
		}
		else if (columns[col].size == 2)
		{
			uint16_t val;
			data.read(&val, 2);
			return S_FMT("%d", val);
		}
		else if (columns[col].size == 4)
		{
			uint32_t val;
			data.read(&val, 4);
			return S_FMT("%d", val);
		}
		else if (columns[col].size == 8)
		{
			uint64_t val;
			data.read(&val, 8);
			return S_FMT("%d", val);
		}
		return "INVALID SIZE";
	}

	// Fixed-point float column
	else if (columns[col].type == COL_FIXED)
	{
		if (columns[col].size == 4)
		{
			int32_t val;
			data.read(&val, 4);
			return S_FMT("%1.3f", (double)val/65536.0);
		}
		return "INVALID SIZE";
	}

	// String column
	else if (columns[col].type == COL_STRING)
	{
		return wxString::FromAscii(data.getData() + data.currentPos(), columns[col].size);
	}

	// Custom value column
	else if (columns[col].type == COL_CUSTOM_VALUE)
	{
		int value;
		if (columns[col].size == 1)
		{
			int8_t val;
			data.read(&val, 1);
			value = val;
		}
		else if (columns[col].size == 2)
		{
			int16_t val;
			data.read(&val, 2);
			value = val;
		}
		else if (columns[col].size == 4)
		{
			int32_t val;
			data.read(&val, 4);
			value = val;
		}
		else if (columns[col].size == 8)
		{
			int64_t val;
			data.read(&val, 8);
			value = val;
		}
		return columns[col].getCustomValue(value);
	}
	
	return "UNKNOWN TYPE";
}

void DataEntryTable::SetValue(int row, int col, const string& value)
{
	// Seek to data position
	if (!data.seek((row * row_stride) + columns[col].row_offset, 0))
		return;

	// Signed integer column
	if (columns[col].type == COL_INT_SIGNED)
	{
		long long longval;
		value.ToLongLong(&longval);

		if (columns[col].size == 1)
		{
			int8_t val = longval;
			data.write(&val, 1);
		}
		else if (columns[col].size == 2)
		{
			int16_t val = longval;
			data.write(&val, 2);
		}
		else if (columns[col].size == 4)
		{
			int32_t val = longval;
			data.write(&val, 4);
		}
		else if (columns[col].size == 8)
		{
			int64_t val = longval;
			data.write(&val, 8);
		}
	}

	// Unsigned integer column
	else if (columns[col].type == COL_INT_UNSIGNED)
	{
		long long longval;
		value.ToLongLong(&longval);
		if (longval < 0)
			longval = 0;

		if (columns[col].size == 1)
		{
			uint8_t val = longval;
			data.write(&val, 1);
		}
		else if (columns[col].size == 2)
		{
			uint16_t val = longval;
			data.write(&val, 2);
		}
		else if (columns[col].size == 4)
		{
			uint32_t val = longval;
			data.write(&val, 4);
		}
		else if (columns[col].size == 8)
		{
			uint64_t val = longval;
			data.write(&val, 8);
		}
	}

	// String column
	else if (columns[col].type == COL_STRING)
	{
		char* str = new char[columns[col].size];
		unsigned minsize = min(columns[col].size, value.size());
		for (unsigned a = 0; a < minsize; a++)
			str[a] = value[a];
		data.write(str, columns[col].size);
		delete[] str;
	}

	// Set cell to modified colour
	GetView()->SetCellTextColour(WXCOL(ColourConfiguration::getColour("modified")), row, col);

	// Set entry modified
	parent->setModified(true);
}

string DataEntryTable::GetColLabelValue(int col)
{
	if ((unsigned)col < columns.size())
		return columns[col].name;

	return "Column" + col;
}

bool DataEntryTable::setupDataStructure(ArchiveEntry* entry)
{
	// Clear existing
	data.clear();
	columns.clear();
	row_stride = 0;
	data_start = 0;

	if (!entry)
		return true;

	// Load entry data
	data.write(entry->getData(), entry->getSize());

	// Setup columns
	string type = entry->getType()->getId();

	// VERTEXES
	if (type == "map_vertexes")
	{
		if (entry->exProp("MapFormat") == "doom64")
		{
			columns.push_back(dep_column_t("X Position", COL_FIXED, 4, 0));
			columns.push_back(dep_column_t("Y Position", COL_FIXED, 4, 4));
			row_stride = 8;
			data_start = 0;
		}
		else
		{
			columns.push_back(dep_column_t("X Position", COL_INT_SIGNED, 2, 0));
			columns.push_back(dep_column_t("Y Position", COL_INT_SIGNED, 2, 2));
			row_stride = 4;
			data_start = 0;
		}
	}

	// LINEDEFS
	else if (type == "map_linedefs")
	{
		// Doom format
		if (entry->exProp("MapFormat") == "doom")
		{
			columns.push_back(dep_column_t("Vertex 1", COL_INT_UNSIGNED, 2, 0));
			columns.push_back(dep_column_t("Vertex 2", COL_INT_UNSIGNED, 2, 2));
			columns.push_back(dep_column_t("Flags", COL_INT_UNSIGNED, 2, 4));
			columns.push_back(dep_column_t("Action Special", COL_INT_UNSIGNED, 2, 6));
			columns.push_back(dep_column_t("Sector Tag", COL_INT_UNSIGNED, 2, 8));
			columns.push_back(dep_column_t("Front Side", COL_INT_UNSIGNED, 2, 10));
			columns.push_back(dep_column_t("Back Side", COL_INT_UNSIGNED, 2, 12));
			row_stride = 14;
			data_start = 0;
		}

		// Hexen format
		else if (entry->exProp("MapFormat") == "hexen")
		{
			columns.push_back(dep_column_t("Vertex 1", COL_INT_UNSIGNED, 2, 0));
			columns.push_back(dep_column_t("Vertex 2", COL_INT_UNSIGNED, 2, 2));
			columns.push_back(dep_column_t("Flags", COL_INT_UNSIGNED, 2, 4));
			columns.push_back(dep_column_t("Action Special", COL_INT_UNSIGNED, 1, 6));
			columns.push_back(dep_column_t("Arg 1", COL_INT_UNSIGNED, 1, 7));
			columns.push_back(dep_column_t("Arg 2", COL_INT_UNSIGNED, 1, 8));
			columns.push_back(dep_column_t("Arg 3", COL_INT_UNSIGNED, 1, 9));
			columns.push_back(dep_column_t("Arg 4", COL_INT_UNSIGNED, 1, 10));
			columns.push_back(dep_column_t("Arg 5", COL_INT_UNSIGNED, 1, 11));
			columns.push_back(dep_column_t("Front Side", COL_INT_UNSIGNED, 2, 12));
			columns.push_back(dep_column_t("Back Side", COL_INT_UNSIGNED, 2, 14));
			row_stride = 16;
			data_start = 0;
		}

		// Doom 64 format
		else if (entry->exProp("MapFormat") == "doom64")
		{
			columns.push_back(dep_column_t("Vertex 1", COL_INT_UNSIGNED, 2, 0));
			columns.push_back(dep_column_t("Vertex 2", COL_INT_UNSIGNED, 2, 2));
			columns.push_back(dep_column_t("Flags", COL_INT_UNSIGNED, 4, 4));
			columns.push_back(dep_column_t("Action Special", COL_INT_UNSIGNED, 2, 8));
			columns.push_back(dep_column_t("Sector Tag", COL_INT_UNSIGNED, 2, 10));
			columns.push_back(dep_column_t("Front Side", COL_INT_UNSIGNED, 2, 12));
			columns.push_back(dep_column_t("Back Side", COL_INT_UNSIGNED, 2, 14));
			row_stride = 16;
			data_start = 0;
		}
	}

	// SIDEDEFS
	else if (type == "map_sidedefs")
	{
		// Doom 64 format
		if (entry->exProp("MapFormat") == "doom64")
		{
			columns.push_back(dep_column_t("X Offset", COL_INT_SIGNED, 2, 0));
			columns.push_back(dep_column_t("Y Offset", COL_INT_SIGNED, 2, 2));
			columns.push_back(dep_column_t("Upper Texture", COL_INT_UNSIGNED, 2, 4));
			columns.push_back(dep_column_t("Lower Texture", COL_INT_UNSIGNED, 2, 6));
			columns.push_back(dep_column_t("Middle Texture", COL_INT_UNSIGNED, 2, 8));
			columns.push_back(dep_column_t("Sector", COL_INT_UNSIGNED, 2, 10));
			row_stride = 12;
			data_start = 0;
		}

		// Doom/Hexen format
		else
		{
			columns.push_back(dep_column_t("X Offset", COL_INT_SIGNED, 2, 0));
			columns.push_back(dep_column_t("Y Offset", COL_INT_SIGNED, 2, 2));
			columns.push_back(dep_column_t("Upper Texture", COL_STRING, 8, 4));
			columns.push_back(dep_column_t("Lower Texture", COL_STRING, 8, 12));
			columns.push_back(dep_column_t("Middle Texture", COL_STRING, 8, 20));
			columns.push_back(dep_column_t("Sector", COL_INT_UNSIGNED, 2, 28));
			row_stride = 30;
			data_start = 0;
		}
	}

	// SECTORS
	else if (type == "map_sectors")
	{
		// Doom 64 format
		if (entry->exProp("MapFormat") == "doom64")
		{
			columns.push_back(dep_column_t("Floor Height", COL_INT_SIGNED, 2, 0));
			columns.push_back(dep_column_t("Ceiling Height", COL_INT_SIGNED, 2, 2));
			columns.push_back(dep_column_t("Floor Texture", COL_INT_UNSIGNED, 2, 4));
			columns.push_back(dep_column_t("Ceiling Texture", COL_INT_UNSIGNED, 2, 6));
			columns.push_back(dep_column_t("Colour1", COL_INT_UNSIGNED, 2, 8));
			columns.push_back(dep_column_t("Colour2", COL_INT_UNSIGNED, 2, 10));
			columns.push_back(dep_column_t("Colour3", COL_INT_UNSIGNED, 2, 12));
			columns.push_back(dep_column_t("Colour4", COL_INT_UNSIGNED, 2, 14));
			columns.push_back(dep_column_t("Colour5", COL_INT_UNSIGNED, 2, 16));
			columns.push_back(dep_column_t("Special", COL_INT_UNSIGNED, 2, 18));
			columns.push_back(dep_column_t("Tag", COL_INT_UNSIGNED, 2, 20));
			columns.push_back(dep_column_t("Flags", COL_INT_UNSIGNED, 2, 22));
			row_stride = 24;
			data_start = 0;
		}

		// Doom/Hexen format
		else
		{
			columns.push_back(dep_column_t("Floor Height", COL_INT_SIGNED, 2, 0));
			columns.push_back(dep_column_t("Ceiling Height", COL_INT_SIGNED, 2, 2));
			columns.push_back(dep_column_t("Floor Texture", COL_STRING, 8, 4));
			columns.push_back(dep_column_t("Ceiling Texture", COL_STRING, 8, 12));
			columns.push_back(dep_column_t("Light Level", COL_INT_UNSIGNED, 2, 20));
			columns.push_back(dep_column_t("Special", COL_INT_UNSIGNED, 2, 22));
			columns.push_back(dep_column_t("Tag", COL_INT_UNSIGNED, 2, 24));
			row_stride = 26;
			data_start = 0;
		}
	}

	// THINGS
	else if (type == "map_things")
	{
		// Doom format
		if (entry->exProp("MapFormat") == "doom")
		{
			columns.push_back(dep_column_t("X Position", COL_INT_SIGNED, 2, 0));
			columns.push_back(dep_column_t("Y Position", COL_INT_SIGNED, 2, 2));
			columns.push_back(dep_column_t("Direction", COL_INT_SIGNED, 2, 4));
			columns.push_back(dep_column_t("Type", COL_INT_UNSIGNED, 2, 6));
			columns.push_back(dep_column_t("Flags", COL_INT_UNSIGNED, 2, 8));
			row_stride = 10;
			data_start = 0;
		}

		// Hexen format
		else if (entry->exProp("MapFormat") == "hexen")
		{
			columns.push_back(dep_column_t("ID", COL_INT_UNSIGNED, 2, 0));
			columns.push_back(dep_column_t("X Position", COL_INT_SIGNED, 2, 2));
			columns.push_back(dep_column_t("Y Position", COL_INT_SIGNED, 2, 5));
			columns.push_back(dep_column_t("Z Height", COL_INT_SIGNED, 2, 6));
			columns.push_back(dep_column_t("Direction", COL_INT_SIGNED, 2, 8));
			columns.push_back(dep_column_t("Type", COL_INT_UNSIGNED, 2, 10));
			columns.push_back(dep_column_t("Flags", COL_INT_UNSIGNED, 2, 12));
			columns.push_back(dep_column_t("Special", COL_INT_UNSIGNED, 1, 14));
			columns.push_back(dep_column_t("Arg 1", COL_INT_UNSIGNED, 1, 15));
			columns.push_back(dep_column_t("Arg 2", COL_INT_UNSIGNED, 1, 16));
			columns.push_back(dep_column_t("Arg 3", COL_INT_UNSIGNED, 1, 17));
			columns.push_back(dep_column_t("Arg 4", COL_INT_UNSIGNED, 1, 18));
			columns.push_back(dep_column_t("Arg 5", COL_INT_UNSIGNED, 1, 19));
			row_stride = 20;
			data_start = 0;
		}

		// Doom64 format
		else
		{
			columns.push_back(dep_column_t("X Position", COL_INT_SIGNED, 2, 0));
			columns.push_back(dep_column_t("Y Position", COL_INT_SIGNED, 2, 2));
			columns.push_back(dep_column_t("Z Height", COL_INT_SIGNED, 2, 4));
			columns.push_back(dep_column_t("Direction", COL_INT_SIGNED, 2, 6));
			columns.push_back(dep_column_t("Type", COL_INT_SIGNED, 2, 8));
			columns.push_back(dep_column_t("Flags", COL_INT_SIGNED, 2, 10));
			columns.push_back(dep_column_t("ID", COL_INT_SIGNED, 2, 12));
			row_stride = 14;
			data_start = 0;
		}
	}

	// SWITCHES
	else if (type == "switches")
	{
		columns.push_back(dep_column_t("On Texture", COL_STRING, 8, 0));
		columns.push_back(dep_column_t("Off Texture", COL_STRING, 8, 9));

		dep_column_t col_type("Type", COL_CUSTOM_VALUE, 2, 18);
		col_type.addCustomValue(SWCH_COMM, "Commercial");
		col_type.addCustomValue(SWCH_FULL, "Registered");
		col_type.addCustomValue(SWCH_DEMO, "Shareware");
		columns.push_back(col_type);

		row_stride = 20;
		data_start = 0;
	}

	if (columns.empty())
		return false;

	return true;
}





DataEntryPanel::DataEntryPanel(wxWindow* parent) : EntryPanel(parent, "data")
{
	// Init variables
	table_data = new DataEntryTable(this);

	// Create grid
	grid_data = new wxGrid(this, -1);
	sizer_main->Add(grid_data, 1, wxEXPAND);

	// Add actions to toolbar
	wxArrayString actions;
	actions.Add("data_add_row");
	actions.Add("data_delete_row");
	actions.Add("data_duplicate");
	toolbar->addActionGroup("Data", actions);
}

DataEntryPanel::~DataEntryPanel()
{
	// Clean up
	delete table_data;
}

bool DataEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Load data table
	table_data->setupDataStructure(entry);
	grid_data->SetTable(table_data);
	grid_data->SetColMinimalAcceptableWidth(64);
	for (int a = 0; a < table_data->GetNumberCols(); a++)
		grid_data->AutoSizeColLabelSize(a);
	grid_data->ForceRefresh();

	return true;
}

bool DataEntryPanel::saveEntry()
{
	entry->importMemChunk(table_data->getData());
	setModified(false);
	return true;
}
