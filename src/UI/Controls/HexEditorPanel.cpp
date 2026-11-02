
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    HexEditorPanel.cpp
// Description: HexEditorPanel class. A panel that displays data in a hex grid,
//              and shows some basic information about the currently selected
//              byte
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "HexEditorPanel.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/CodePages.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, hex_grid_width, 16, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// HexTable Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the number of rows in the grid
// -----------------------------------------------------------------------------
int HexTable::GetNumberRows()
{
	return (data_.size() / hex_grid_width) + 1;
}

// -----------------------------------------------------------------------------
// Returns the number of columns in the grid (always 16)
// -----------------------------------------------------------------------------
int HexTable::GetNumberCols()
{
	return hex_grid_width;
}

// -----------------------------------------------------------------------------
// Returns the value of the byte at [row],[col] as a string
// -----------------------------------------------------------------------------
wxString HexTable::GetValue(int row, int col)
{
	if (static_cast<unsigned>(row * hex_grid_width + col) >= data_.size())
		return wxEmptyString;
	else
	{
		uint8_t val = data_[row * hex_grid_width + col];

		// Hex
		if (view_type_ == 0)
			return WX_FMT("{:02X}", val);

		// Dec
		else if (view_type_ == 1)
			return WX_FMT("{}", val);

		// ASCII
		else if (view_type_ == 2)
			return wxString::FromAscii(static_cast<char>(val));

		return wxEmptyString;
	}
}

// -----------------------------------------------------------------------------
// Sets the value of the byte at [row],[col]
// (does nothing, only here because it's required)
// -----------------------------------------------------------------------------
void HexTable::SetValue(int row, int col, const wxString& value)
{
	// Can't set values
}

// -----------------------------------------------------------------------------
// Loads in data from [mc]. Returns true on success, false otherwise
// -----------------------------------------------------------------------------
bool HexTable::loadData(const MemChunk& mc)
{
	data_.importMem(mc.data(), mc.size());
	return true;
}

// -----------------------------------------------------------------------------
// Returns the offset of the byte at [row],[col]
// -----------------------------------------------------------------------------
uint32_t HexTable::offset(int row, int col) const
{
	return row * hex_grid_width + col;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as an unsigned byte
// -----------------------------------------------------------------------------
uint8_t HexTable::uByteValue(uint32_t offset) const
{
	if (offset < data_.size())
		return data_[offset];
	else
		return 0;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as an unsigned short
// -----------------------------------------------------------------------------
uint16_t HexTable::uShortValue(uint32_t offset)
{
	uint16_t val = 0;
	if (offset < data_.size() - 1)
		memcpy(&val, data_.data() + offset, 2);
	return val;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as an unsigned 32-bit integer
// -----------------------------------------------------------------------------
uint32_t HexTable::uInt32Value(uint32_t offset)
{
	uint32_t val = 0;
	if (offset < data_.size() - 3)
		memcpy(&val, data_.data() + offset, 4);
	return val;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as an unsigned 64-bit integer
// -----------------------------------------------------------------------------
uint64_t HexTable::uInt64Value(uint32_t offset)
{
	uint64_t val = 0;
	if (offset < data_.size() - 7)
		memcpy(&val, data_.data() + offset, 8);
	return val;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as a signed byte
// -----------------------------------------------------------------------------
int8_t HexTable::byteValue(uint32_t offset)
{
	int8_t val = 0;
	if (offset < data_.size())
		val = *(data_.data() + offset);
	return val;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as a signed short
// -----------------------------------------------------------------------------
int16_t HexTable::shortValue(uint32_t offset)
{
	int16_t val = 0;
	if (offset < data_.size() - 1)
		memcpy(&val, data_.data() + offset, 2);
	return val;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as a signed 32-bit integer
// -----------------------------------------------------------------------------
int32_t HexTable::int32Value(uint32_t offset)
{
	int32_t val = 0;
	if (offset < data_.size() - 3)
		memcpy(&val, data_.data() + offset, 4);
	return val;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as a signed 64-bit integer
// -----------------------------------------------------------------------------
int64_t HexTable::int64Value(uint32_t offset)
{
	int64_t val = 0;
	if (offset < data_.size() - 7)
		memcpy(&val, data_.data() + offset, 8);
	return val;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as a float
// -----------------------------------------------------------------------------
float HexTable::floatValue(uint32_t offset)
{
	float val = 0;
	if (offset < data_.size() - 3)
		val = *(data_.data() + offset);
	return val;
}

// -----------------------------------------------------------------------------
// Returns the value at [offset] as a double
// -----------------------------------------------------------------------------
double HexTable::doubleValue(uint32_t offset)
{
	double val = 0;
	if (offset < data_.size() - 7)
		val = *(data_.data() + offset);
	return val;
}


// -----------------------------------------------------------------------------
//
// HexEditorPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// HexEditorPanel class constructor
// -----------------------------------------------------------------------------
HexEditorPanel::HexEditorPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	// Create controls
	rb_view_hex_   = new wxRadioButton(this, -1, wxS("Hex"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	rb_view_dec_   = new wxRadioButton(this, -1, wxS("Decimal"));
	rb_view_ascii_ = new wxRadioButton(this, -1, wxS("ASCII"));
	table_hex_     = new HexTable();
	grid_hex_     = new wxGrid(this, -1, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_SIMPLE | wxVSCROLL);
	label_offset_ = new wxStaticText(this, -1, wxS("Offset:"));
	label_byte_   = new wxStaticText(this, -1, wxS("Signed Byte:"));
	label_ubyte_  = new wxStaticText(this, -1, wxS("Unsigned Byte:"));
	label_ascii_  = new wxStaticText(this, -1, wxS("ASCII:"));
	label_short_le_   = new wxStaticText(this, -1, wxS("Signed Short:"));
	label_ushort_le_  = new wxStaticText(this, -1, wxS("Unsigned Short:"));
	label_int32_le_   = new wxStaticText(this, -1, wxS("Signed Integer (32bit):"));
	label_uint32_le_  = new wxStaticText(this, -1, wxS("Unsigned Integer (32bit):"));
	label_short_be_   = new wxStaticText(this, -1, wxS("Signed Short:"));
	label_ushort_be_  = new wxStaticText(this, -1, wxS("Unsigned Short:"));
	label_int32_be_   = new wxStaticText(this, -1, wxS("Signed Integer (32bit):"));
	label_uint32_be_  = new wxStaticText(this, -1, wxS("Unsigned Integer (32bit):"));
	btn_go_to_offset_ = new wxButton(this, -1, wxS("Go to Offset..."));

	// Setup hex grid
	auto cellsize      = FromDIP(28);
	auto scrollbarsize = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
	grid_hex_->SetDefaultRowSize(cellsize, true);
	grid_hex_->SetDefaultColSize(cellsize, true);
	grid_hex_->HideColLabels();
	grid_hex_->HideRowLabels();
	grid_hex_->EnableEditing(false);
	grid_hex_->DisableDragGridSize();
	grid_hex_->SetDefaultCellAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
	grid_hex_->SetTable(table_hex_);
	grid_hex_->SetInitialSize({ (cellsize * hex_grid_width) + scrollbarsize + 2, -1 });

	setupLayout();

	// Bind events
	grid_hex_->Bind(wxEVT_GRID_SELECT_CELL, &HexEditorPanel::onCellSelected, this);
	btn_go_to_offset_->Bind(wxEVT_BUTTON, &HexEditorPanel::onBtnGoToOffset, this);
	rb_view_hex_->Bind(wxEVT_RADIOBUTTON, &HexEditorPanel::onRBViewType, this);
	rb_view_dec_->Bind(wxEVT_RADIOBUTTON, &HexEditorPanel::onRBViewType, this);
	rb_view_ascii_->Bind(wxEVT_RADIOBUTTON, &HexEditorPanel::onRBViewType, this);

	SetInitialSize(wxDefaultSize);
	wxWindowBase::Layout();
}

// -----------------------------------------------------------------------------
// Loads data from [mc] into the hex grid
// -----------------------------------------------------------------------------
bool HexEditorPanel::loadData(const MemChunk& mc)
{
	if (table_hex_->loadData(mc))
	{
		grid_hex_->SetTable(table_hex_);
		Layout();
		grid_hex_->Refresh();
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Lays out the controls on the panel
// -----------------------------------------------------------------------------
void HexEditorPanel::setupLayout()
{
	SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Left side
	auto vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, wxSizerFlags().Expand());

	// View type
	auto lh = ui::LayoutHelper(this);
	lh.layoutHorizontally(
		vbox,
		vector<wxObject*>{ new wxStaticText(this, -1, wxS("View As:")), rb_view_hex_, rb_view_dec_, rb_view_ascii_ },
		lh.sfWithSmallBorder(0, wxBOTTOM));

	// Hex grid
	vbox->Add(grid_hex_, wxSizerFlags().Expand());

	// Right side
	vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, lh.sfWithBorder(1, wxLEFT).Expand());

	// Values
	auto frame      = new wxStaticBox(this, -1, wxS("Values (General)"));
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	lh.layoutVertically(
		framesizer, { label_offset_, label_byte_, label_ubyte_, label_ascii_ }, lh.sfWithBorder(1).Expand());
	vbox->Add(framesizer, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Little endian values
	frame      = new wxStaticBox(this, -1, wxS("Values (Little Endian)"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	lh.layoutVertically(
		framesizer,
		{ label_short_le_, label_ushort_le_, label_int32_le_, label_uint32_le_ },
		lh.sfWithBorder(1).Expand());
	vbox->Add(framesizer, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// Big endian values
	frame      = new wxStaticBox(this, -1, wxS("Values (Big Endian)"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	lh.layoutVertically(
		framesizer,
		{ label_short_be_, label_ushort_be_, label_int32_be_, label_uint32_be_ },
		lh.sfWithBorder(1).Expand());
	vbox->Add(framesizer, lh.sfWithBorder(0, wxBOTTOM).Expand());

	// 'Go to Offset' button
	vbox->Add(btn_go_to_offset_, lh.sfWithBorder(0, wxBOTTOM));
}


// -----------------------------------------------------------------------------
//
// HexEditorPanel Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the cell selection (focus) is changed
// -----------------------------------------------------------------------------
void HexEditorPanel::onCellSelected(wxGridEvent& e)
{
	if (!e.Selecting())
		return;

	// Get offset of focused cell
	uint32_t offset = table_hex_->offset(e.GetRow(), e.GetCol());

	// Check offset
	if (offset > table_hex_->getData().size())
		return;

	// Reset labels
	label_offset_->SetLabel(WX_FMT("Offset: {}", offset));
	label_byte_->SetLabel(wxS("Signed Byte:"));
	label_ubyte_->SetLabel(wxS("Unsigned Byte:"));
	label_ascii_->SetLabel(wxS("ASCII:"));
	label_short_le_->SetLabel(wxS("Signed Short:"));
	label_ushort_le_->SetLabel(wxS("Unsigned Short:"));
	label_int32_le_->SetLabel(wxS("Signed Int (32bit):"));
	label_uint32_le_->SetLabel(wxS("Unsigned Int (32bit):"));
	// label_int64_le->SetLabel(wxS("Signed Int (64bit):"));
	// label_uint64_le->SetLabel(wxS("Unsigned Int (64bit):"));
	// label_float_le->SetLabel(wxS("Float:"));
	// label_double_le->SetLabel(wxS("Double:"));
	label_short_be_->SetLabel(wxS("Signed Short:"));
	label_ushort_be_->SetLabel(wxS("Unsigned Short:"));
	label_int32_be_->SetLabel(wxS("Signed Int (32bit):"));
	label_uint32_be_->SetLabel(wxS("Unsigned Int (32bit):"));
	// label_int64_be->SetLabel(wxS("Signed Int (64bit):"));
	// label_uint64_be->SetLabel(wxS("Unsigned Int (64bit):"));
	// label_float_be->SetLabel(wxS("Float:"));
	// label_double_be->SetLabel(wxS("Double:"));

	// Get values
	uint32_t size    = table_hex_->getData().size() - offset;
	int8_t   vbyte   = 0;
	uint8_t  vubyte  = 0;
	int16_t  vshort  = 0;
	uint16_t vushort = 0;
	int32_t  vint32  = 0;
	uint32_t vuint32 = 0;
	int64_t  vint64  = 0;
	uint64_t vuint64 = 0;
	float    vfloat  = 0;
	double   vdouble = 0;
	if (size > 0)
	{
		// Byte values
		vubyte = table_hex_->uByteValue(offset);
		vbyte  = table_hex_->byteValue(offset);

		label_byte_->SetLabel(WX_FMT("Signed Byte: {}", vbyte));
		label_ubyte_->SetLabel(WX_FMT("Unsigned Byte: {}", vubyte));
		if (vubyte <= 128)
			label_ascii_->SetLabel(WX_FMT("ASCII: {}", codepages::fromASCII(vubyte)));

		if (size > 1)
		{
			// Short values
			vshort  = table_hex_->shortValue(offset);
			vushort = table_hex_->uShortValue(offset);

			label_short_le_->SetLabel(WX_FMT("Signed Short: {}", wxINT16_SWAP_ON_BE(vshort)));
			label_ushort_le_->SetLabel(WX_FMT("Unsigned Short: {}", wxUINT16_SWAP_ON_BE(vushort)));
			label_short_be_->SetLabel(WX_FMT("Signed Short: {}", wxINT16_SWAP_ON_LE(vshort)));
			label_ushort_be_->SetLabel(WX_FMT("Unsigned Short: {}", wxUINT16_SWAP_ON_LE(vushort)));

			if (size > 3)
			{
				// 4-byte values
				vint32  = table_hex_->int32Value(offset);
				vuint32 = table_hex_->uInt32Value(offset);
				vfloat  = table_hex_->floatValue(offset);

				label_int32_le_->SetLabel(WX_FMT("Signed Int (32bit): {}", wxINT32_SWAP_ON_BE(vint32)));
				label_uint32_le_->SetLabel(WX_FMT("Unsigned Int (32bit): {}", wxUINT32_SWAP_ON_BE(vuint32)));
				label_int32_be_->SetLabel(WX_FMT("Signed Int (32bit): {}", wxINT32_SWAP_ON_LE(vint32)));
				label_uint32_be_->SetLabel(WX_FMT("Unsigned Int (32bit): {}", wxUINT32_SWAP_ON_LE(vuint32)));

				if (size > 7)
				{
					// 8-byte values
					vint64  = table_hex_->int64Value(offset);
					vuint64 = table_hex_->uInt64Value(offset);
					vdouble = table_hex_->doubleValue(offset);

					// label_int64_le->SetLabel(s_fmt("Signed Int (64bit): %d", wxINT64_SWAP_ON_BE(vint64)));
					// label_uint64_le->SetLabel(s_fmt("Unsigned Int (64bit): %u", wxUINT64_SWAP_ON_BE(vuint64)));
					// label_int64_be->SetLabel(s_fmt("Signed Int (64bit): %d", wxINT64_SWAP_ON_LE(vint64)));
					// label_uint64_be->SetLabel(s_fmt("Unsigned Int (64bit): %u", wxUINT64_SWAP_ON_LE(vuint64)));
				}
			}
		}
	}

	// Refresh
	Update();
}

// -----------------------------------------------------------------------------
// Called when the 'Go to Offset' button is clicked
// -----------------------------------------------------------------------------
void HexEditorPanel::onBtnGoToOffset(wxCommandEvent& e)
{
	// Do nothing if no data
	if (table_hex_->getData().size() == 0)
		return;

	// Pop up dialog to prompt user for an offset
	int ofs = wxGetNumberFromUser(
		wxS("Enter Offset"), wxS("Offset"), wxS("Go to Offset"), 0, 0, table_hex_->getData().size() - 1);
	if (ofs >= 0)
	{
		// Determine row/col of offset
		int row = ofs / hex_grid_width;
		int col = ofs % hex_grid_width;

		// Go to that cell
		grid_hex_->GoToCell(row, col);
		grid_hex_->SetFocus();
	}
}

// -----------------------------------------------------------------------------
// Called when one of the 'View As' radio buttons is selected
// -----------------------------------------------------------------------------
void HexEditorPanel::onRBViewType(wxCommandEvent& e)
{
	// Set view type
	if (rb_view_hex_->GetValue())
		table_hex_->setViewType(0);
	else if (rb_view_dec_->GetValue())
		table_hex_->setViewType(1);
	else
		table_hex_->setViewType(2);

	// Refresh
	grid_hex_->Refresh();
}
