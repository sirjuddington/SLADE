
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    DataEntryPanel.cpp
 * Description: An EntryPanel for displaying entries that can be
 *              represented as a table/grid of data
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "DataEntryPanel.h"
#include "General/ColourConfiguration.h"
#include "MainEditor/BinaryControlLump.h"
#include "MainEditor/MainWindow.h"


/*******************************************************************
 * DATAENTRYTABLE CLASS FUNCTIONS
 *******************************************************************/

/* DataEntryTable::DataEntryTable
 * DataEntryTable class constructor
 *******************************************************************/
DataEntryTable::DataEntryTable(DataEntryPanel* parent)
{
	row_stride = 0;
	data_start = 0;
	data_stop = 0;
	row_first = 0;
	this->parent = parent;
}

/* DataEntryTable::~DataEntryTable
 * DataEntryTable class destructor
 *******************************************************************/
DataEntryTable::~DataEntryTable()
{
}

/* DataEntryTable::GetNumberRows
 * Returns the number of rows contained in the data
 *******************************************************************/
int DataEntryTable::GetNumberRows()
{
	if (row_stride == 0)
		return 0;
	else
		return ((data_stop ? data_stop : data.getSize()) - data_start) / row_stride;
}

/* DataEntryTable::GetNumberCols
 * Returns the number of columns for the current data type
 *******************************************************************/
int DataEntryTable::GetNumberCols()
{
	return columns.size();
}

/* DataEntryTable::GetValue
 * Returns the string value for the cell at [row,col]
 *******************************************************************/
string DataEntryTable::GetValue(int row, int col)
{
	if (!data.seek(data_start + ((row * row_stride) + columns[col].row_offset), 0))
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
		int value = 0;
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
		return S_FMT("%d: %s", value, columns[col].getCustomValue(value));
	}
	
	return "UNKNOWN TYPE";
}

/* DataEntryTable::SetValue
 * Sets the value for the cell at [row,col] to [value]
 *******************************************************************/
void DataEntryTable::SetValue(int row, int col, const string& value)
{
	// Seek to data position
	if (!data.seek((row * row_stride) + columns[col].row_offset, 0))
		return;

	// Signed integer or custom value column
	if (columns[col].type == COL_INT_SIGNED || columns[col].type == COL_CUSTOM_VALUE)
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
		unsigned minsize = MIN(columns[col].size, value.size());
		for (unsigned a = 0; a < minsize; a++)
			str[a] = value[a];
		data.write(str, columns[col].size);
		delete[] str;
	}

	// Set cell to modified colour
	bool set = true;
	for (unsigned a = 0; a < cells_modified.size(); a++)
	{
		if (cells_modified[a].x == row && cells_modified[a].y == col)
		{
			set = false;
			break;
		}
	}
	if (set)
		cells_modified.push_back(point2_t(row, col));

	// Set entry modified
	parent->setModified(true);
}

/* DataEntryTable::GetColLabelValue
 * Returns the header label text for column [col]
 *******************************************************************/
string DataEntryTable::GetColLabelValue(int col)
{
	if ((unsigned)col < columns.size())
		return columns[col].name;

	return S_FMT("Column%d", col);
}

/* DataEntryTable::GetRowLabelValue
 * Returns the header label text for [row]
 *******************************************************************/
string DataEntryTable::GetRowLabelValue(int row)
{
	return row_prefix + S_FMT("%d", row_first + row);
}

/* DataEntryTable::DeleteRows
 * Deletes [num] rows, starting at [pos]
 *******************************************************************/
bool DataEntryTable::DeleteRows(size_t pos, size_t num)
{
	// Copy existing data
	MemChunk copy;
	copy.write(data.getData(), data.getSize());

	// Write new data (excluding deleted rows)
	unsigned start = data_start + (row_stride * pos);
	unsigned end = data_start + (row_stride * (pos + num));
	data.clear();
	data.write(copy.getData(), start);
	data.write(copy.getData() + end, copy.getSize() - end);

	// Update new rows
	vector<int> new_rows_new;
	for (unsigned a = 0; a < rows_new.size(); a++)
	{
		if ((unsigned)rows_new[a] >= pos + num)
			new_rows_new.push_back(rows_new[a] - num);
		else if ((unsigned)rows_new[a] < pos)
			new_rows_new.push_back(rows_new[a]);
	}
	rows_new = new_rows_new;

	// Send message to grid
	wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, pos, num);
	GetView()->ProcessTableMessage(msg);

	return true;
}

/* DataEntryTable::InsertRows
 * Inserts [num] blank rows beginning at [pos]
 *******************************************************************/
bool DataEntryTable::InsertRows(size_t pos, size_t num)
{
	// Copy existing data
	MemChunk copy;
	copy.write(data.getData(), data.getSize());

	// Write leading rows
	unsigned start = data_start + (row_stride * pos);
	data.clear();
	data.write(copy.getData(), start);

	// Write new (blank) rows
	uint8_t* temp = new uint8_t[row_stride * num];
	memset(temp, 0, row_stride * num);
	data.write(temp, row_stride * num);
	delete[] temp;

	// Write ending rows
	data.write(copy.getData() + start, copy.getSize() - start);

	// Update new rows
	for (unsigned a = 0; a < rows_new.size(); a++)
	{
		if ((unsigned)rows_new[a] >= pos)
			rows_new[a] += num;
	}
	for (unsigned a = 0; a < num; a++)
		rows_new.push_back(pos + a);

	// Send message to grid
	wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, pos, num);
	GetView()->ProcessTableMessage(msg);

	return true;
}

/* DataEntryTable::GetAttr
 * Returns the (display) attributes for the cell at [row,col]
 *******************************************************************/
wxGridCellAttr* DataEntryTable::GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind)
{
	wxGridCellAttr* attr = new wxGridCellAttr();

	// Check if cell is part of a new row
	bool new_row = false;
	for (unsigned a = 0; a < rows_new.size(); a++)
	{
		if (rows_new[a] == row)
		{
			attr->SetTextColour(WXCOL(ColourConfiguration::getColour("new")));
			new_row = true;
			break;
		}
	}

	// Check if cell is modified
	if (!new_row)
	{
		for (unsigned a = 0; a < cells_modified.size(); a++)
		{
			if (cells_modified[a].x == row && cells_modified[a].y == col)
			{
				attr->SetTextColour(WXCOL(ColourConfiguration::getColour("modified")));
				break;
			}
		}
	}

	return attr;
}

/* DataEntryTable::setupDataStructure
 * Determines the data structure (columns etc.) for [entry]'s type
 *******************************************************************/
bool DataEntryTable::setupDataStructure(ArchiveEntry* entry)
{
	// Clear existing
	data.clear();
	data_clipboard.clear();
	cells_modified.clear();
	rows_new.clear();
	columns.clear();
	row_stride = 0;
	data_start = 0;
	data_stop = 0;
	row_first = 0;
	row_prefix = "";

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
		}
		else
		{
			columns.push_back(dep_column_t("X Position", COL_INT_SIGNED, 2, 0));
			columns.push_back(dep_column_t("Y Position", COL_INT_SIGNED, 2, 2));
			row_stride = 4;
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
			columns.push_back(dep_column_t("Floor Colour", COL_INT_UNSIGNED, 2, 8));
			columns.push_back(dep_column_t("Ceiling Colour", COL_INT_UNSIGNED, 2, 10));
			columns.push_back(dep_column_t("Thing Colour", COL_INT_UNSIGNED, 2, 12));
			columns.push_back(dep_column_t("Wall Top Colour", COL_INT_UNSIGNED, 2, 14));
			columns.push_back(dep_column_t("Wall Bottom Colour", COL_INT_UNSIGNED, 2, 16));
			columns.push_back(dep_column_t("Special", COL_INT_UNSIGNED, 2, 18));
			columns.push_back(dep_column_t("Tag", COL_INT_UNSIGNED, 2, 20));
			columns.push_back(dep_column_t("Flags", COL_INT_UNSIGNED, 2, 22));
			row_stride = 24;
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
		}
	}

	// SEGS
	else if (type == "map_segs")
	{
		columns.push_back(dep_column_t("Vertex 1", COL_INT_UNSIGNED, 2, 0));
		columns.push_back(dep_column_t("Vertex 2", COL_INT_UNSIGNED, 2, 2));
		columns.push_back(dep_column_t("Angle", COL_INT_SIGNED, 2, 4));
		columns.push_back(dep_column_t("Line", COL_INT_UNSIGNED, 2, 6));
		columns.push_back(dep_column_t("Side", COL_CUSTOM_VALUE, 2, 8));
		columns.back().addCustomValue(0, "Front");
		columns.back().addCustomValue(1, "Back");
		columns.push_back(dep_column_t("Offset", COL_INT_SIGNED, 2, 10));
		row_stride = 12;
	}

	// SSECTORS
	else if (type == "map_ssectors")
	{
		columns.push_back(dep_column_t("Seg Count", COL_INT_UNSIGNED, 2, 0));
		columns.push_back(dep_column_t("First Seg", COL_INT_UNSIGNED, 2, 2));
		row_stride = 4;
	}

	// NODES
	else if (type == "map_nodes")
	{
		columns.push_back(dep_column_t("Partition X", COL_INT_SIGNED, 2, 0));
		columns.push_back(dep_column_t("Partition Y", COL_INT_SIGNED, 2, 2));
		columns.push_back(dep_column_t("Partition X Diff", COL_INT_SIGNED, 2, 4));
		columns.push_back(dep_column_t("Partition Y Diff", COL_INT_SIGNED, 2, 6));
		columns.push_back(dep_column_t("Right Box Top", COL_INT_SIGNED, 2, 8));
		columns.push_back(dep_column_t("Right Box Bottom", COL_INT_SIGNED, 2, 10));
		columns.push_back(dep_column_t("Right Box Left", COL_INT_SIGNED, 2, 12));
		columns.push_back(dep_column_t("Right Box Right", COL_INT_SIGNED, 2, 14));
		columns.push_back(dep_column_t("Left Box Top", COL_INT_SIGNED, 2, 16));
		columns.push_back(dep_column_t("Left Box Bottom", COL_INT_SIGNED, 2, 18));
		columns.push_back(dep_column_t("Left Box Left", COL_INT_SIGNED, 2, 20));
		columns.push_back(dep_column_t("Left Box Right", COL_INT_SIGNED, 2, 22));
		columns.push_back(dep_column_t("Right Child", COL_INT_UNSIGNED, 2, 24));
		columns.push_back(dep_column_t("Left Child", COL_INT_UNSIGNED, 2, 26));
		row_stride = 28;
	}

	// LIGHTS
	else if (type == "map_lights")
	{
		columns.push_back(dep_column_t("Red", COL_INT_UNSIGNED, 1, 0));
		columns.push_back(dep_column_t("Green", COL_INT_UNSIGNED, 1, 1));
		columns.push_back(dep_column_t("Blue", COL_INT_UNSIGNED, 1, 2));
		columns.push_back(dep_column_t("Pad (Unused)", COL_INT_UNSIGNED, 1, 3));
		columns.push_back(dep_column_t("Tag", COL_INT_UNSIGNED, 2, 4));
		row_stride = 6;
	}

	// SWITCHES
	else if (type == "switches")
	{
		columns.push_back(dep_column_t("Off Texture", COL_STRING, 8, 0));
		columns.push_back(dep_column_t("On Texture", COL_STRING, 8, 9));

		dep_column_t col_type("Type", COL_CUSTOM_VALUE, 2, 18);
		col_type.addCustomValue(SWCH_DEMO, "Shareware");
		col_type.addCustomValue(SWCH_FULL, "Registered");
		col_type.addCustomValue(SWCH_COMM, "Commercial");
		columns.push_back(col_type);

		row_stride = 20;
	}

	// ANIMATED
	else if (type == "animated")
	{
		dep_column_t col_type("Type", COL_CUSTOM_VALUE, 1, 0);
		col_type.addCustomValue(0, "Flat");
		col_type.addCustomValue(1, "Texture");
		col_type.addCustomValue(2, "Flat (Decals)");
		col_type.addCustomValue(3, "Texture (Decals)");

		columns.push_back(col_type);
		columns.push_back(dep_column_t("Last Texture", COL_STRING, 8, 1));
		columns.push_back(dep_column_t("First Texture", COL_STRING, 8, 10));
		columns.push_back(dep_column_t("Speed (Tics)", COL_INT_UNSIGNED, 4, 19));
		row_stride = 23;
	}

	// PNAMES
	else if (type == "pnames" || type == "notpnames")
	{
		columns.push_back(dep_column_t("Patch Name", COL_STRING, 8, 0));
		row_stride = 8;
		data_start = 4;
	}

	// DIALOGUE
	else if (type == "map_dialog")
	{
		// Full version:
		columns.push_back(dep_column_t("Speaker ID", COL_INT_UNSIGNED, 4, 0));
		columns.push_back(dep_column_t("Drop Type", COL_INT_SIGNED, 4, 4));
		columns.push_back(dep_column_t("Item Check 1", COL_INT_SIGNED, 4, 8));
		columns.push_back(dep_column_t("Item Check 2", COL_INT_SIGNED, 4, 12));
		columns.push_back(dep_column_t("Item Check 3", COL_INT_SIGNED, 4, 16));
		columns.push_back(dep_column_t("Link", COL_INT_SIGNED, 4, 20));
		columns.push_back(dep_column_t("Speaker Name", COL_STRING, 16, 24));
		columns.push_back(dep_column_t("Sound", COL_STRING, 8, 40));
		columns.push_back(dep_column_t("Backdrop", COL_STRING, 8, 48));
		columns.push_back(dep_column_t("Dialogue Text", COL_STRING, 320, 56));
		unsigned offset = 320 + 56;
		row_stride = 1516;

		/*//Teaser version:
		columns.push_back(dep_column_t("Speaker ID", COL_INT_UNSIGNED, 4, 0));
		columns.push_back(dep_column_t("Drop Type", COL_INT_SIGNED, 4, 4));
		columns.push_back(dep_column_t("Voice Number", COL_INT_UNSIGNED, 4, 8));
		columns.push_back(dep_column_t("Speaker Name", COL_STRING, 16, 12));
		columns.push_back(dep_column_t("Dialogue Text", COL_STRING, 320, 28));
		unsigned offset = 320 + 28;
		row_stride = 1488;*/

		// Responses
		for (unsigned a = 1; a <= 5; a++)
		{
			columns.push_back(dep_column_t(S_FMT("Response %d: Give Type", a), COL_INT_SIGNED, 4, offset));
			columns.push_back(dep_column_t(S_FMT("Response %d: Item 1", a), COL_INT_SIGNED, 4, offset + 4));
			columns.push_back(dep_column_t(S_FMT("Response %d: Item 2", a), COL_INT_SIGNED, 4, offset + 8));
			columns.push_back(dep_column_t(S_FMT("Response %d: Item 3", a), COL_INT_SIGNED, 4, offset + 12));
			columns.push_back(dep_column_t(S_FMT("Response %d: Count 1", a), COL_INT_SIGNED, 4, offset + 16));
			columns.push_back(dep_column_t(S_FMT("Response %d: Count 2", a), COL_INT_SIGNED, 4, offset + 20));
			columns.push_back(dep_column_t(S_FMT("Response %d: Count 3", a), COL_INT_SIGNED, 4, offset + 24));
			columns.push_back(dep_column_t(S_FMT("Response %d: Choice Text", a), COL_STRING, 32, offset + 28));
			columns.push_back(dep_column_t(S_FMT("Response %d: Success Text", a), COL_STRING, 80, offset + 60));
			columns.push_back(dep_column_t(S_FMT("Response %d: Link", a), COL_INT_SIGNED, 4, offset + 140));
			columns.push_back(dep_column_t(S_FMT("Response %d: Log", a), COL_INT_UNSIGNED, 4, offset + 144));
			columns.push_back(dep_column_t(S_FMT("Response %d: Fail Text", a), COL_STRING, 80, offset + 148));
			offset += 228;
		}

	}

	// GENMIDI
	else if (type == "genmidi")
	{
		columns.push_back(dep_column_t("Flags", COL_INT_UNSIGNED, 2, 0));
		columns.push_back(dep_column_t("Second Tune", COL_INT_UNSIGNED, 1, 2));
		columns.push_back(dep_column_t("Fixed Note", COL_INT_UNSIGNED, 1, 3));

		// Voice data
		unsigned offset = 4;
		for (size_t i = 1; i < 3; ++i)
		{
			columns.push_back(dep_column_t(S_FMT("V%d: Mod Multi", i), COL_INT_UNSIGNED, 1, offset + 0));
			columns.push_back(dep_column_t(S_FMT("V%d: Mod Attack", i), COL_INT_UNSIGNED, 1, offset + 1));
			columns.push_back(dep_column_t(S_FMT("V%d: Mod Sustain", i), COL_INT_UNSIGNED, 1, offset + 2));
			columns.push_back(dep_column_t(S_FMT("V%d: Mod Waveform", i), COL_INT_UNSIGNED, 1, offset + 3));
			columns.push_back(dep_column_t(S_FMT("V%d: Mod Key Scale", i), COL_INT_UNSIGNED, 1, offset + 4));
			columns.push_back(dep_column_t(S_FMT("V%d: Mod Output", i), COL_INT_UNSIGNED, 1, offset + 5));
			columns.push_back(dep_column_t(S_FMT("V%d: Feedback", i), COL_INT_UNSIGNED, 1, offset + 6));
			columns.push_back(dep_column_t(S_FMT("V%d: Car Multi", i), COL_INT_UNSIGNED, 1, offset + 7));
			columns.push_back(dep_column_t(S_FMT("V%d: Car Attack", i), COL_INT_UNSIGNED, 1, offset + 8));
			columns.push_back(dep_column_t(S_FMT("V%d: Car Sustain", i), COL_INT_UNSIGNED, 1, offset + 9));
			columns.push_back(dep_column_t(S_FMT("V%d: Car Waveform", i), COL_INT_UNSIGNED, 1, offset + 10));
			columns.push_back(dep_column_t(S_FMT("V%d: Car Key Scale", i), COL_INT_UNSIGNED, 1, offset + 11));
			columns.push_back(dep_column_t(S_FMT("V%d: Car Output", i), COL_INT_UNSIGNED, 1, offset + 12));
			columns.push_back(dep_column_t(S_FMT("V%d: Note Offset", i), COL_INT_SIGNED, 2, offset + 14));
			offset += 16;
		}
		row_stride = 36;
		data_start = 8;
		data_stop = 6308;

		// There are instrument names in a second table; unfortunately retrieving them would be hard
		// (there are 6300 bytes of offset between both, plus an additional row stride of 32 bytes).
		// Commenting out all the above code and uncommenting the one below allows to see the names.
		// However seeing the values is probably more generally useful, the names are standard GM.
		/* 
		columns.push_back(dep_column_t("Instrument Name", COL_STRING, 32, 0));
		row_stride = 32;
		data_start = 6308;
		*/

	}

	if (columns.empty())
		return false;

	return true;
}

/* DataEntryTable::copyRows
 * Copies [num] rows' data beginning from [row]
 *******************************************************************/
void DataEntryTable::copyRows(int row, int num, bool add)
{
	if (!add)
		data_clipboard.clear();

	data_clipboard.write(data.getData() + data_start + (row * row_stride), num * row_stride);
}

/* DataEntryTable::pasteRows
 * Inserts any previously copied rows at [row]
 *******************************************************************/
void DataEntryTable::pasteRows(int row)
{
	// Ignore if no copied data
	if (data_clipboard.getSize() == 0)
		return;

	// Copy existing data
	MemChunk copy;
	copy.write(data.getData(), data.getSize());

	// Write leading rows
	unsigned start = data_start + (row_stride * row);
	data.clear();
	data.write(copy.getData(), start);

	// Write new rows
	data.write(data_clipboard.getData(), data_clipboard.getSize());

	// Write ending rows
	data.write(copy.getData() + start, copy.getSize() - start);

	// Update new rows
	int num = data_clipboard.getSize() / row_stride;
	for (unsigned a = 0; a < rows_new.size(); a++)
	{
		if (rows_new[a] >= row)
			rows_new[a] += num;
	}
	for (int a = 0; a < num; a++)
		rows_new.push_back(row + a);

	// Send message to grid
	wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, row, num);
	GetView()->ProcessTableMessage(msg);
}


/*******************************************************************
 * DATAENTRYPANEL CLASS FUNCTIONS
 *******************************************************************/

/* DataEntryPanel::DataEntryPanel
 * DataEntryPanel class constructor
 *******************************************************************/
DataEntryPanel::DataEntryPanel(wxWindow* parent) : EntryPanel(parent, "data")
{
	// Init variables
	table_data = NULL;

	// Cell value combo box
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	sizer_main->Add(vbox, 1, wxEXPAND);
	combo_cell_value = new wxComboBox(this, -1, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER);
	vbox->Add(combo_cell_value, 0, wxEXPAND|wxBOTTOM, 4);

	// Create grid
	grid_data = new wxGrid(this, -1);
	vbox->Add(grid_data, 1, wxEXPAND);

	// Add actions to toolbar
	wxArrayString actions;
	actions.Add("data_add_row");
	actions.Add("data_delete_row");
	actions.Add("data_cut_row");
	actions.Add("data_copy_row");
	actions.Add("data_paste_row");
	toolbar->addActionGroup("Data", actions);

	// Bind events
	Bind(wxEVT_KEY_DOWN, &DataEntryPanel::onKeyDown, this);
	grid_data->Bind(wxEVT_GRID_CELL_RIGHT_CLICK, &DataEntryPanel::onGridRightClick, this);
	grid_data->Bind(wxEVT_GRID_SELECT_CELL, &DataEntryPanel::onGridCursorChanged, this);
	combo_cell_value->Bind(wxEVT_COMBOBOX, &DataEntryPanel::onComboCellValueSet, this);
	combo_cell_value->Bind(wxEVT_TEXT_ENTER, &DataEntryPanel::onComboCellValueSet, this);
}

/* DataEntryPanel::~DataEntryPanel
 * DataEntryPanel class destructor
 *******************************************************************/
DataEntryPanel::~DataEntryPanel()
{
}

/* DataEntryPanel::loadEntry
 * Loads data from [entry] into the panel
 *******************************************************************/
bool DataEntryPanel::loadEntry(ArchiveEntry* entry)
{
	// Load data table
	if (!table_data)
		table_data = new DataEntryTable(this);
	table_data->setupDataStructure(entry);
	grid_data->ClearGrid();
	grid_data->SetTable(table_data);
	combo_cell_value->Clear();

	// Set column widths
	grid_data->SetColMinimalAcceptableWidth(64);
	for (int a = 0; a < table_data->GetNumberCols(); a++)
		grid_data->AutoSizeColLabelSize(a);
	grid_data->ForceRefresh();

	Layout();

	return true;
}

/* DataEntryPanel::saveEntry
 * Saves any changes to the entry
 *******************************************************************/
bool DataEntryPanel::saveEntry()
{
	entry->importMemChunk(table_data->getData());
	setModified(false);
	return true;
}

/* DataEntryPanel::deleteRow
 * Deletes currently selected row(s)
 *******************************************************************/
void DataEntryPanel::deleteRow()
{
	// Get selected rows
	wxArrayInt selected_rows = grid_data->GetSelectedRows();

	// Delete row(s)
	if (selected_rows.empty())
		grid_data->DeleteRows(grid_data->GetGridCursorRow(), 1);
	else
	{
		// Go through selected rows
		for (unsigned a = 0; a < selected_rows.size(); a++)
		{
			// Delete row
			grid_data->DeleteRows(selected_rows[a], 1);

			// Adjust remaining selection
			for (unsigned b = a; b < selected_rows.size(); b++)
			{
				if (selected_rows[b] > selected_rows[a])
					selected_rows[b] = selected_rows[b] - 1;
			}
		}
	}

	// Update grid
	//grid_data->UpdateDimensions();
	grid_data->ClearSelection();
	grid_data->ForceRefresh();
	setModified(true);
}

/* DataEntryPanel::addRow
 * Adds an empty row at the current selection cursor position
 *******************************************************************/
void DataEntryPanel::addRow()
{
	grid_data->InsertRows(grid_data->GetGridCursorRow(), 1);
	//grid_data->UpdateDimensions();
	grid_data->ClearSelection();
	grid_data->ForceRefresh();
	setModified(true);
}

/* DataEntryPanel::copyRow
 * Copies data from the currently selected row(s)
 *******************************************************************/
void DataEntryPanel::copyRow(bool cut)
{
	// Get selected rows
	wxArrayInt selected_rows = grid_data->GetSelectedRows();

	// Copy row(s)
	if (selected_rows.empty())
	{
		// Copy
		table_data->copyRows(grid_data->GetGridCursorRow(), 1);

		// Delete if cutting
		if (cut)
			grid_data->DeleteRows(grid_data->GetGridCursorRow(), 1);
	}
	else
	{
		// Copy
		table_data->copyRows(selected_rows[0], 1);
		for (unsigned a = 1; a < selected_rows.size(); a++)
			table_data->copyRows(selected_rows[a], 1, true);

		// Delete if cutting
		if (cut)
		{
			// Go through selected rows
			for (unsigned a = 0; a < selected_rows.size(); a++)
			{
				// Delete row
				grid_data->DeleteRows(selected_rows[a], 1);

				// Adjust remaining selection
				for (unsigned b = a; b < selected_rows.size(); b++)
				{
					if (selected_rows[b] > selected_rows[a])
						selected_rows[b] = selected_rows[b] - 1;
				}
			}
		}
	}

	// Update grid
	//grid_data->UpdateDimensions();
	grid_data->ClearSelection();
	grid_data->ForceRefresh();
	setModified(true);
}

/* DataEntryPanel::pasteRow
 * Pastes previously copied row data at the current cursor position
 *******************************************************************/
void DataEntryPanel::pasteRow()
{
	table_data->pasteRows(grid_data->GetGridCursorRow());
	//grid_data->UpdateDimensions();
	grid_data->ClearSelection();
	grid_data->ForceRefresh();
	setModified(true);
}

/* DataEntryPanel::changeValue
 * Shows a dialog to change the value of currently selected cells
 * (single-column selection only)
 *******************************************************************/
void DataEntryPanel::changeValue()
{
	// Get selection
	vector<point2_t> selection = getSelection();

	// Determine common value (if any)
	string initial_val;
	for (unsigned a = 0; a < selection.size(); a++)
	{
		string cell_value = grid_data->GetCellValue(selection[a].x, selection[a].y);
		if (initial_val.empty())
			initial_val = cell_value;
		else if (initial_val != cell_value)
		{
			initial_val = "";
			break;
		}
	}

	// Create dialog
	wxDialog dlg(theMainWindow, -1, "Change Value");

	dep_column_t ci = table_data->getColumnInfo(selection[0].y);
	wxArrayString choices;
	for (unsigned a = 0; a < ci.custom_values.size(); a++)
		choices.Add(S_FMT("%d: %s", ci.custom_values[a].key, ci.custom_values[a].value));
	wxComboBox* combo = new wxComboBox(&dlg, -1, initial_val, wxDefaultPosition, wxDefaultSize, choices);

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(vbox);
	vbox->Add(combo, 0, wxEXPAND|wxALL, 10);
	vbox->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxALL, 10);

	// Show dialog
	dlg.Fit();
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		// Get entered value
		string val = combo->GetValue();
		long lval;
		if (!val.ToLong(&lval))
		{
			// Invalid number, check for option value
			string numpart = val.BeforeFirst(':');
			if (!numpart.ToLong(&lval))
				return;
		}

		// Apply value to selected cells
		for (unsigned a = 0; a < selection.size(); a++)
			grid_data->SetCellValue(selection[a].x, selection[a].y, S_FMT("%d", lval));
		grid_data->ForceRefresh();
	}
}

/* DataEntryPanel::handleAction
 * Handles any SAction messages (from the panel toolbar)
 *******************************************************************/
bool DataEntryPanel::handleAction(string action_id)
{
	if (action_id == "data_add_row")
		addRow();
	else if (action_id == "data_delete_row")
		deleteRow();
	else if (action_id == "data_copy_row")
		copyRow(false);
	else if (action_id == "data_cut_row")
		copyRow(true);
	else if (action_id == "data_paste_row")
		pasteRow();
	else if (action_id == "data_change_value")
		changeValue();
	else
		return false;

	return true;
}

/* DataEntryPanel::getColWithSelection
 * Returns the column of the current selection (-1 if selection
 * spans multiple columns)
 *******************************************************************/
int DataEntryPanel::getColWithSelection()
{
	vector<point2_t> selection = getSelection();

	if (selection.empty())
		return -1;

	int col = -1;
	for (unsigned a = 0; a < selection.size(); a++)
	{
		if (col < 0)
			col = selection[a].y;
		else if (col != selection[a].y)
			return -1;
	}

	return col;
}

/* DataEntryPanel::getSelection
 * Gets the positions of the currently selected cells
 *******************************************************************/
vector<point2_t> DataEntryPanel::getSelection()
{
	vector<point2_t> selection;

	// Just go through entire grid
	int rows = table_data->GetNumberRows();
	int cols = table_data->GetNumberCols();
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			if (grid_data->IsInSelection(r, c))
				selection.push_back(point2_t(r, c));
		}
	}

	// If no selection, add current cursor cell
	if (selection.empty())
		selection.push_back(point2_t(grid_data->GetGridCursorRow(), grid_data->GetGridCursorCol()));

	return selection;
}


/*******************************************************************
 * DATAENTRYPANEL CLASS EVENTS
 *******************************************************************/

/* DataEntryPanel::onKeyDown
 * Called when a key is pressed in the panel
 *******************************************************************/
void DataEntryPanel::onKeyDown(wxKeyEvent& e)
{
	// Cut
	if (e.CmdDown() && e.GetKeyCode() == 'X')
		copyRow(true);

	// Copy
	else if (e.CmdDown() && e.GetKeyCode() == 'C')
		copyRow(false);

	// Paste
	else if (e.CmdDown() && e.GetKeyCode() == 'V')
		pasteRow();

	else
		e.Skip();
}

/* DataEntryPanel::onGridRightClick
 * Called when the right mouse button is clicked on the grid
 *******************************************************************/
void DataEntryPanel::onGridRightClick(wxGridEvent& e)
{
	// Check if only one column is selected
	int col = getColWithSelection();
	LOG_MESSAGE(2, "Column %d", col);

	wxMenu menu;
	theApp->getAction("data_add_row")->addToMenu(&menu);
	theApp->getAction("data_delete_row")->addToMenu(&menu);
	menu.AppendSeparator();
	theApp->getAction("data_cut_row")->addToMenu(&menu);
	theApp->getAction("data_copy_row")->addToMenu(&menu);
	theApp->getAction("data_paste_row")->addToMenu(&menu);
	if (col >= 0)
	{
		menu.AppendSeparator();
		theApp->getAction("data_change_value")->addToMenu(&menu);
	}
	PopupMenu(&menu);
}

/* DataEntryPanel::onGridCursorChanged
 * Called when the grid cursor changes cell
 *******************************************************************/
void DataEntryPanel::onGridCursorChanged(wxGridEvent& e)
{
	combo_cell_value->Clear();
	dep_column_t col = table_data->getColumnInfo(e.GetCol());
	for (unsigned a = 0; a < col.custom_values.size(); a++)
		combo_cell_value->AppendString(S_FMT("%d: %s", col.custom_values[a].key, col.custom_values[a].value));

	combo_cell_value->SetValue(grid_data->GetCellValue(e.GetRow(), e.GetCol()));
}

/* DataEntryPanel::onComboCellValueSet
 * Called when the cell value combo is changed (enter pressed or an
 * option selected from the dropdown)
 *******************************************************************/
void DataEntryPanel::onComboCellValueSet(wxCommandEvent& e)
{
	int row = grid_data->GetGridCursorRow();
	int col = grid_data->GetGridCursorCol();
	grid_data->SetCellValue(row, col, combo_cell_value->GetValue());
	combo_cell_value->SetValue(grid_data->GetCellValue(row, col));
}
