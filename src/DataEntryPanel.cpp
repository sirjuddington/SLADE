
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
#include "WxStuff.h"
#include "DataEntryPanel.h"
#include "ColourConfiguration.h"
#include "BinaryControlLump.h"


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
	{
		//LOG_MESSAGE(2, "size %d, %d rows", data.getSize(), data.getSize() / row_stride);
		return (data.getSize() - data_start) / row_stride;
	}
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

/* DataEntryTable::GetColLabelValue
 * Returns the header label text for column [col]
 *******************************************************************/
string DataEntryTable::GetColLabelValue(int col)
{
	if ((unsigned)col < columns.size())
		return columns[col].name;

	return "Column" + col;
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

	// Send message to grid
	wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, pos, num);
	GetView()->ProcessTableMessage(msg);

	return true;
}

/* DataEntryTable::setupDataStructure
 * Determines the data structure (columns etc.) for [entry]'s type
 *******************************************************************/
bool DataEntryTable::setupDataStructure(ArchiveEntry* entry)
{
	// Clear existing
	data.clear();
	data_clipboard.clear();
	columns.clear();
	row_stride = 0;
	data_start = 0;
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
		data_start = 0;
	}

	// SSECTORS
	else if (type == "map_ssectors")
	{
		columns.push_back(dep_column_t("Seg Count", COL_INT_UNSIGNED, 2, 0));
		columns.push_back(dep_column_t("First Seg", COL_INT_UNSIGNED, 2, 2));
		row_stride = 4;
		data_start = 0;
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
		data_start = 0;
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

	// ANIMATED
	else if (type == "animated")
	{
		dep_column_t col_type("Type", COL_CUSTOM_VALUE, 1, 0);
		col_type.addCustomValue(0, "Flat");
		col_type.addCustomValue(1, "Texture");
		col_type.addCustomValue(2, "Flat (Decals)");
		col_type.addCustomValue(3, "Texture (Decals)");

		columns.push_back(col_type);
		columns.push_back(dep_column_t("First Texture", COL_STRING, 8, 1));
		columns.push_back(dep_column_t("Last Texture", COL_STRING, 8, 10));
		columns.push_back(dep_column_t("Speed (Tics)", COL_INT_UNSIGNED, 4, 19));
		row_stride = 23;
		data_start = 0;
	}

	// PNAMES
	else if (type == "pnames")
	{
		columns.push_back(dep_column_t("Patch Name", COL_STRING, 8, 0));
		row_stride = 8;
		data_start = 4;
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

	// Send message to grid
	int num = data_clipboard.getSize() / row_stride;
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

	// Create grid
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer_main->Add(hbox, 1, wxEXPAND);
	grid_data = new wxGrid(this, -1);
	hbox->Add(grid_data, 1, wxEXPAND);

	// Add actions to toolbar
	wxArrayString actions;
	actions.Add("data_add_row");
	actions.Add("data_delete_row");
	actions.Add("data_cut_row");
	actions.Add("data_copy_row");
	actions.Add("data_paste_row");
	toolbar->addActionGroup("Data", actions);
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
	grid_data->SetTable(table_data);
	grid_data->SetColMinimalAcceptableWidth(64);
	for (int a = 0; a < table_data->GetNumberCols(); a++)
		grid_data->AutoSizeColLabelSize(a);
	grid_data->ForceRefresh();

	Layout();

	// Bind events
	Bind(wxEVT_KEY_DOWN, &DataEntryPanel::onKeyDown, this);

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
	grid_data->UpdateDimensions();
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
	grid_data->UpdateDimensions();
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
		for (unsigned a = 0; a < selected_rows.size(); a++)
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
	grid_data->UpdateDimensions();
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
	grid_data->UpdateDimensions();
	grid_data->ClearSelection();
	grid_data->ForceRefresh();
	setModified(true);
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
	else
		return false;

	return true;
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
