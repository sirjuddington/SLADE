
#ifndef __HEX_EDITOR_PANEL_H__
#define __HEX_EDITOR_PANEL_H__

#include <wx/grid.h>

class HexTable : public wxGridTableBase
{
private:
	MemChunk data;

public:
	HexTable();
	~HexTable();

	MemChunk&	getData() { return data; }

	// Overrides
	int		GetNumberRows();
	int		GetNumberCols();
	string	GetValue(int row, int col);
	void 	SetValue(int row, int col, const string& value);

	bool		loadData(MemChunk& mc);
	uint32_t	getOffset(int row, int col);

	// Get values
	uint8_t		getUByteValue(uint32_t offset);
	uint16_t	getUShortValue(uint32_t offset);
	uint32_t	getUInt32Value(uint32_t offset);
	uint64_t	getUInt64Value(uint32_t offset);
	int8_t		getByteValue(uint32_t offset);
	int16_t		getShortValue(uint32_t offset);
	int32_t		getInt32Value(uint32_t offset);
	int64_t		getInt64Value(uint32_t offset);
	float		getFloatValue(uint32_t offset);
	double		getDoubleValue(uint32_t offset);
};

class HexEditorPanel : public wxPanel
{
private:
	wxGrid*		grid_hex;
	HexTable*	table_hex;
	wxButton*	btn_go_to_offset;

	// Info labels
	wxStaticText*	label_offset;
	wxStaticText*	label_ubyte;
	wxStaticText*	label_byte;
	wxStaticText*	label_ascii;
	wxStaticText*	label_ushort_le;
	wxStaticText*	label_uint32_le;
	wxStaticText*	label_uint64_le;
	wxStaticText*	label_short_le;
	wxStaticText*	label_int32_le;
	wxStaticText*	label_int64_le;
	wxStaticText*	label_float_le;
	wxStaticText*	label_double_le;
	wxStaticText*	label_ushort_be;
	wxStaticText*	label_uint32_be;
	wxStaticText*	label_uint64_be;
	wxStaticText*	label_short_be;
	wxStaticText*	label_int32_be;
	wxStaticText*	label_int64_be;
	wxStaticText*	label_float_be;
	wxStaticText*	label_double_be;

public:
	HexEditorPanel(wxWindow* parent);
	~HexEditorPanel();

	bool	loadData(MemChunk& mc);

	void	onCellSelected(wxGridEvent& e);
	void	onBtnGoToOffset(wxCommandEvent& e);
};

#endif//__HEX_EDITOR_PANEL_H__
