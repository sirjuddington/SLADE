#pragma once

class HexTable : public wxGridTableBase
{
public:
	HexTable() {}
	~HexTable() {}

	MemChunk& getData() { return data_; }

	// Overrides
	int    GetNumberRows() override;
	int    GetNumberCols() override;
	string GetValue(int row, int col) override;
	void   SetValue(int row, int col, const string& value) override;

	bool     loadData(MemChunk& mc);
	uint32_t getOffset(int row, int col);
	void     setViewType(int type) { view_type_ = type; }

	// Get values
	uint8_t  getUByteValue(uint32_t offset);
	uint16_t getUShortValue(uint32_t offset);
	uint32_t getUInt32Value(uint32_t offset);
	uint64_t getUInt64Value(uint32_t offset);
	int8_t   getByteValue(uint32_t offset);
	int16_t  getShortValue(uint32_t offset);
	int32_t  getInt32Value(uint32_t offset);
	int64_t  getInt64Value(uint32_t offset);
	float    getFloatValue(uint32_t offset);
	double   getDoubleValue(uint32_t offset);

private:
	MemChunk data_;
	int      view_type_ = 0;
};

class HexEditorPanel : public wxPanel
{
public:
	HexEditorPanel(wxWindow* parent);
	~HexEditorPanel() {}

	bool loadData(MemChunk& mc);

private:
	wxGrid*        grid_hex_         = nullptr;
	HexTable*      table_hex_        = nullptr;
	wxButton*      btn_go_to_offset_ = nullptr;
	wxRadioButton* rb_view_hex_      = nullptr;
	wxRadioButton* rb_view_dec_      = nullptr;
	wxRadioButton* rb_view_ascii_    = nullptr;

	// Info labels
	wxStaticText* label_offset_    = nullptr;
	wxStaticText* label_ubyte_     = nullptr;
	wxStaticText* label_byte_      = nullptr;
	wxStaticText* label_ascii_     = nullptr;
	wxStaticText* label_ushort_le_ = nullptr;
	wxStaticText* label_uint32_le_ = nullptr;
	wxStaticText* label_uint64_le_ = nullptr;
	wxStaticText* label_short_le_  = nullptr;
	wxStaticText* label_int32_le_  = nullptr;
	wxStaticText* label_int64_le_  = nullptr;
	wxStaticText* label_float_le_  = nullptr;
	wxStaticText* label_double_le_ = nullptr;
	wxStaticText* label_ushort_be_ = nullptr;
	wxStaticText* label_uint32_be_ = nullptr;
	wxStaticText* label_uint64_be_ = nullptr;
	wxStaticText* label_short_be_  = nullptr;
	wxStaticText* label_int32_be_  = nullptr;
	wxStaticText* label_int64_be_  = nullptr;
	wxStaticText* label_float_be_  = nullptr;
	wxStaticText* label_double_be_ = nullptr;

	void setupLayout();

	// Events
	void onCellSelected(wxGridEvent& e);
	void onBtnGoToOffset(wxCommandEvent& e);
	void onRBViewType(wxCommandEvent& e);
};
