#pragma once

class ModifyOffsetsDialog : public wxDialog
{
public:
	ModifyOffsetsDialog();
	~ModifyOffsetsDialog();

	Vec2i offset();
	int      alignType();
	bool     autoOffset();
	bool     relativeOffset();
	bool     xOffChange();
	bool     yOffChange();
	Vec2i calculateOffsets(int xoff, int yoff, int width, int height);

private:
	wxChoice* combo_aligntype_;

	wxTextCtrl* entry_xoff_;
	wxTextCtrl* entry_yoff_;
	wxCheckBox* cbox_relative_;

	wxRadioButton* opt_set_;
	wxRadioButton* opt_auto_;

	// Events
	void onOptSet(wxCommandEvent& e);
	void onOptAuto(wxCommandEvent& e);
};
