#pragma once

class ModifyOffsetsDialog : public wxDialog
{
public:
	ModifyOffsetsDialog();
	~ModifyOffsetsDialog() = default;

	Vec2i offset() const;
	int   alignType() const;
	bool  autoOffset() const;
	bool  relativeOffset() const;
	bool  xOffChange() const;
	bool  yOffChange() const;
	Vec2i calculateOffsets(int xoff, int yoff, int width, int height) const;

private:
	wxChoice* combo_aligntype_ = nullptr;

	wxTextCtrl* entry_xoff_    = nullptr;
	wxTextCtrl* entry_yoff_    = nullptr;
	wxCheckBox* cbox_relative_ = nullptr;

	wxRadioButton* opt_set_  = nullptr;
	wxRadioButton* opt_auto_ = nullptr;

	// Events
	void onOptSet(wxCommandEvent& e);
	void onOptAuto(wxCommandEvent& e);
};
