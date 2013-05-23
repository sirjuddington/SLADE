
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    HexEditorPanel.cpp
 * Description: HexEditorPanel class. A panel that displays data
 *              in a hex grid, and shows some basic information
 *              about the currently selected byte
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
#include "HexEditorPanel.h"
#include "Console.h"
#include "CodePages.h"
#include "ArchiveManager.h"
#include <wx/numdlg.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
const int NUMCOLS = 16;


/*******************************************************************
 * HEXTABLE CLASS FUNCTIONS
 *******************************************************************
 * HexTable simply provides data for the wxGrid in HexEditorPanel to display
 */

/* HexTable::HexTable
 * HexTable class constructor
 *******************************************************************/
HexTable::HexTable()
{
}

/* HexTable::~HexTable
 * HexTable class destructor
 *******************************************************************/
HexTable::~HexTable()
{
}

/* HexTable::GetNumberRows
 * Returns the number of rows in the grid
 *******************************************************************/
int HexTable::GetNumberRows()
{
	return (data.getSize() / NUMCOLS) + 1;
}

/* HexTable::GetNumberCols
 * Returns the number of columns in the grid (always 16)
 *******************************************************************/
int HexTable::GetNumberCols()
{
	return NUMCOLS;
}

/* HexTable::GetValue
 * Returns the value of the byte at [row],[col] as a hex string
 *******************************************************************/
string HexTable::GetValue(int row, int col)
{
	if (unsigned(row*NUMCOLS+col) >= data.getSize())
		return "";
	else
	{
		uint8_t val = data[row*NUMCOLS + col];
		return S_FMT("%02X", val);
	}
}

/* HexTable::SetValue
 * Sets the value of the byte at [row],[col]
 * (does nothing, only here because it's required)
 *******************************************************************/
void HexTable::SetValue(int row, int col, const string& value)
{
	// Can't set values
}

/* HexTable::loadData
 * Loads in data from [mc]. Returns true on success, false otherwise
 *******************************************************************/
bool HexTable::loadData(MemChunk& mc)
{
	data.importMem(mc.getData(), mc.getSize());
	return true;
}

/* HexTable::getOffset
 * Returns the offset of the byte at [row],[col]
 *******************************************************************/
uint32_t HexTable::getOffset(int row, int col)
{
	return row*NUMCOLS + col;
}

/* HexTable::getUByteValue
 * Returns the value at [offset] as an unsigned byte
 *******************************************************************/
uint8_t HexTable::getUByteValue(uint32_t offset)
{
	if (offset < data.getSize())
		return data[offset];
	else
		return 0;
}

/* HexTable::getUShortValue
 * Returns the value at [offset] as an unsigned short
 *******************************************************************/
uint16_t HexTable::getUShortValue(uint32_t offset)
{
	uint16_t val = 0;
	if (offset < data.getSize()-1)
		memcpy(&val, data.getData()+offset, 2);
	return val;
}

/* HexTable::getUInt32Value
 * Returns the value at [offset] as an unsigned 32-bit integer
 *******************************************************************/
uint32_t HexTable::getUInt32Value(uint32_t offset)
{
	uint32_t val = 0;
	if (offset < data.getSize()-3)
		memcpy(&val, data.getData()+offset, 4);
	return val;
}

/* HexTable::getUInt64Value
 * Returns the value at [offset] as an unsigned 64-bit integer
 *******************************************************************/
uint64_t HexTable::getUInt64Value(uint32_t offset)
{
	uint64_t val = 0;
	if (offset < data.getSize()-7)
		memcpy(&val, data.getData()+offset, 8);
	return val;
}

/* HexTable::getByteValue
 * Returns the value at [offset] as a signed byte
 *******************************************************************/
int8_t HexTable::getByteValue(uint32_t offset)
{
	int8_t val = 0;
	if (offset < data.getSize())
		val = *(data.getData() + offset);
	return val;
}

/* HexTable::getShortValue
 * Returns the value at [offset] as a signed short
 *******************************************************************/
int16_t HexTable::getShortValue(uint32_t offset)
{
	int16_t val = 0;
	if (offset < data.getSize()-1)
		memcpy(&val, data.getData()+offset, 2);
	return val;
}

/* HexTable::getInt32Value
 * Returns the value at [offset] as a signed 32-bit integer
 *******************************************************************/
int32_t HexTable::getInt32Value(uint32_t offset)
{
	int32_t val = 0;
	if (offset < data.getSize()-3)
		memcpy(&val, data.getData()+offset, 4);
	return val;
}

/* HexTable::getInt64Value
 * Returns the value at [offset] as a signed 64-bit integer
 *******************************************************************/
int64_t HexTable::getInt64Value(uint32_t offset)
{
	int64_t val = 0;
	if (offset < data.getSize()-7)
		memcpy(&val, data.getData()+offset, 8);
	return val;
}

/* HexTable::getFloatValue
 * Returns the value at [offset] as a float
 *******************************************************************/
float HexTable::getFloatValue(uint32_t offset)
{
	float val = 0;
	if (offset < data.getSize()-3)
		val = *(data.getData() + offset);
	return val;
}

/* HexTable::getDoubleValue
 * Returns the value at [offset] as a double
 *******************************************************************/
double HexTable::getDoubleValue(uint32_t offset)
{
	double val = 0;
	if (offset < data.getSize()-7)
		val = *(data.getData() + offset);
	return val;
}


/*******************************************************************
 * HEXEDITORPANEL CLASS FUNCTIONS
 *******************************************************************/

/* HexEditorPanel::HexEditorPanel
 * HexEditorPanel class constructor
 *******************************************************************/
HexEditorPanel::HexEditorPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Setup layout
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Create hex table
	table_hex = new HexTable();

	// Create hex grid
	grid_hex = new wxGrid(this, -1, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS|wxBORDER_SUNKEN);
	sizer->Add(grid_hex, 0, wxEXPAND|wxALL, 4);

	// Setup hex grid
	grid_hex->SetDefaultRowSize(26, true);
	grid_hex->SetDefaultColSize(26, true);
	grid_hex->HideColLabels();
	grid_hex->HideRowLabels();
	grid_hex->EnableEditing(false);
	grid_hex->DisableDragGridSize();
	grid_hex->SetDefaultCellAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
	grid_hex->SetTable(table_hex);
	grid_hex->SetInitialSize(wxSize(27*NUMCOLS+8, -1));

	// Info frames
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND|wxALL);

	wxStaticBox* frame = new wxStaticBox(this, -1, "Values (General)");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	vbox->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	label_offset = new wxStaticText(this, -1, "Offset:");
	label_byte = new wxStaticText(this, -1, "Signed Byte:");
	label_ubyte = new wxStaticText(this, -1, "Unsigned Byte:");
	label_ascii = new wxStaticText(this, -1, "ASCII:");
	framesizer->Add(label_offset, 0, wxEXPAND|wxALL, 4);
	framesizer->Add(label_byte, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	framesizer->Add(label_ubyte, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	framesizer->Add(label_ascii, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	frame = new wxStaticBox(this, -1, "Values (Little Endian)");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	vbox->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	label_short_le = new wxStaticText(this, -1, "Signed Short:");
	label_ushort_le = new wxStaticText(this, -1, "Unsigned Short:");
	label_int32_le = new wxStaticText(this, -1, "Signed Integer (32bit):");
	label_uint32_le = new wxStaticText(this, -1, "Unsigned Integer (32bit):");
	//label_int64_le = new wxStaticText(this, -1, "Signed Integer (64bit):");
	//label_uint64_le = new wxStaticText(this, -1, "Unsigned Integer (64bit):");
	//label_float_le = new wxStaticText(this, -1, "Float:");
	//label_double_le = new wxStaticText(this, -1, "Double:");
	framesizer->Add(label_short_le, 0, wxEXPAND|wxALL, 4);
	framesizer->Add(label_ushort_le, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	framesizer->Add(label_int32_le, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	framesizer->Add(label_uint32_le, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//framesizer->Add(label_int64_le, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//framesizer->Add(label_uint64_le, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//framesizer->Add(label_float_le, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//framesizer->Add(label_double_le, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	frame = new wxStaticBox(this, -1, "Values (Big Endian)");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	vbox->Add(framesizer, 0, wxEXPAND|wxALL, 4);

	label_short_be = new wxStaticText(this, -1, "Signed Short:");
	label_ushort_be = new wxStaticText(this, -1, "Unsigned Short:");
	label_int32_be = new wxStaticText(this, -1, "Signed Integer (32bit):");
	label_uint32_be = new wxStaticText(this, -1, "Unsigned Integer (32bit):");
	//label_int64_be = new wxStaticText(this, -1, "Signed Integer (64bit):");
	//label_uint64_be = new wxStaticText(this, -1, "Unsigned Integer (64bit):");
	//label_float_be = new wxStaticText(this, -1, "Float:");
	//label_double_be = new wxStaticText(this, -1, "Double:");
	framesizer->Add(label_short_be, 0, wxEXPAND|wxALL, 4);
	framesizer->Add(label_ushort_be, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	framesizer->Add(label_int32_be, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	framesizer->Add(label_uint32_be, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//framesizer->Add(label_int64_be, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//framesizer->Add(label_uint64_be, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//framesizer->Add(label_float_be, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);
	//framesizer->Add(label_double_be, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);


	// Add 'Go to Offset' button
	btn_go_to_offset = new wxButton(this, -1, "Go to Offset...");
	vbox->Add(btn_go_to_offset, 0, wxALL, 4);

	// Bind events
	grid_hex->Bind(wxEVT_GRID_SELECT_CELL, &HexEditorPanel::onCellSelected, this);
	btn_go_to_offset->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &HexEditorPanel::onBtnGoToOffset, this);

	SetInitialSize(wxDefaultSize);
	Layout();
}

/* HexEditorPanel::~HexEditorPanel
 * HexEditorPanel class destructor
 *******************************************************************/
HexEditorPanel::~HexEditorPanel()
{
}

/* HexEditorPanel::loadData
 * Loads data from [mc] into the hex grid
 *******************************************************************/
bool HexEditorPanel::loadData(MemChunk& mc)
{
	if (table_hex->loadData(mc))
	{
		grid_hex->SetTable(table_hex);
		Layout();
		grid_hex->Refresh();
		return true;
	}
	else
		return false;
}


/*******************************************************************
 * HEXEDITORPANEL CLASS EVENTS
 *******************************************************************/

/* HexEditorPanel::onCellSelected
 * Called when the cell selection (focus) is changed
 *******************************************************************/
void HexEditorPanel::onCellSelected(wxGridEvent& e)
{
	if (!e.Selecting())
		return;

	// Get offset of focused cell
	uint32_t offset = table_hex->getOffset(e.GetRow(), e.GetCol());

	// Check offset
	if (offset > table_hex->getData().getSize())
		return;

	// Reset labels
	label_offset->SetLabel(S_FMT("Offset: %d", offset));
	label_byte->SetLabel("Signed Byte:");
	label_ubyte->SetLabel("Unsigned Byte:");
	label_ascii->SetLabel("ASCII:");
	label_short_le->SetLabel("Signed Short:");
	label_ushort_le->SetLabel("Unsigned Short:");
	label_int32_le->SetLabel("Signed Int (32bit):");
	label_uint32_le->SetLabel("Unsigned Int (32bit):");
	//label_int64_le->SetLabel("Signed Int (64bit):");
	//label_uint64_le->SetLabel("Unsigned Int (64bit):");
	//label_float_le->SetLabel("Float:");
	//label_double_le->SetLabel("Double:");
	label_short_be->SetLabel("Signed Short:");
	label_ushort_be->SetLabel("Unsigned Short:");
	label_int32_be->SetLabel("Signed Int (32bit):");
	label_uint32_be->SetLabel("Unsigned Int (32bit):");
	//label_int64_be->SetLabel("Signed Int (64bit):");
	//label_uint64_be->SetLabel("Unsigned Int (64bit):");
	//label_float_be->SetLabel("Float:");
	//label_double_be->SetLabel("Double:");

	// Get values
	uint32_t size = table_hex->getData().getSize() - offset;
	int8_t vbyte = 0;
	uint8_t vubyte = 0;
	int16_t vshort = 0;
	uint16_t vushort = 0;
	int32_t vint32 = 0;
	uint32_t vuint32 = 0;
	int64_t vint64 = 0;
	uint64_t vuint64 = 0;
	float vfloat = 0;
	double vdouble = 0;
	if (size > 0)
	{
		// Byte values
		vubyte = table_hex->getUByteValue(offset);
		vbyte = table_hex->getByteValue(offset);

		label_byte->SetLabel(S_FMT("Signed Byte: %d", vbyte));
		label_ubyte->SetLabel(S_FMT("Unsigned Byte: %u", vubyte));
		if (vubyte <= 128)
			label_ascii->SetLabel(S_FMT("ASCII: %s", CHR(CodePages::fromASCII(vubyte))));

		if (size > 1)
		{
			// Short values
			vshort = table_hex->getShortValue(offset);
			vushort = table_hex->getUShortValue(offset);

			label_short_le->SetLabel(S_FMT("Signed Short: %d", wxINT16_SWAP_ON_BE(vshort)));
			label_ushort_le->SetLabel(S_FMT("Unsigned Short: %u", wxUINT16_SWAP_ON_BE(vushort)));
			label_short_be->SetLabel(S_FMT("Signed Short: %d", wxINT16_SWAP_ON_LE(vshort)));
			label_ushort_be->SetLabel(S_FMT("Unsigned Short: %u", wxUINT16_SWAP_ON_LE(vushort)));

			if (size > 3)
			{
				// 4-byte values
				vint32 = table_hex->getInt32Value(offset);
				vuint32 = table_hex->getUInt32Value(offset);
				vfloat = table_hex->getFloatValue(offset);

				label_int32_le->SetLabel(S_FMT("Signed Int (32bit): %d", wxINT32_SWAP_ON_BE(vint32)));
				label_uint32_le->SetLabel(S_FMT("Unsigned Int (32bit): %u", wxUINT32_SWAP_ON_BE(vuint32)));
				label_int32_be->SetLabel(S_FMT("Signed Int (32bit): %d", wxINT32_SWAP_ON_LE(vint32)));
				label_uint32_be->SetLabel(S_FMT("Unsigned Int (32bit): %u", wxUINT32_SWAP_ON_LE(vuint32)));

				if (size > 7)
				{
					// 8-byte values
					vint64 = table_hex->getInt64Value(offset);
					vuint64 = table_hex->getUInt64Value(offset);
					vdouble = table_hex->getDoubleValue(offset);

					//label_int64_le->SetLabel(s_fmt("Signed Int (64bit): %d", wxINT64_SWAP_ON_BE(vint64)));
					//label_uint64_le->SetLabel(s_fmt("Unsigned Int (64bit): %u", wxUINT64_SWAP_ON_BE(vuint64)));
					//label_int64_be->SetLabel(s_fmt("Signed Int (64bit): %d", wxINT64_SWAP_ON_LE(vint64)));
					//label_uint64_be->SetLabel(s_fmt("Unsigned Int (64bit): %u", wxUINT64_SWAP_ON_LE(vuint64)));
				}
			}
		}
	}

	// Refresh
	Update();
}

/* HexEditorPanel::onBtnGoToOffset
 * Called when the 'Go to Offset' button is clicked
 *******************************************************************/
void HexEditorPanel::onBtnGoToOffset(wxCommandEvent& e)
{
	// Do nothing if no data
	if (table_hex->getData().getSize() == 0)
		return;

	// Pop up dialog to prompt user for an offset
	int ofs = wxGetNumberFromUser("Enter Offset", "Offset", "Go to Offset", 0, 0, table_hex->getData().getSize()-1);
	if (ofs >= 0)
	{
		// Determine row/col of offset
		int row = ofs / NUMCOLS;
		int col = ofs % NUMCOLS;

		// Go to that cell
		grid_hex->GoToCell(row, col);
		grid_hex->SetFocus();
	}
}
